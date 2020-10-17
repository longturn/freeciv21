cmake_minimum_required(VERSION 3.10...3.17 FATAL_ERROR)

# Set up cmake search path
if (EMSCRIPTEN)
  message(STATUS "Using Emscripten-specific settings")
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/emscripten)
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(FreecivBuildOptions)

project(freeciv21 VERSION 3.1.0 LANGUAGES C CXX)

# After project() because the list of languages has to be known
include(FreecivDependencies)
include(FreecivHelpers)

# Put all executables at the root of the build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Include subdirectories with the actual project definitions
add_subdirectory(utility)
add_subdirectory(common)
add_subdirectory(data)

if (FREECIV_BUILD_LIBCLIENT)
  add_subdirectory(client)
endif()
if (FREECIV_BUILD_LIBSERVER)
  add_subdirectory(server)
endif()

# Always add tools, conditionals are inside
add_subdirectory(tools)

# Always install the documentation
install(
  FILES
  AUTHORS
  ChangeLog
  COPYING
  INSTALL
  NEWS
  doc/BUGS
  doc/CodingStyle
  doc/FAQ
  doc/HACKING
  doc/INSTALL.Cygwin
  doc/INSTALL.meson
  doc/HOWTOPLAY
  doc/README.achievements
  doc/README.actions
  doc/README.agents
  doc/README.AI
  doc/README.AI_modules
  doc/README.attributes
  doc/README.delta
  doc/README.effects
  doc/README.fcdb
  doc/README.governor
  doc/README.graphics
  doc/README.modpack_installer
  doc/README.msys2
  doc/README.nations
  doc/README.packaging
  doc/README.rulesets
  doc/README.tilesets
  doc/README.scenarios
  doc/README.scorelog
  doc/README.sound
  doc/TODO
  TYPE DOC)
