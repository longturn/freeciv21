include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(GNUInstallDirs) # For install paths

# We install so many files... skip up-to-date messages
set(CMAKE_INSTALL_MESSAGE LAZY)

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
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Required to generate the network protocol implementation
find_package(PythonInterp 3 REQUIRED)

# Required as the main networking and utility library
find_package(Qt5 5.10 COMPONENTS Core Network REQUIRED)

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
find_package(Iconv)
if(Iconv_FOUND)
  set(HAVE_ICONV TRUE) # For compiler macro
  set(FREECIV_HAVE_ICONV TRUE) # For CMake code
endif()
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
# Lua is not binary compatible even between minor releases. We stick to Lua 5.4.
#
# The tolua program is compatible with Lua 5.4, but the library may not be (eg
# on Debian it's linked to Lua 5.2). We always build the library. When not
# cross-compiling, we can also build the program. When cross-compiling, an
# externally provided tolua program is required (or an emulator for the target
# platform, eg qemu).
if (CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
  find_package(ToLuaProgram REQUIRED)
else()
  find_package(ToLuaProgram)
endif()
add_subdirectory(dependencies/lua-5.4)
add_subdirectory(dependencies/tolua-5.2) # Will build the program if not found.

# Compression
find_package(ZLIB REQUIRED)

# Miscellaneous POSIX headers and functions
if(UNIX)
  require_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
  require_include_file("libgen.h" HAVE_LIBGEN_H)
  require_include_file("pwd.h" HAVE_PWD_H)
  require_include_file("sys/time.h" HAVE_SYS_TIME_H)
  require_include_file("sys/time.h" FREECIV_HAVE_SYS_TIME_H)
  require_include_file("sys/types.h" FREECIV_HAVE_SYS_TYPES_H)
  require_include_file("unistd.h" HAVE_UNISTD_H)
  require_include_file("unistd.h" FREECIV_HAVE_UNISTD_H)
endif()

check_function_exists("getpwuid" HAVE_GETPWUID)
# Some systems don't have a well-defined root user
if (EMSCRIPTEN)
  set(ALWAYS_ROOT TRUE)
endif()

# Networking library
if (WIN32 OR MINGW OR MSYS)
  set(FREECIV_MSWINDOWS TRUE)
endif()

if (EMSCRIPTEN)
  # This is a bit hacky and maybe it should be removed.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0")
endif()

if (FREECIV_BUILD_LIBCLIENT)
  # Version comparison library (this should really be part of utility/)
  add_subdirectory(dependencies/cvercmp)
endif()

# GUI dependencies
if (FREECIV_ENABLE_CLIENT
    OR FREECIV_ENABLE_FCMP_QT
    OR FREECIV_ENABLE_RULEDIT)
  # May want to relax the version later
  find_package(Qt5 5.10 COMPONENTS Widgets REQUIRED)
endif()

# FCMP-specific dependencies
if (FREECIV_ENABLE_FCMP_CLI OR FREECIV_ENABLE_FCMP_QT)
  find_package(SQLite3 REQUIRED)
endif()
