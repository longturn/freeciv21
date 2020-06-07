add_library(savegame INTERFACE)
target_sources(
  savegame
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/savecompat.c
  ${CMAKE_CURRENT_SOURCE_DIR}/savegame2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/savegame3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/savemain.c
)

target_include_directories(savegame INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
