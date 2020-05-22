add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/tolua_fcdb_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/tolua_fcdb_gen.c
  COMMAND
    tolua_exe
    -n client
    -o ${CMAKE_CURRENT_BINARY_DIR}/tolua_fcdb_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/tolua_fcdb_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tolua_fcdb.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/tolua_fcdb.pkg
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.c
  COMMAND
    tolua_exe
    -n client
    -o ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tolua_server.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/tolua_server.pkg
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
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_server_gen.c
)
target_include_directories(scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scripting freeciv_server)