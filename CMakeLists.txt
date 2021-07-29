# Win32 et al requires cmake version 3.16 because we use a newer command
#   "file(GET_RUNTIME_DEPENDENCIES ..." that is only available at v3.16+
# On other platforms (*nix, MacOS) we need to support older 3.12+ for the
#   server and other components
if(WIN32 OR MSYS OR MINGW)
  cmake_minimum_required(VERSION 3.16...3.20 FATAL_ERROR)
else()
  cmake_minimum_required(VERSION 3.12...3.17 FATAL_ERROR)
endif()

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
set(FREECIV21_VERSION_MINOR 0)
set(FREECIV21_VERSION_PATCH 20210721)
set(FREECIV21_VERSION "${FREECIV21_VERSION_MAJOR}.${FREECIV21_VERSION_MINOR}.${FREECIV21_VERSION_PATCH}")

# Set project
project(freeciv21 VERSION ${FREECIV21_VERSION} LANGUAGES C CXX)

# Gather all the things we need for Windows builds early.
if(WIN32 OR MSYS OR MINGW)
  # We need to alter the out of box values of these variables for Win32 et al builds
  set(CMAKE_INSTALL_DATAROOTDIR ".")
  set(CMAKE_INSTALL_BINDIR ".")
  set(PROJECT_NAME "data")
  set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/")
  get_filename_component(MINGW_PATH ${CMAKE_CXX_COMPILER} PATH)
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

# Add docs
add_subdirectory(docs)

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

# Installing on Win32 et al platforms requires some custom configs
if(WIN32 OR MSYS OR MINGW)
  # Custom command files to run the applications
  install(
    FILES
    ${CMAKE_SOURCE_DIR}/windows/installer_msys2/freeciv21-server.cmd
    ${CMAKE_SOURCE_DIR}/windows/client.ico
    ${CMAKE_SOURCE_DIR}/windows/mp.ico
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)

  # Install the ca-bundle for the modpack installer
  install(
    FILES 
    ${MINGW_PATH}/../ssl/certs/ca-bundle.crt
    DESTINATION ${CMAKE_INSTALL_BINDIR}/ssl/certs
    COMPONENT freeciv21)

  # Install OpenSSL library, not found with GET_RUNTIME_DEPENDENCIES
  install(
    FILES
    ${MINGW_PATH}/libcrypto-1_1-x64.dll
    ${MINGW_PATH}/libssl-1_1-x64.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)
    
  # This allows us to determine the external libraries we need to include at install time
  #   dynamically instead of doing it manually.
  install(CODE [[
    message(STATUS "Collecting dependencies for freeciv21 executables...")

    # Take a variable that is available at "install" time and repurpose
    string(REGEX REPLACE "objdump.exe" "" MINGW_PATH ${CMAKE_OBJDUMP})

    # Function to analyze the third party dll files linked to the exe's
    #   Uses the repurposed variable from above to tell the function where
    #   the dll files are located. Ignores dll's that come with Windows.
    file(GET_RUNTIME_DEPENDENCIES
      RESOLVED_DEPENDENCIES_VAR r_deps
      UNRESOLVED_DEPENDENCIES_VAR u_deps
      DIRECTORIES ${MINGW_PATH}
      PRE_EXCLUDE_REGEXES "^api-ms-*"
      POST_EXCLUDE_REGEXES "C:[\\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/].*"
      EXECUTABLES
        "${CMAKE_INSTALL_PREFIX}/freeciv21-*.exe"
      )
      message(STATUS "Installing library dependencies for freeciv21 executables...")
      file(INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX} MESSAGE_LAZY FILES ${r_deps})
    ]] COMPONENT freeciv21)
    
  # Qt5 Plugins and required DLLs
  # Before installation, run a series of commands that copy each of the Qt
  # runtime files to the appropriate directory for installation
  install(CODE [[
  
    message(STATUS "Collecting Qt dependencies for freeciv21 GUI executables...")

    # Take a variable that is available at "install" time and repurpose
    string(REGEX REPLACE "objdump.exe" "" MINGW_PATH ${CMAKE_OBJDUMP})
    
    # Run Qt's windeployqt.exe to find the required DLLs for the GUI apps.
    execute_process(
      COMMAND ${MINGW_PATH}/windeployqt.exe --no-translations --no-virtualkeyboard --no-compiler-runtime --no-webkit2 --no-angle --no-opengl-sw --list mapping ${CMAKE_INSTALL_PREFIX}
      OUTPUT_VARIABLE _output
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Run a loop to go thought the output and copy the files we need
    message(STATUS "Installing Qt library dependencies for freeciv21 GUI executables...")
    separate_arguments(_files WINDOWS_COMMAND ${_output})
      while(_files)
          list(GET _files 0 _src)
          list(GET _files 1 _dest)
          execute_process(
            COMMAND cp ${_src} "${CMAKE_INSTALL_PREFIX}/${_dest}"
          )
          message(STATUS "Installing: ${CMAKE_INSTALL_PREFIX}/${_dest}")
          list(REMOVE_AT _files 0 1)
      endwhile()
    ]] COMPONENT freeciv21)

endif()

# Packaging (CPack)
include(CPackConfig)
