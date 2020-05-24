add_library(
  tools_ruleutil
  STATIC
  comments.c
  rulesave.c
)
target_include_directories(tools_ruleutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tools_ruleutil PRIVATE freeciv_server)
