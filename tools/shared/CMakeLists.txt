add_library(
  tools_shared
  STATIC
  tools_fc_interface.cpp
)

target_include_directories(tools_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tools_shared PRIVATE server)
