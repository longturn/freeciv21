#############################################
# Installation configuration for Freeciv21
#############################################

# Always install the base documentation
install(
  FILES
  ${CMAKE_SOURCE_DIR}/AUTHORS
  ${CMAKE_SOURCE_DIR}/COPYING
  ${CMAKE_SOURCE_DIR}/INSTALL
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT freeciv21)

# Always install the Licenses
install(
  FILES
  ${CMAKE_SOURCE_DIR}/dist/licenses/0-MSYS2-INDEX.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/APACHE-2.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/BSD-2-CLAUSE.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/BSD-3-CLAUSE.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/BZ2.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/FTL.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/GPL2.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/GPL3.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/LGPL-2.0-ONLY.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/LGPL-2.0-OR-LATER.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/LGPL-3.0-ONLY.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/LICENSEREF-KDE-ACCEPTED-LGPL.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/MIT.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/OPENSSL.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/PNG.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/UNICODE.txt
  ${CMAKE_SOURCE_DIR}/dist/licenses/ZLIB.txt
  DESTINATION ${CMAKE_INSTALL_DOCDIR}/licenses
  COMPONENT freeciv21)

# Common installation for all Win32 et al platforms
if(WIN32 OR MSYS)
  # Custom command files to run the applications
  install(
    FILES
    ${CMAKE_SOURCE_DIR}/data/icons/128x128/freeciv21-client.ico
    ${CMAKE_SOURCE_DIR}/data/icons/128x128/freeciv21-modpack.ico
    ${CMAKE_SOURCE_DIR}/data/icons/128x128/freeciv21-server.ico
    ${CMAKE_SOURCE_DIR}/dist/windows/freeciv21-server.cmd
    ${CMAKE_SOURCE_DIR}/dist/windows/qt.conf
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)
endif()

# MSYS2 specific installation
if(MSYS OR MINGW)
  # Install OpenSSL library, not found with GET_RUNTIME_DEPENDENCIES
  install(
    FILES
    ${CLANG_PATH}/libcrypto-3-x64.dll
    ${CLANG_PATH}/libssl-3-x64.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)

  # This allows us to determine the external libraries we need to include at install time
  # dynamically instead of doing it manually.
  install(CODE [[
    message(STATUS "Collecting dependencies for freeciv21 executables...")
    set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL objdump)

    # We need the CLANG_PATH variable that isn't available in a install(CODE ) block.
    # So we find the llvm-objdump.exe and use its path to set
    string(REGEX REPLACE "llvm-objdump.exe" "" CLANG_PATH ${CMAKE_OBJDUMP})

    # Function to analyze the third party dll files linked to the exe's
    # Uses the repurposed variable from above to tell the function where
    # the dll files are located. Ignores dll's that come with Windows.
    file(GLOB exes "${CMAKE_INSTALL_PREFIX}/freeciv21-*.exe")
    file(GET_RUNTIME_DEPENDENCIES
      RESOLVED_DEPENDENCIES_VAR r_deps
      UNRESOLVED_DEPENDENCIES_VAR u_deps
      DIRECTORIES ${CLANG_PATH}
      PRE_EXCLUDE_REGEXES "^api-ms-*"
      POST_EXCLUDE_REGEXES "C:[\\\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\\\/].*"
      EXECUTABLES ${exes}
      )
      message(STATUS "Installing library dependencies for freeciv21 executables...")
      file(INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX} MESSAGE_LAZY FILES ${r_deps})
    ]] COMPONENT freeciv21)

  # Qt6 Plugins and required DLLs
  # Before installation, run a Qt6 command that copies each of the Qt
  # runtime files to the appropriate directory for installation
  install(CODE [[
    message(STATUS "Collecting Qt dependencies for freeciv21 GUI executables...")
    string(REGEX REPLACE "llvm-objdump.exe" "" CLANG_PATH ${CMAKE_OBJDUMP})

    # Run Qt's windeployqt6.exe to find the required DLLs for the GUI apps.
    # It also copies the files we need to where we need them.
    message(STATUS "Installing Qt library dependencies for freeciv21 GUI executables...")
    execute_process(
      COMMAND ${CLANG_PATH}/windeployqt6.exe --no-translations --verbose 0 --libdir ${CMAKE_INSTALL_PREFIX} 
    --plugindir ${CMAKE_INSTALL_PREFIX} --list mapping ${CMAKE_INSTALL_PREFIX}
    )
    ]] COMPONENT freeciv21)

elseif(WIN32)
  # The Visual Studio generator places all files and associated DLL libraries
  #  into a build directory. So we just grab those for install.
  install(
    DIRECTORY ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin/
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.dll PATTERN *.pdb)

  # Install the Qt framework DLL's'
  install(
    DIRECTORY ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/plugins/
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.dll)

  # Grab some files that get missed
  install(
    FILES
    ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/SDL2.dll
    ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/SDL2_mixer.dll
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT freeciv21)
endif()

# Unix/Linux specific install steps
if(UNIX AND NOT APPLE)

  # Get the current day in year-month-day format
  string(TIMESTAMP currentDay "%Y-%m-%d")

  # Fixes a bug of some sort on Linux where this gets set to /usr/local, but installs to /usr
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "/usr"
         CACHE PATH "default install path" FORCE)
    message(STATUS "CMAKE_INSTALL_PREFIX changed from the default to /usr.")
  endif()

  # Install MetaInfo and Desktop files for the applications asked for at configure
  if(FREECIV_ENABLE_CLIENT)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.desktop.in
                   net.longturn.freeciv21.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.metainfo.xml.in
                   net.longturn.freeciv21.metainfo.xml
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT freeciv21
    )
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT freeciv21
    )

    if(FREECIV_ENABLE_MANPAGES)
      install(
        FILES
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-client.6
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/man/man6
        COMPONENT freeciv21
      )
    endif()
  endif(FREECIV_ENABLE_CLIENT)

  if(FREECIV_ENABLE_SERVER)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.server.desktop.in
                   net.longturn.freeciv21.server.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.server.metainfo.xml.in
                   net.longturn.freeciv21.server.metainfo.xml
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.server.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT freeciv21
    )
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.server.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT freeciv21
    )

    if(FREECIV_ENABLE_MANPAGES)
      install(
        FILES
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-server.6
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-game-manual.6
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-manual.6
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-ruleup.6
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/man/man6
        COMPONENT freeciv21
      )
    endif()
  endif(FREECIV_ENABLE_SERVER)

  if(FREECIV_ENABLE_FCMP_QT)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.modpack.desktop.in
                   net.longturn.freeciv21.modpack.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.modpack.metainfo.xml.in
                   net.longturn.freeciv21.modpack.metainfo.xml
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.modpack.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT freeciv21
    )
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.modpack.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
      COMPONENT freeciv21
    )

    if(FREECIV_ENABLE_MANPAGES)
      install(
        FILES
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-modpack-qt.6
        ${CMAKE_BINARY_DIR}/docs/man/freeciv21-modpack.6
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/man/man6
        COMPONENT freeciv21
      )
    endif()
  endif(FREECIV_ENABLE_FCMP_QT)

 if(FREECIV_ENABLE_RULEDIT)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.ruledit.desktop.in
                   net.longturn.freeciv21.ruledit.desktop
                   @ONLY NEWLINE_STYLE UNIX)
    configure_file(${CMAKE_SOURCE_DIR}/dist/net.longturn.freeciv21.ruledit.metainfo.xml.in
                   net.longturn.freeciv21.ruledit.metainfo.xml
                   @ONLY NEWLINE_STYLE UNIX)
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.ruledit.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
      COMPONENT tool_ruledit
    )
    install(
      FILES
      ${CMAKE_BINARY_DIR}/net.longturn.freeciv21.ruledit.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
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

  if(MSYS OR WIN32)
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
  endif(MSYS OR WIN32)
endif()

if(APPLE)
  # Use Auto Revision variables to convert some templates to real files at build
  # time. Avoid overwriting if the version didn't change.
  configure_file("dist/Info.plist.in" dist/Info.plist.new
                @ONLY NEWLINE_STYLE UNIX)
  file(COPY_FILE "${CMAKE_BINARY_DIR}/dist/Info.plist.new"
                "${CMAKE_BINARY_DIR}/dist/Info.plist"
      ONLY_IF_DIFFERENT)

   install(CODE [[
    message(STATUS "Creating Freeciv21 App Bundle for macOS...")
    # Check to see if the app bundle was already created in a previous run
    if(EXISTS "${CMAKE_BINARY_DIR}/Freeciv21.app")
      message(STATUS "App Bundle exists, removing...")
      file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/Freeciv21.app")
      file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/client.iconset")
      file(REMOVE "${CMAKE_BINARY_DIR}/client.icns")
      file(GLOB DMG_FILES "${CMAKE_BINARY_DIR}/Freeciv21*.dmg*")
      file(REMOVE ${DMG_FILES})
    endif()
    # Create app bundle
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Freeciv21.app")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/MacOS")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources/doc")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources/locale")
    file(COPY_FILE "${CMAKE_BINARY_DIR}/dist/Info.plist"
                   "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Info.plist")
    file(COPY "${CMAKE_BINARY_DIR}/install/bin/"
      DESTINATION "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/MacOS/"
      FILES_MATCHING PATTERN "Freeciv21-*"
    )
    file(COPY "${CMAKE_BINARY_DIR}/install/share/freeciv21/"
      DESTINATION "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources/"
      FILES_MATCHING PATTERN "*"
    )
    file(COPY "${CMAKE_BINARY_DIR}/install/share/doc/freeciv21/"
      DESTINATION "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources/doc/"
      FILES_MATCHING PATTERN "*"
    )
    file(COPY "${CMAKE_BINARY_DIR}/install/share/locale/"
      DESTINATION "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources/locale/"
      FILES_MATCHING PATTERN "*"
    )
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/client.iconset")
    file(COPY_FILE "${CMAKE_SOURCE_DIR}/data/icons/16x16/freeciv21-client.png"
                   "${CMAKE_BINARY_DIR}/client.iconset/icon_16x16.png")
    file(COPY_FILE "${CMAKE_SOURCE_DIR}/data/icons/32x32/freeciv21-client.png"
                   "${CMAKE_BINARY_DIR}/client.iconset/icon_16x16@2x.png")
    file(COPY_FILE "${CMAKE_SOURCE_DIR}/data/icons/32x32/freeciv21-client.png"
                   "${CMAKE_BINARY_DIR}/client.iconset/icon_32x32.png")
    file(COPY_FILE "${CMAKE_SOURCE_DIR}/data/icons/64x64/freeciv21-client.png"
                   "${CMAKE_BINARY_DIR}/client.iconset/icon_32x32@2x.png")
    file(COPY_FILE "${CMAKE_SOURCE_DIR}/data/icons/128x128/freeciv21-client.png"
                   "${CMAKE_BINARY_DIR}/client.iconset/icon_128x128.png")
    execute_process(
      COMMAND iconutil --convert icns "${CMAKE_BINARY_DIR}/client.iconset")
    file(COPY_FILE "${CMAKE_BINARY_DIR}/client.icns"
                   "${CMAKE_BINARY_DIR}/Freeciv21.app/Contents/Resources/client.icns")
    message(STATUS "Freeciv21 App Bundle successfully created. Open from '${CMAKE_BINARY_DIR}' to play.")
    ]] COMPONENT freeciv21)
endif(APPLE)
