add_library(
  savegame
  STATIC
  savecompat.c
  savegame2.c
  savegame3.c
  savemain.c
)
target_include_directories(savegame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(savegame freeciv_server)
