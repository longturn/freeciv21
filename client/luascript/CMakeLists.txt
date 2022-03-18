add_library(
  luascript
  STATIC
  script_client.cpp
)

target_include_directories(luascript PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(luascript PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(luascript PUBLIC client)
target_link_libraries(luascript PUBLIC ${LUA_LIBRARIES})
