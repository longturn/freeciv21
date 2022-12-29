# CPack configuration file

# General Configuration for all OS's
set(CPACK_PACKAGE_NAME "Freeciv21")
set(CPACK_PACKAGE_VENDOR "longturn.net")
set(CPACK_PACKAGE_CONTACT "longturn.net")
set(CPACK_PACKAGE_VERSION_MAJOR ${FC21_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${FC21_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${FC21_PATCH_VERSION})
set(CPACK_PACKAGE_VERSION ${FC21_REV_TAG})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Freeciv21 - Freeciv for the 21st Century")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://longturn.net")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://longturn.net")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_CHECKSUM "SHA256")

if(WIN32 OR MSYS OR MINGW)

  # Use the NSIS Package for Windows
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_BINARY_NSIS "ON")

  # Establish some variables to place the package where we want it
  if(NOT CPACK_SYSTEM_NAME)
    if("$ENV{MSYSTEM}" STREQUAL "MINGW32")
      set(CPACK_CPU_ARCH "i686")
    else()
      set(CPACK_CPU_ARCH $ENV{MSYSTEM_CARCH})
    endif()
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CPACK_CPU_ARCH}")
  endif()

  set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/${CPACK_SYSTEM_NAME}")
  set(CPACK_OUTPUT_FILE_PREFIX ${CPACK_PACKAGE_DIRECTORY})
  # The name of the package exe file
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${FC21_REV_TAG}-${CPACK_SYSTEM_NAME}")

  ## Component definition
  #  - variable names are UPPER CASE, even if component names are lower case
  #  - components/groups are ordered alphabetically by component/group name; groups always come first
  #  - empty (e.g. OS-specific) components are discarded automatically

  # Define the components and how they are organized in the install package
  set(CPACK_COMPONENTS_ALL freeciv21 tool_ruledit tool_fcmp_cli tool_ruleup tool_manual translations)
  set(CPACK_COMPONENT_FREECIV21_INSTALL_TYPES Default Custom)
  set(CPACK_COMPONENT_FREECIV21_REQUIRED)
  set(CPACK_COMPONENT_TOOL_RULEDIT_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TOOL_FCMP_CLI_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TOOL_RULEUP_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TOOL_MANUAL_INSTALL_TYPES Custom)
  set(CPACK_COMPONENT_TRANSLATIONS_INSTALL_TYPES Default Custom)

  set(CPACK_COMPONENT_TOOL_FCMP_CLI_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL_RULEDIT_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL_RULEUP_GROUP "Tools")
  set(CPACK_COMPONENT_TOOL_MANUAL_GROUP "Tools")
  set(CPACK_COMPONENT_GROUP_TOOLS_DESCRIPTION
    "All of the tools you'll ever need to support Freeciv21 game play.")
  set(CPACK_COMPONENT_GROUP_TOOLS_EXPANDED)

  # Define the names of the components and how they are displayed
  set(CPACK_COMPONENT_FREECIV21_DISPLAY_NAME "Freeciv21")
  set(CPACK_COMPONENT_TOOL_RULEDIT_DISPLAY_NAME "Ruleset Editor")
  set(CPACK_COMPONENT_TOOL_FCMP_CLI_DISPLAY_NAME "Modpack Installer CLI Edition")
  set(CPACK_COMPONENT_TOOL_RULEUP_DISPLAY_NAME "Ruleset Upgrade Tool")
  set(CPACK_COMPONENT_TOOL_MANUAL_DISPLAY_NAME "Server Manual Tool")
  set(CPACK_COMPONENT_TRANSLATIONS_DISPLAY_NAME "Languages")

  ## Generator-specific configuration ##

  # NSIS (Windows .exe installer)
  set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/dist/client.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/dist/client.ico")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_SOURCE_DIR}/dist/client.ico")
  set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
  set(CPACK_NSIS_MENU_LINKS "${CPACK_PACKAGE_HOMEPAGE_URL}" "Longturn Homepage")
  set(CPACK_NSIS_MODIFY_PATH "OFF")
  set(CPACK_NSIS_CONTACT "longturn.net@gmail.com")

  set(CPACK_NSIS_COMPRESSOR "/SOLID lzma") # zlib|bzip2|lzma
  set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  SetCompressorDictSize 64") # hack (improve compression)
  set(CPACK_NSIS_COMPRESSOR "${CPACK_NSIS_COMPRESSOR}\n  BrandingText '${CPACK_PACKAGE_DESCRIPTION_SUMMARY}'") # hack (overwrite BrandingText)

endif()

# Unix/Linux specific packages
if(UNIX AND NOT APPLE)

  # Establish some variables to place the package where we want it
  if(NOT CPACK_SYSTEM_NAME)
    execute_process(COMMAND "uname"
                    OUTPUT_VARIABLE CPACK_SYSTEM_NAME
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(CPACK_CPU_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CPACK_CPU_ARCH}")
  endif()

  set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/${CPACK_SYSTEM_NAME}")
  set(CPACK_OUTPUT_FILE_PREFIX ${CPACK_PACKAGE_DIRECTORY})
  # The name of the package file
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${FC21_REV_TAG}-${CPACK_SYSTEM_NAME}")

  # Debian "deb" file generator settings
  set(CPACK_GENERATOR "DEB")
  set(CPACK_BINARY_DEB "ON")
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_PACKAGE_SECTION "Games")
  set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
  set(CPACK_DEB_COMPONENT_INSTALL "ON")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "fonts-linuxlibertine (>= 5.3.0)")

endif()

# Apple macOS
if(APPLE)

  set(CPACK_BINARY_BUNDLE "ON")
  set(CPACK_BINARY_DRAGNDROP "ON")
  set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CPACK_OUTPUT_FILE_PREFIX ${CPACK_PACKAGE_DIRECTORY})
  set(CPACK_CPU_ARCH ${CMAKE_SYSTEM_PROCESSOR})
  set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CPACK_CPU_ARCH}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${FC21_REV_TAG}-${CPACK_SYSTEM_NAME}")
  set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
  set(CPACK_BUNDLE_NAME "Freeciv21 Installer")
  set(CPACK_BUNDLE_PLIST "${CPACK_SOURCE_DIR}/dist/Info.plist")
  set(CPACK_BUNDLE_ICON "${CPACK_SOURCE_DIR}/dist/client.ico")
  set(CPACK_DMG_VOLUME_NAME "Freeciv21 Installer")

endif()

message(STATUS "Including CPack")
include(CPack)
