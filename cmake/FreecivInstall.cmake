#############################################
# Installation configuration for Freeciv21
#############################################

# Always install the base documentation
install(
  FILES
  AUTHORS
  COPYING
  INSTALL
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT freeciv21)

# Always install the Licenses
install(
  FILES
  dist/licenses/0-MSYS2-INDEX.txt
  dist/licenses/APACHE-2.txt
  dist/licenses/BSD-2-CLAUSE.txt
  dist/licenses/BSD-3-CLAUSE.txt
  dist/licenses/BZ2.txt
  dist/licenses/FTL.txt
  dist/licenses/GPL2.txt
  dist/licenses/GPL3.txt
  dist/licenses/IMAGEMAGICK.txt
  dist/licenses/LGPL-2.0-ONLY.txt
  dist/licenses/LGPL-2.0-OR-LATER.txt
  dist/licenses/LGPL-3.0-ONLY.txt
  dist/licenses/LICENSEREF-KDE-ACCEPTED-LGPL.txt
  dist/licenses/MAGICKWAND.txt
  dist/licenses/MIT.txt
  dist/licenses/OPENSSL.txt
  dist/licenses/PNG.txt
  dist/licenses/UNICODE.txt
  dist/licenses/ZLIB.txt
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/licenses
  COMPONENT freeciv21)

# Common installation for all Win32 et al platforms
if(WIN32 OR MSYS OR MINGW)
  # Custom command files to run the applications
  install(
    FILES
    dist/freeciv21-server.cmd
    dist/client.ico
    dist/mp.ico
    dist/server.ico
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)
endif()

# MSYS2 and MINGW specific installation
if(MSYS OR MINGW)
  # Install OpenSSL library, not found with GET_RUNTIME_DEPENDENCIES
  if("$ENV{MSYSTEM}" STREQUAL "MINGW32")
    install(
      FILES
      ${MINGW_PATH}/libcrypto-3.dll
      ${MINGW_PATH}/libssl-3.dll
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT freeciv21)
  else()
    install(
      FILES
      ${MINGW_PATH}/libcrypto-3-x64.dll
      ${MINGW_PATH}/libssl-3-x64.dll
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT freeciv21)
  endif()

  # This allows us to determine the external libraries we need to include at install time
  #   dynamically instead of doing it manually.
  install(CODE [[
    message(STATUS "Collecting dependencies for freeciv21 executables...")
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)

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
  #   Before installation, run a series of commands that copy each of the Qt
  #   runtime files to the appropriate directory for installation
  install(CODE [[

    message(STATUS "Collecting Qt dependencies for freeciv21 GUI executables...")

    # Take a variable that is available at "install" time and repurpose
    string(REGEX REPLACE "objdump.exe" "" MINGW_PATH ${CMAKE_OBJDUMP})

    # Run Qt's windeployqt.exe to find the required DLLs for the GUI apps.
    execute_process(
      COMMAND ${MINGW_PATH}/windeployqt.exe --no-translations --no-virtualkeyboard --no-compiler-runtime
        --no-webkit2 --no-angle --no-opengl-sw --list mapping ${CMAKE_INSTALL_PREFIX}
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
elseif(WIN32)
  # The Visual Studio generator places all files and associated DLL libraries
  #  into a build directory. So we just grab those for install.
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.dll PATTERN *.pdb)

  # QT platform plugin must be only one directory level away from the EXEs
  #   so we ensure that here. This does create a duplicate file in install
  #   directory, but since we are not packaging in Visual Studio it isn't
  #   issue.
  install(
    DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/plugins/platforms
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.dll)

  # Grab a few files that get missed
  install(
    FILES
    ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/SDL2.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)
endif()

# Unix/Linux specific install steps
if(UNIX AND NOT APPLE)

  # Fixes a bug of some sort on Linux where this gets set to /usr/local, but installs to /usr
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr"
         CACHE PATH "default install path" FORCE)
    message(STATUS "CMAKE_INSTALL_PREFIX changed from the default to /usr.")
  endif()

  # Install MetaInfo and Desktop files for the applications asked for at configure
  if(FREECIV_ENABLE_CLIENT)
    install(
      FILES
      dist/net.longturn.freeciv21.client.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT freeciv21
    )
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.client.desktop.in
                   net.longturn.freeciv21.client.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.client.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT freeciv21
    )
  endif(FREECIV_ENABLE_CLIENT)

  if(FREECIV_ENABLE_SERVER)
    install(
      FILES
      dist/net.longturn.freeciv21.server.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT freeciv21
    )
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.server.desktop.in
                   net.longturn.freeciv21.server.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.server.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT freeciv21
    )
  endif(FREECIV_ENABLE_SERVER)

  if(FREECIV_ENABLE_FCMP_QT)
    install(
      FILES
      dist/net.longturn.freeciv21.modpack.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT freeciv21
    )
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.modpack-qt.desktop.in
                   net.longturn.freeciv21.modpack-qt.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.modpack-qt.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT freeciv21
    )
  endif(FREECIV_ENABLE_FCMP_QT)

 if(FREECIV_ENABLE_RULEDIT)
    install(
      FILES
      dist/net.longturn.freeciv21.ruledit.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT tool_ruledit
    )
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.ruledit.desktop.in
                   net.longturn.freeciv21.ruledit.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.ruledit.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT tool_ruledit
    )
  endif(FREECIV_ENABLE_RULEDIT)
endif(UNIX AND NOT APPLE)

# We grab the Libertinus Font package online for the client
if(FREECIV_ENABLE_CLIENT AND FREECIV_DOWNLOAD_FONTS)
  message(STATUS "Downloading Libertinus Font Package")

  include(ExternalProject)
  ExternalProject_Add(Libertinus
    PREFIX ${CMAKE_BINARY_DIR}
    LOG_DIR ${CMAKE_BINARY_DIR}
    DOWNLOAD_NO_PROGRESS TRUE
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    TIMEOUT 300
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    URL https://github.com/alerque/libertinus/releases/download/v7.040/Libertinus-7.040.zip
    URL_HASH SHA256=2cce08507441d8ae7b835cfe51fb643ad5d9f6b44db4360c4e244f0e474a72f6
  )

  if(MSYS OR MINGW OR WIN32)
    install(
      DIRECTORY ${CMAKE_BINARY_DIR}/src/Libertinus
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/fonts
      COMPONENT freeciv21
      FILES_MATCHING PATTERN *.otf PATTERN *.txt
    )
  else()
      install(
      DIRECTORY ${CMAKE_BINARY_DIR}/src/Libertinus
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/freeciv21/fonts
      COMPONENT freeciv21
      FILES_MATCHING PATTERN *.otf PATTERN *.txt
    )
  endif(MSYS OR MINGW OR WIN32)
endif()

