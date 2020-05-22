add_library(
  scripting
  STATIC
  api_fcdb_auth.c
  api_fcdb_base.c
  api_server_base.c
  api_server_edit.c
  api_server_game_methods.c
  api_server_luadata.c
  api_server_notify.c
  script_fcdb.c
  script_server.c
  tolua_fcdb_gen.c
  tolua_server_gen.c
)
target_include_directories(scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scripting freeciv_server)