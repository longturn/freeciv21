cmake_minimum_required(VERSION 3.10)

# Project settings
project(freeciv21 VERSION 2.6.2)
set(BUG_URL "https://github.com/longturn/freeciv/issues")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if(EMSCRIPTEN)
  set(USE_FLAGS "-s USE_ZLIB=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0")
  set(CMAKE_EXECUTABLE_SUFFIX .js)
endif()

# Dependencies
if(NOT EMSCRIPTEN)
  find_package(CURL REQUIRED)
  find_package(ICU COMPONENTS uc REQUIRED)
  find_package(ZLIB REQUIRED)
endif()
find_package(Lua 5.3)
if(NOT LUA_FOUND)
  message(STATUS "Using built-in lua")
  set(BUILD_LUA TRUE)
endif()
find_package(PythonInterp 3 REQUIRED)

find_program(TOLUA_EXECUTABLE tolua)
if(NOT TOLUA_EXECUTABLE)
  message(STATUS "Using built-in tolua")
  set(BUILD_TOLUA TRUE)
endif()

# Detect common headers
include(CMakeBackwardCompatibilityC)
include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckVariableExists)

# Freeciv features not supported by this build system
set(FREECIV_WEB FALSE)
set(FREECIV_TESTMATIC FALSE)
set(FREECIV_HAVE_XML_REGISTRY FALSE)
set(FREECIV_JSON_CONNECTION FALSE)
set(FREECIV_SOCKET_ZERO_NOT_STDIN FALSE) # BeOS

# Defaults
set(FREECIV_META_URL "http://meta.freeciv.org/metaserver.php"
    CACHE STRING "Metaserver URL")
set(FREECIV_RELEASE_MONTH 0 TRUE CACHE STRING "Month of next release")
set(FREECIV_DELTA_PROTOCOL TRUE CACHE BOOL "Whether to enable delta protocol")
set(FREECIV_STORAGE_DIR "~/.freeciv"
    CACHE STRING "Location for freeciv to store its information")

set(FREECIV_DEBUG TRUE CACHE BOOL "Extra debugging support")
set(FREECIV_NDEBUG FALSE CACHE BOOL "No specific debugging support at all")

set(FREECIV_AI_MOD_LAST 2 CACHE STRING "Maximum number of AI modules")

# check_include_file("inttypes.h" FREECIV_HAVE_INTL_H)
check_include_file("libint.h" FREECIV_HAVE_LIBINT_H)
check_include_file("readline/readline.h" HAVE_READLINE_H)
find_library(READLINE_LIBRARY readline)
if(HAVE_READLINE_H AND READLINE_LIBRARY)
  set(FREECIV_HAVE_LIBREADLINE TRUE)
else()
  set(FREECIV_HAVE_LIBREADLINE FALSE)
endif()
check_include_file("stdbool.h" FREECIV_HAVE_STDBOOL_H)

# Check for compiler features
if(c_static_assert IN_LIST CMAKE_C_COMPILE_FEATURES)
  set(FREECIV_C11_STATIC_ASSERT TRUE)

  check_c_source_compiles("
    #include <assert.h>
    #include <string.h>
    static const char str[] = \"12345\";
    void main() {
      _Static_assert(5 == strlen(str), \"Wrong length\");
    }"
    FREECIV_STATIC_STRLEN)
endif()

# Check for compiler features
if(cxx_static_assert IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  set(FREECIV_CXX11_STATIC_ASSERT TRUE)
endif()

# Enable i18n if supported
find_package(Intl)
set(FREECIV_ENABLE_NLS ${Intl_FOUND})
set(FREECIV_HAVE_LIBINTL_H ${Intl_FOUND})

# Detect the thread library
# Freeciv supports C11 threads but the find module doesn't
find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
  set(FREECIV_HAVE_PTHREAD TRUE)
  set(FREECIV_HAVE_THREAD_COND TRUE) # Have condition variables
elseif (CMAKE_USE_WIN32_THREADS_INIT)
  set(FREECIV_HAVE_WINTHREADS TRUE)
endif()
set(FREECIV_HAVE_TINYCTHR FALSE)

# Detect the networking library
check_c_source_compiles("
  #include <sys/socket.h>
  socklen_t *x;
  void main() {}"
  FREECIV_HAVE_SOCKLEN_T)

check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("pwd.h" HAVE_PWD_H)
check_include_file("signal.h" HAVE_SIGNAL_H)
check_include_file("sys/uio.h" HAVE_SYS_UIO_H)

check_function_exists(fcntl HAVE_FCNTL)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_function_exists(ioctl HAVE_IOCTL)
check_include_file("sys/ioctl.h" HAVE_SYS_IOCTL_H)
if((HAVE_FCNTL AND HAVE_FCNTL_H) OR (HAVE_IOCTL AND HAVE_SYS_IOCTL_H))
  set(NONBLOCKING_SOCKETS TRUE)
else()
  message(FATAL_ERROR "Non-blocking sockets are not available")
endif()

check_include_file("inttypes.h" FREECIV_HAVE_INTTYPES_H)
check_include_file("stdint.h" FREECIV_HAVE_STDINT_H)
check_include_file("dirent.h" FREECIV_HAVE_DIRENT_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
set(FREECIV_HAVE_NETINET_IN_H ${HAVE_NETINET_IN_H})
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
set(FREECIV_HAVE_SYS_SELECT_H ${HAVE_SYS_SELECT_H})
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
set(FREECIV_HAVE_SYS_SOCKET_H ${HAVE_SYS_SOCKET_H})
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
set(FREECIV_HAVE_SYS_TIME_H ${HAVE_SYS_TIME_H})
check_include_file("sys/types.h" FREECIV_HAVE_SYS_TYPES_H)
check_include_file("unistd.h" FREECIV_HAVE_UNISTD_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("ws2tcpip.h" FREECIV_HAVE_WS2TCPIP_H)
check_include_file("winsock.h" FREECIV_HAVE_WINSOCK)
check_include_file("winsock2.h" FREECIV_HAVE_WINSOCK2)
set(FREECIV_MSWINDOWS ${WIN32})
check_include_file("locale.h" FREECIV_HAVE_LOCALE_H)
check_function_exists(opendir FREECIV_HAVE_OPENDIR)
check_function_exists(getaddrinfo HAVE_GETADDRINFO)
check_function_exists(inet_pton HAVE_INET_PTON)
check_function_exists(inet_ntop HAVE_INET_NTOP)
check_function_exists(getnameinfo HAVE_GETNAMEINFO)
check_c_source_compiles("
  #include <sys/socket.h>
  void main()
  {
    socket(AF_INET6, SOCK_STREAM, 0);
  }"
  HAVE_AF_INET6)
if(HAVE_GETADDRINFO AND HAVE_INET_PTON AND HAVE_INET_NTOP AND HAVE_GETNAMEINFO AND HAVE_AF_INET6)
  set(FREECIV_IPV6_SUPPORT TRUE)
else()
  set(FREECIV_IPV6_SUPPORT FALSE)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES "netinet/in.h")
check_type_size(ip_mreqn SIZEOF_IP_MREQN)
unset(CMAKE_EXTRA_INCLUDE_FILES)

add_subdirectory(dependencies)
add_subdirectory(utility)
add_subdirectory(common)
add_subdirectory(client)
if(!EMSCRIPTEN)
  add_subdirectory(server)
  add_subdirectory(tools)
endif()
