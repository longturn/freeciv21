include(CMakeDependentOption)

option(FREECIV_ENABLE_CLIENT "Build the client" ON)
option(FREECIV_ENABLE_SERVER "Build the server" ON)

option(FREECIV_ENABLE_TOOLS "Build tools" ON)
cmake_dependent_option(FREECIV_ENABLE_RULEDIT "Build the ruleset editor" ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(FREECIV_ENABLE_CIVMANUAL "Build the manual extractor" ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(FREECIV_ENABLE_FCMP "Build the modpack installer" ON FREECIV_ENABLE_TOOLS OFF)
cmake_dependent_option(FREECIV_ENABLE_RULEUP "Build the ruleset updater" ON FREECIV_ENABLE_TOOLS OFF)

option(FREECIV_ENABLE_NLS "Enable internationalization" ON)

set(FREECIV_BUG_URL "https://github.com/longturn/freeciv21/issues"
    CACHE STRING "Where to file bug reports")
mark_as_advanced(FREECIV_BUG_URL)

set(FREECIV_STORAGE_DIR "~/.freeciv"
    CACHE STRING "Location for freeciv to store its information")
mark_as_advanced(FREECIV_STORAGE_DIR)
