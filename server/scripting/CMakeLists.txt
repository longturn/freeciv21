add_library(
  scripting
  STATIC
  api_server_base.cpp
  api_server_edit.cpp
  api_server_game_methods.cpp
  api_server_luadata.cpp
  api_server_notify.cpp
  script_fcdb.cpp
  script_server.cpp
)

target_include_directories(scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(scripting PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scripting PUBLIC server)
target_link_libraries(scripting PUBLIC  ${LUA_LIBRARIES})
