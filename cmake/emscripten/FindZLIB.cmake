# "Finds" threads for emscripten.
# Emulates https://cmake.org/cmake/help/latest/module/FindZLIB.html

if (NOT EMSCRIPTEN)
  message(FATAL_ERROR "This module only works with emscripten")
endif()

add_library(ZLIB::ZLIB INTERFACE IMPORTED)
set_target_properties(
  ZLIB::ZLIB
  PROPERTIES
  INTERFACE_COMPILE_OPTIONS "SHELL:-s USE_ZLIB=1"
  INTERFACE_LINK_OPTIONS "SHELL:-s USE_ZLIB=1"
)

set(ZLIB_FOUND TRUE)
