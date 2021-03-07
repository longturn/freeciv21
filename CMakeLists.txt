cmake_minimum_required(VERSION 3.12...3.17 FATAL_ERROR)

# set vcpkg if exist
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

# Set up cmake search path
if (EMSCRIPTEN)
  message(STATUS "Using Emscripten-specific settings")
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/emscripten)
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Always generate the compilation database by default
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(FreecivBuildOptions)

set(FREECIV21_VERSION_MAJOR 3)
set(FREECIV21_VERSION_MINOR 1)
set(FREECIV21_VERSION_PATCH 0)
set(FREECIV21_VERSION "${FREECIV21_VERSION_MAJOR}.${FREECIV21_VERSION_MINOR}.${FREECIV21_VERSION_PATCH}")
project(freeciv21 VERSION ${FREECIV21_VERSION} LANGUAGES C CXX)

if (NOT ${CMAKE_BUILD_TYPE} EQUAL "Release")
  add_compile_definitions(FREECIV_DEBUG)
endif()

# After project() because the list of languages has to be known
include(FreecivDependencies)
include(FreecivHelpers)
# Put all executables at the root of the build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# We do this after all targets with third-party code have been created, so
# the options only apply to code we own.
include(EnableCompilerWarnings)

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
  COPYING
  INSTALL
  doc/BUGS
  doc/CodingStyle
  doc/FAQ
  doc/HACKING
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
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT freeciv21)

# -----------------------------------------------------------------------------
# Uninstall Target
# -----------------------------------------------------------------------------
#if(WIN32 OR MSYS OR MINGW)


#    add_custom_target(uninstall
#        "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_INSTALL_PREFIX}")
#else()
#    configure_file(
#        "${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
#        "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
#        @ONLY)
#    add_custom_target(uninstall
#        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
#endif()

# -----------------------------------------------------------------------------
# Packaging (CPack)
# -----------------------------------------------------------------------------
include(cmake/CPackConfig.cmake)

# ----------------------------------------------------------------------
# Information Summary
# ----------------------------------------------------------------------
message("")
message("------------------------------------------------------------------------")
message("Configuration Summary")
message("------------------------------------------------------------------------")
# project info
message("PROJECT_NAME:                ${PROJECT_NAME}")
message("FREECIV21_VERSION:           ${FREECIV21_VERSION}")
message("")

# cmake info
message("CMAKE_BINARY_DIR:            ${CMAKE_BINARY_DIR}")
message("CMAKE_INSTALL_PREFIX:        ${CMAKE_INSTALL_PREFIX}")
message("CMAKE_INSTALL_LIBDIR:        ${CMAKE_INSTALL_LIBDIR}")
message("PACKAGE_LOCALE_DIR           ${PACKAGE_LOCALE_DIR}")
message("CMAKE_SYSTEM_NAME:           ${CMAKE_SYSTEM_NAME}")
message("CMAKE_SYSTEM_VERSION:        ${CMAKE_SYSTEM_VERSION}")
message("CMAKE_SYSTEM_PROCESSOR:      ${CMAKE_SYSTEM_PROCESSOR}")
message("CMAKE_C_COMPILER:            ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER:          ${CMAKE_CXX_COMPILER}")
message("CMAKE_BUILD_TYPE:            ${CMAKE_BUILD_TYPE}")
message("")

if(WIN32 OR MSYS OR MINGW)
  message("CMAKE_PREFIX_PATH:           ${CMAKE_PREFIX_PATH}")
  message("CMAKE_FIND_LIBRARY_PREFIXES: ${CMAKE_FIND_LIBRARY_PREFIXES}")
  message("CMAKE_FIND_LIBRARY_SUFFIXES: ${CMAKE_FIND_LIBRARY_SUFFIXES}")
  message("CPACK_GENERATOR:             ${CPACK_GENERATOR}")
  message("CPACK_SYSTEM_NAME:           ${CPACK_SYSTEM_NAME}")
  message("")
endif()
message("------------------------------------------------------------------------")