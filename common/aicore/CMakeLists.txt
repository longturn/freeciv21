add_library(
  aicore
  STATIC
  aiactions.c
  aisupport.c
  path_finding.c
  pf_tools.c
  cm.c
  citymap.c
  caravan.c
)

target_include_directories(aicore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(aicore PUBLIC common)
