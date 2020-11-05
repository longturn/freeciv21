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

option(FREECIV_ENABLE_NLS "Enable internationalization" OFF)

set(FREECIV_BUG_URL "https://github.com/longturn/freeciv21/issues"
    CACHE STRING "Where to file bug reports")
mark_as_advanced(FREECIV_BUG_URL)

set(FREECIV_META_URL "http://meta.freeciv.org/metaserver.php"
    CACHE STRING "Metaserver URL")
mark_as_advanced(FREECIV_META_URL)

set(FREECIV_STORAGE_DIR "~/.freeciv"
    CACHE STRING "Location for freeciv to store its information")
mark_as_advanced(FREECIV_STORAGE_DIR)

set(FREECIV_AI_MOD_LAST 2 CACHE STRING "The number of AI modules to build")
mark_as_advanced(FREECIV_AI_MOD_LAST)

# Do we need the client libraries?
if (FREECIV_ENABLE_CLIENT OR FREECIV_ENABLE_CIVMANUAL)
  set(FREECIV_BUILD_LIBCLIENT TRUE)
endif()

# Do we need the server libraries?
if (FREECIV_ENABLE_SERVER
    OR FREECIV_ENABLE_CIVMANUAL
    OR FREECIV_ENABLE_RULEDIT
    OR FREECIV_ENABLE_RULEUP)
  set(FREECIV_BUILD_LIBSERVER TRUE)
endif()
