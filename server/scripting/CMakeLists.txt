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
  api_fcdb_auth.cpp
  api_fcdb_base.cpp
  api_server_base.cpp
  api_server_edit.cpp
  api_server_game_methods.cpp
  api_server_luadata.cpp
  api_server_notify.cpp
  script_fcdb.cpp
  script_server.cpp
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
