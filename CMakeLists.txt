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

# Set the build defaults
include(cmake/FreecivBuildOptions.cmake)

# Set some project variables to reuse later
set(FREECIV21_VERSION_MAJOR 3)
set(FREECIV21_VERSION_MINOR 1)
set(FREECIV21_VERSION_PATCH 0)
set(FREECIV21_VERSION "${FREECIV21_VERSION_MAJOR}.${FREECIV21_VERSION_MINOR}.${FREECIV21_VERSION_PATCH}")

# Set project
project(freeciv21 VERSION ${FREECIV21_VERSION} LANGUAGES C CXX)

# Gather all the things we need for Windows builds early.
if(WIN32 OR MSYS OR MINGW)
  set(CMAKE_INSTALL_DATAROOTDIR ".")
  set(CMAKE_INSTALL_BINDIR ".")
  set(PROJECT_NAME "data")
  set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/")

  get_filename_component(MINGW_PATH ${CMAKE_CXX_COMPILER} PATH)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
    ${MINGW_PATH}/libbrotlicommon.dll
    ${MINGW_PATH}/libbrotlidec.dll
    ${MINGW_PATH}/libbz2-1.dll
    ${MINGW_PATH}/libdouble-conversion.dll
    ${MINGW_PATH}/libFLAC-8.dll
    ${MINGW_PATH}/libfreetype-6.dll
    ${MINGW_PATH}/libgcc_s_seh-1.dll
    ${MINGW_PATH}/libglib-2.0-0.dll
    ${MINGW_PATH}/libgraphite2.dll
    ${MINGW_PATH}/libharfbuzz-0.dll
    ${MINGW_PATH}/libiconv-2.dll
    ${MINGW_PATH}/libicudt68.dll
    ${MINGW_PATH}/libicuin68.dll
    ${MINGW_PATH}/libicuuc68.dll
    ${MINGW_PATH}/libintl-8.dll
    ${MINGW_PATH}/libKF5Archive.dll
    ${MINGW_PATH}/liblzma-5.dll
    ${MINGW_PATH}/libmpg123-0.dll
    ${MINGW_PATH}/libogg-0.dll
    ${MINGW_PATH}/libopus-0.dll
    ${MINGW_PATH}/libopusfile-0.dll
    ${MINGW_PATH}/libpcre-1.dll
    ${MINGW_PATH}/libpcre2-16-0.dll
    ${MINGW_PATH}/libpng16-16.dll
    ${MINGW_PATH}/libreadline8.dll
    ${MINGW_PATH}/libsqlite3-0.dll
    ${MINGW_PATH}/libstdc++-6.dll
    ${MINGW_PATH}/libtermcap-0.dll
    ${MINGW_PATH}/libvorbis-0.dll
    ${MINGW_PATH}/libvorbisfile-3.dll
    ${MINGW_PATH}/libwinpthread-1.dll
    ${MINGW_PATH}/libzstd.dll
    ${MINGW_PATH}/Qt5Core.dll
    ${MINGW_PATH}/Qt5Gui.dll
    ${MINGW_PATH}/Qt5Network.dll
    ${MINGW_PATH}/Qt5Widgets.dll
    ${MINGW_PATH}/SDL2.dll
    ${MINGW_PATH}/SDL2_mixer.dll
    ${MINGW_PATH}/zlib1.dll
    )
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT "freeciv21")
  include(InstallRequiredSystemLibraries)
endif()

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

if(WIN32 OR MSYS OR MINGW)
  # Install the libraries compiled in the section above
  install(
    PROGRAMS
    ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)

  # Custom command files to run the applications
  install(
    FILES
    ${CMAKE_SOURCE_DIR}/windows/installer_msys2/freeciv21-client.cmd
    ${CMAKE_SOURCE_DIR}/windows/installer_msys2/freeciv21-modpack-qt.cmd
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)

  # Qt5 Plugins -- this is a hack
  install(
    DIRECTORY ${MINGW_PATH}/../share/qt5/plugins/platforms
    DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.dll)

  set(CMAKE_INSTALL_LOCALEDIR "${CMAKE_INSTALL_DATAROOTDIR}/locale")

endif()

# Packaging (CPack)
include(CPackConfig)
