include(CMakeDependentOption)

option(FREECIV_ENABLE_CLIENT "Build the client" ON)
option(FREECIV_ENABLE_SERVER "Build the server" ON)

option(FREECIV_ENABLE_TOOLS "Build tools" ON)
cmake_dependent_option(
  FREECIV_ENABLE_RULEDIT
  "Build the ruleset editor"
  ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(
  FREECIV_ENABLE_CIVMANUAL
  "Build the manual extractor"
  ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(
  FREECIV_ENABLE_FCMP_CLI
  "Build the modpack installer (command-line interface)"
  ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(
  FREECIV_ENABLE_FCMP_QT
  "Build the modpack installer (Qt interface)"
  ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(
  FREECIV_ENABLE_RULEUP
  "Build the ruleset updater"
  ON FREECIV_ENABLE_TOOLS OFF)

option(FREECIV_ENABLE_NLS "Enable internationalization" ON)

option(FREECIV_ENABLE_WERROR "Error out on select compiler warnings" ON)

set(FREECIV_BUG_URL "https://github.com/longturn/freeciv21/issues"
    CACHE STRING "Where to file bug reports")
mark_as_advanced(FREECIV_BUG_URL)

set(FREECIV_META_URL "http://meta.freeciv.org/metaserver.php"
    CACHE STRING "Metaserver URL")
mark_as_advanced(FREECIV_META_URL)

set(FREECIV_AI_MOD_LAST 2 CACHE STRING "The number of AI modules to build")
mark_as_advanced(FREECIV_AI_MOD_LAST)

# Do we need the server libraries?
if (FREECIV_ENABLE_SERVER
    OR FREECIV_ENABLE_CIVMANUAL
    OR FREECIV_ENABLE_RULEDIT
    OR FREECIV_ENABLE_RULEUP)
  set(FREECIV_BUILD_LIBSERVER TRUE)
endif()

# If we ask for the client, we have to have the Qt modpack installer from tools
if (FREECIV_ENABLE_CLIENT)
  cmake_dependent_option(
  FREECIV_ENABLE_FCMP_QT
  "Build the modpack installer (Qt interface)"
  ON FREECIV_ENABLE_TOOLS OFF)
endif()

# By default we do not enable VCPKG
option(FREECIV_USE_VCPKG "Use VCPKG" OFF)

option(FREECIV_DOWNLOAD_FONTS "Download fonts needed by Freeciv21" ON)
mark_as_advanced(FREECIV_DOWNLOAD_FONTS)
