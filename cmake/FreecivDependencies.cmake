include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)

# errors out if an include file is not found
macro(require_include_file file variable)
  check_include_file(${file} ${variable})
  if(NOT ${variable})
    message(FATAL_ERROR "Cannot find header ${file}")
  endif()
endmacro()

# errors out if a function is not found
macro(require_function_exists function variable)
  check_function_exists(${function} ${variable})
  if(NOT ${variable})
    message(FATAL_ERROR "Cannot find function ${function}")
  endif()
endmacro()

# Language support
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# Provided by any C99 compiler, no need to check them
set(HAVE_SIGNAL_H TRUE)
set(FREECIV_HAVE_INTTYPES_H TRUE)
set(FREECIV_HAVE_LOCALE_H TRUE)
set(FREECIV_HAVE_STDINT_H TRUE)

# Required to generate the network protocol implementation
find_package(PythonInterp 3 REQUIRED)

# Required for utility
find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
  set(FREECIV_HAVE_PTHREAD TRUE)
  set(FREECIV_HAVE_THREAD_COND TRUE) # Have condition variables
elseif (CMAKE_USE_WIN32_THREADS_INIT)
  set(FREECIV_HAVE_WINTHREADS TRUE)
endif()
# Not supported by FindThreads
set(FREECIV_C11_THR FALSE)
set(FREECIV_HAVE_TINYCTHR FALSE)

# Required for utility
find_package(CURL REQUIRED)
find_package(ICU COMPONENTS uc REQUIRED)
find_package(Readline)
if(Readline_FOUND)
  set(FREECIV_HAVE_LIBREADLINE TRUE)
endif()

# Internationalization
if(FREECIV_ENABLE_NLS)
  find_package(Intl REQUIRED)
  set(FREECIV_HAVE_LIBINTL_H TRUE)
endif()

# Lua
#
# Lua is not binary compatible even between minor releases. We stick to Lua 5.3.
#
# The tolua program is compatible with Lua 5.3, but the library may not be (eg
# on Debian it's linked to Lua 5.2). We always build the library. When not
# cross-compiling, we can also build the program. When cross-compiling, an
# externally provided tolua program is required.
if(CMAKE_CROSSCOMPILING)
  find_package(ToLuaProgram REQUIRED)
else()
  find_package(ToLuaProgram)
endif()
add_subdirectory(dependencies/lua-5.3)
add_subdirectory(dependencies/tolua-5.2) # Will build the program if not found.

# Compression
find_package(ZLIB REQUIRED)

# Miscellaneous POSIX headers and functions
if(UNIX)
  require_include_file("dirent.h" FREECIV_HAVE_DIRENT_H)
  require_include_file("pwd.h" HAVE_PWD_H)
  require_include_file("sys/time.h" HAVE_SYS_TIME_H)
  require_include_file("sys/time.h" FREECIV_HAVE_SYS_TIME_H)
  require_include_file("sys/types.h" FREECIV_HAVE_SYS_TYPES_H)
  require_include_file("sys/uio.h" HAVE_SYS_UIO_H)
  require_include_file("unistd.h" HAVE_UNISTD_H)
  require_include_file("unistd.h" FREECIV_HAVE_UNISTD_H)

  require_function_exists(opendir FREECIV_HAVE_OPENDIR)
  require_function_exists(getaddrinfo HAVE_GETADDRINFO)
  require_function_exists(getnameinfo HAVE_GETNAMEINFO)

  set(CMAKE_EXTRA_INCLUDE_FILES "netinet/in.h")
  check_type_size(ip_mreqn SIZEOF_IP_MREQN)
  unset(CMAKE_EXTRA_INCLUDE_FILES)
endif()

# Networking library
# TODO Windows support
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
if(HAVE_SYS_SOCKET_H)
  # Use socket.h if supported
  check_include_file("sys/socket.h" FREECIV_HAVE_SYS_SOCKET_H)
  check_c_source_compiles("
    #include <sys/socket.h>
    socklen_t *x;
    void main() {}"
    FREECIV_HAVE_SOCKLEN_T)

  require_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
  require_include_file("netdb.h" HAVE_NETDB_H)
  require_include_file("netinet/in.h" HAVE_NETINET_IN_H)
  require_include_file("netinet/in.h" FREECIV_HAVE_NETINET_IN_H)
  require_include_file("sys/select.h" HAVE_SYS_SELECT_H)
  require_include_file("sys/select.h" FREECIV_HAVE_SYS_SELECT_H)

  # IPv6 functions of POSIX-2001, not strictly required
  check_function_exists(inet_pton HAVE_INET_PTON)
  check_function_exists(inet_ntop HAVE_INET_NTOP)
  check_c_source_compiles("
    #include <sys/socket.h>
    void main()
    {
      socket(AF_INET6, SOCK_STREAM, 0);
    }"
    HAVE_AF_INET6)
  if(HAVE_INET_PTON AND HAVE_INET_NTOP AND HAVE_AF_INET6)
    set(FREECIV_IPV6_SUPPORT TRUE)
  else()
    set(FREECIV_IPV6_SUPPORT FALSE)
  endif()
else()
  error("Could not find a supported networking library")
endif()

# Client-specific dependencies
if(FREECIV_ENABLE_CLIENT)
  # Version comparison library (this should really be part of utility/)
  add_subdirectory(dependencies/cvercmp)

  # May want to relax the version later
  find_package(Qt5 5.10 COMPONENTS Widgets REQUIRED)
endif()
