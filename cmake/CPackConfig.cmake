############################
# CPack configuration file #
############################

## General Configuration for all OS's ##

set(CPACK_PACKAGE_NAME "Freeciv21")
set(CPACK_PACKAGE_VENDOR "longturn.net")
set(CPACK_PACKAGE_VERSION_MAJOR ${FREECIV21_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${FREECIV21_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${FREECIV21_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Freeciv21 - Freeciv for the 21st Century")
set(CPACK_INSTALL_CMAKE_PROJECTS ${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://longturn.net")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0.txt")
#set(CPACK_PACKAGE_CONTACT "Inkscape developers <inkscape-devel@lists.inkscape.org>")

# if(WIN32 OR MSYS OR MINGW)

  set(CPACK_GENERATOR "NSIS")
  set(CPACK_PACKAGE_CHECKSUM "SHA256")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
  #set(CPACK_PACKAGE_DESCRIPTION "A description of the project, used in places such as the introduction screen of CPack-generated Windows installers.")

  if(NOT CPACK_SYSTEM_NAME)
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  endif()

  set(CPACK_PACKAGE_DIRECTORY ${CPACK_SYSTEM_NAME})
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION})
  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION})
  
  set(CPACK_PACKAGE_EXECUTABLES "freeciv-qt;Freeciv21 Qt Client;freeciv-modpack-qt;Freeciv21 Qt Modpack Installer")

  set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)

  # this allows to override above configuration per cpack generator at CPack-time
  set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/cmake/CPack.cmake")
  configure_file("${CMAKE_SOURCE_DIR}/cmake/CPack.cmake" "${CMAKE_BINARY_DIR}/cmake/CPack.cmake" @ONLY)

  ## Generator-specific configuration ##

  # NSIS (Windows .exe installer)
  #set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.ico")
  #set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_SOURCE_DIR}/packaging/nsis/header.bmp")
  #set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}/packaging/nsis/welcomefinish.bmp")
  #set(CPACK_NSIS_INSTALLED_ICON_NAME "bin/inkscape.exe")
  #set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/share/branding/inkscape.svg")
  set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_NSIS_MENU_LINKS "${CPACK_PACKAGE_HOMEPAGE_URL}" "Longturn Homepage")
  set(CPACK_NSIS_COMPRESSOR "/SOLID lzma") # zlib|bzip2|lzma
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "OFF")
  set(CPACK_NSIS_MODIFY_PATH "ON") # while the name does not suggest it, this also provides the possibility to add desktop icons

  set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  SetCompressorDictSize 64") # hack (improve compression)
  set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  BrandingText '${CPACK_PACKAGE_DESCRIPTION_SUMMARY}'") # hack (overwrite BrandingText)
  set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  !define MUI_COMPONENTSPAGE_SMALLDESC") # hack (better components page layout)

  #file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/packaging/nsis/fileassoc.nsh" native_path)
  #string(REPLACE "\\" "\\\\" native_path "${native_path}")
  #set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "!include ${native_path}")
  #set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "\
  #  !insertmacro APP_ASSOCIATE 'svg' 'Inkscape.SVG' 'Scalable Vector Graphics' '$INSTDIR\\\\bin\\\\inkscape.exe,0' 'Open with Inkscape' '$INSTDIR\\\\bin\\\\inkscape.exe \\\"%1\\\"'\n\
  #  !insertmacro APP_ASSOCIATE 'svgz' 'Inkscape.SVGZ' 'Compressed Scalable Vector Graphics' '$INSTDIR\\\\bin\\\\inkscape.exe,0' 'Open with Inkscape' '$INSTDIR\\\\bin\\\\inkscape.exe \\\"%1\\\"'\n\
  #  !insertmacro UPDATEFILEASSOC")
  #set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "\
  #  !insertmacro APP_UNASSOCIATE 'svg' 'Inkscape.SVG'\n\
  #  !insertmacro APP_UNASSOCIATE 'svgz' 'Inkscape.SVGZ'\n\
  #  !insertmacro UPDATEFILEASSOC")

  ## Component definition
  #  - variable names are UPPER CASE, even if component names are lower case
  #  - components/groups are ordered alphabetically by component/group name; groups always come first
  #  - empty (e.g. OS-specific) components are discarded automatically

  set(CPACK_COMPONENTS_ALL freeciv21 tool-ruledit tool-fcmp-cli tool-ruleup tool-manual translations)
  set(CPACK_COMPONENT_FREECIV21_DISPLAY_NAME "All programs and supporting files to run Freeciv21. Includes Qt Client, Server and Qt Modpack Installer.")
  set(CPACK_COMPONENT_TOOL-RULEDIR_DISPLAY_NAME "Freeciv21 Ruleset Editor")
  set(CPACK_COMPONENT_TOOL-FCMP-CLI_DISPLAY_NAME "Modpack Installer Command Line Interpreter Edition")
  set(CPACK_COMPONENT_TOOL-RULEUP_DISPLAY_NAME "Freeciv21 Ruleset Upgrade Tool")
  set(CPACK_COMPONENT_TOOL-MANUAL_DISPLAY_NAME "Freeciv21 Server Manual Tool")
  set(CPACK_COMPONENT_TRANSLATIONS_DISPLAY_NAME "Supported Languages")

  set(CPACK_COMPONENT_TOOL-RULEDIR_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL-FCMP-CLI_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL-RULEDIR_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL-RULEUP_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL-MANUAL_GROUP "Tools")
  set(CPACK_COMPONENT_GROUP_TOOLS_DESCRIPTION
  "All of the tools you'll ever need to support Freeciv21 game play.")
  set(CPACK_COMPONENT_GROUP_TOOLS_EXPANDED)

  set(CPACK_ALL_INSTALL_TYPES Default Custom)
  set(CPACK_COMPONENT_FREECIV21_INSTALL_TYPES Default Custom)
  set(CPACK_COMPONENT_FREECIV21_REQUIRED)
  set(CPACK_COMPONENT_TOOL-RULEDIR_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TOOL-FCMP-CLI_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TOOL-RULEUP_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TOOL-MANUAL_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TRANSLATIONS_INSTALL_TYPES Default Custom)

  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}-${FREECIV21_VERSION}")

  # -----------------------------------------------------------------------------
  # 'dist-win-exe' - generate .exe installer (NSIS) for Windows
  # -----------------------------------------------------------------------------
  add_custom_target(dist-win-exe COMMAND ${CMAKE_CPACK_COMMAND} -G NSIS)
  
  message("-- Including CPack NSIS")
  ## load cpack module (do this *after* all the CPACK_* variables have been set)
  include(CPack)

# endif()
