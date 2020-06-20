# "Finds" ICU for emscripten. Emulates the required subset of
# https://cmake.org/cmake/help/latest/module/FindICU.html

if (NOT EMSCRIPTEN)
  message(FATAL_ERROR "This module only works with emscripten")
endif()

add_library(ICU::uc INTERFACE IMPORTED)
set_target_properties(
  ICU::uc
  PROPERTIES
  INTERFACE_COMPILE_OPTIONS "SHELL:-s USE_ICU=1"
  INTERFACE_LINK_OPTIONS "SHELL:-s USE_ICU=1"
)

set(ICU_FOUND TRUE)
set(ICU_uc_FOUND TRUE)
