cmake_minimum_required(VERSION 3.10)

# Project settings
project(freeciv21 VERSION 3.1.0)
set(BUG_URL "https://github.com/longturn/freeciv/issues")

# Dependencies
find_package(CURL REQUIRED)
find_package(ICU COMPONENTS uc REQUIRED)
find_package(Lua 5.3)
if(NOT LUA_FOUND)
  set(BUILD_LUA TRUE)
endif()
find_package(PythonInterp 3 REQUIRED)
find_package(ZLIB REQUIRED)

# Detect common headers
include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)

# Freeciv features not supported by this build system
set(FREECIV_WEB FALSE)
set(FREECIV_TESTMATIC FALSE)
set(FREECIV_HAVE_XML_REGISTRY FALSE)
set(FREECIV_JSON_CONNECTION FALSE)
set(FREECIV_SOCKET_ZERO_NOT_STDIN FALSE) # BeOS
set(FREECIV_IPV6_SUPPORT FALSE)

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
check_include_file("readline/readline.h" FREECIV_HAVE_LIBREADLINE)
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
check_include_file("unistd.h" HAVE_UNISTD_H)

check_include_file("inttypes.h" FREECIV_HAVE_INTTYPES_H)
check_include_file("stdint.h" FREECIV_HAVE_STDINT_H)
check_include_file("dirent.h" FREECIV_HAVE_DIRENT_H)
check_include_file("netinet/in.h" FREECIV_HAVE_NETINET_IN_H)
check_include_file("sys/select.h" FREECIV_HAVE_SYS_SELECT_H)
check_include_file("sys/socket.h" FREECIV_HAVE_SYS_SOCKET_H)
check_include_file("sys/time.h" FREECIV_HAVE_SYS_TIME_H)
check_include_file("sys/types.h" FREECIV_HAVE_SYS_TYPES_H)
check_include_file("unistd.h" FREECIV_HAVE_UNISTD_H)
check_include_file("ws2tcpip.h" FREECIV_HAVE_WS2TCPIP_H)
check_include_file("winsock.h" FREECIV_HAVE_WINSOCK)
check_include_file("winsock2.h" FREECIV_HAVE_WINSOCK2)
set(FREECIV_MSWINDOWS ${WIN32})
check_include_file("locale.h" FREECIV_HAVE_LOCALE_H)
check_function_exists(opendir FREECIV_HAVE_OPENDIR)

add_subdirectory(dependencies)
add_subdirectory(utility)
add_subdirectory(common)
add_subdirectory(client)