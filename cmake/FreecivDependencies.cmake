include(CheckCSourceCompiles)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(GNUInstallDirs) # For install paths

# We install so many files... skip up-to-date messages
set(CMAKE_INSTALL_MESSAGE LAZY)

# Language support
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Should be available in C++11 but not all systems have it
check_function_exists(at_quick_exit HAVE_AT_QUICK_EXIT)

# Required to generate the network protocol implementation
find_package(PythonInterp 3 REQUIRED)

# Required as the main networking and utility library
find_package(Qt5 5.10 COMPONENTS Core Network REQUIRED)

# Required for utility
if(FREECIV_ENABLE_SERVER)
  find_package(Readline REQUIRED)
  check_symbol_exists(rl_completion_suppress_append "readline/readline.h" HAVE_SUPPRESS_APPEND)
endif()

# Internationalization
add_custom_target(freeciv_translations)
if(FREECIV_ENABLE_NLS)
  find_package(Intl REQUIRED)
  if (${CMAKE_VERSION} VERSION_EQUAL 3.20.0 AND NOT Intl_LIBRARIES)
    message(FATAL_ERROR
      "Due to a bug in CMake, translations can not be used with "
      "CMake 3.20.0 on your system. "
      "You can turn off translations by passing -DFREECIV_ENABLE_NLS=NO to "
      "CMake at configure time. "
      "Please see https://github.com/longturn/freeciv21/issues/383")
  endif()

  set(FREECIV_HAVE_LIBINTL_H TRUE)
  set(ENABLE_NLS TRUE)
  if(UNIX)
    set(LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")
  elseif(WIN32 OR MSYS OR MINGW)
    set(LOCALEDIR "${CMAKE_INSTALL_LOCALEDIR}")
  endif()
  include(GettextTranslate)
  set(GettextTranslate_GMO_BINARY TRUE)
  set(GettextTranslate_POT_BINARY TRUE)
  add_subdirectory(translations/core)
  add_subdirectory(translations/nations)
  add_dependencies(freeciv_translations freeciv21-core.pot-update)
  add_dependencies(freeciv_translations freeciv21-nations.pot-update)
  if (FREECIV_BUILD_TOOLS EQUAL ON)
    add_subdirectory(translations/ruledit)
    add_dependencies(ruledit_translations freeciv21-ruledit.pot-update)
  endif()
  add_dependencies(freeciv_translations update-po)
  add_dependencies(freeciv_translations update-gmo)
endif()

# SDL2 for audio
find_package(SDL2 QUIET)
find_package(SDL2_mixer QUIET)
if (SDL2_MIXER_LIBRARIES AND SDL2_LIBRARY)
  set(AUDIO_SDL TRUE)
endif()
if (NOT SDL2_LIBRARY)
  message("SDL2 not found")
  set(SDL2_INCLUDE_DIR "")
endif()
if (NOT SDL2_MIXER_LIBRARIES)
  message("SDL2_mixer not found")
  set(SDL2_MIXER_LIBRARIES "")
  set(SDL2_MIXER_INCLUDE_DIR "")
endif()

# Lua
#
# Lua is not binary compatible even between minor releases. We stick to Lua 5.3.
#
# The tolua program is compatible with Lua 5.3, but the library may not be (eg
# on Debian it's linked to Lua 5.2). We always build the library. When not
# cross-compiling, we can also build the program. When cross-compiling, an
# externally provided tolua program is required (or an emulator for the target
# platform, eg qemu).
find_package(Lua 5.3 REQUIRED)

# Create an imported target since it's not created by CMake :(
# Get a library name for IMPORTED_LOCATION
if (NOT EMSCRIPTEN AND NOT APPLE)
  add_library(lua UNKNOWN IMPORTED GLOBAL)
  list(GET LUA_LIBRARIES 0 loc)
  set_target_properties(lua PROPERTIES
    IMPORTED_LOCATION "${loc}"
    INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}")
  # Link to all libs, not just the first
  target_link_libraries(lua INTERFACE "${LUA_LIBRARIES}")
endif()

if (CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
  find_package(ToLuaProgram REQUIRED)
else()
  find_package(ToLuaProgram)
endif()
add_subdirectory(dependencies/tolua-5.2) # Will build the program if not found.
add_subdirectory(dependencies/sol2)

# backward-cpp
include(FreecivBackward)

# Compression
find_package(KF5Archive REQUIRED)
set(FREECIV_HAVE_BZ2 ${KArchive_HAVE_BZIP2})
set(FREECIV_HAVE_LZMA ${KArchive_HAVE_LZMA})
set(FREECIV_HAVE_ZSTD ${KArchive_HAVE_ZSTD})

find_package(ZLIB REQUIRED) # Network protocol code

# Some systems don't have a well-defined root user
if (EMSCRIPTEN)
  set(ALWAYS_ROOT TRUE)
endif()

# Networking library
if (WIN32 OR MINGW OR MSYS)
  set(FREECIV_MSWINDOWS TRUE)
endif()

# Define the GUI type for Win32 Qt programs
# Removes the console window that pops up with the GUI app
if (WIN32 OR MINGW OR MSYS)
  set(GUI_TYPE WIN32)
else()
  set(GUI_TYPE "")
endif()

if (EMSCRIPTEN)
  # This is a bit hacky and maybe it should be removed.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=0")
endif()

if (FREECIV_BUILD_LIBCLIENT
    OR FREECIV_ENABLE_FCMP_CLI
    OR FREECIV_ENABLE_FCMP_QT)
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
