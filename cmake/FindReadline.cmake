# Find readline. Defines Readline::readline if found.

include(FindPackageHandleStandardArgs)

find_path(READLINE_INCLUDE_DIRS readline/readline.h PATH_SUFFIXES readline)
find_library(READLINE_LIBRARIES NAMES readline)

find_package_handle_standard_args(
  Readline DEFAULT_MSG READLINE_LIBRARIES READLINE_INCLUDE_DIRS)

if(Readline_FOUND AND NOT TARGET Readline::readline)
  add_library(Readline::readline UNKNOWN IMPORTED)
  set_target_properties(
    Readline::readline
    PROPERTIES
    IMPORTED_LOCATION "${READLINE_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${READLINE_INCLUDE_DIRS}"
  )
endif()
