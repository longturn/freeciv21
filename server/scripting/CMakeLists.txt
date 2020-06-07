tolua_generate(
  INPUT tolua_fcdb.pkg
  HEADER tolua_fcdb_gen.h
  SOURCE tolua_fcdb_gen.c
  PACKAGE_NAME fcdb
)
tolua_generate(
  INPUT tolua_server.pkg
  HEADER tolua_server_gen.h
  SOURCE tolua_server_gen.c
  PACKAGE_NAME server
)

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
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_fcdb_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_fcdb_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.h
)

target_include_directories(scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(scripting PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scripting PUBLIC server)
target_link_libraries(scripting PUBLIC tolua)
