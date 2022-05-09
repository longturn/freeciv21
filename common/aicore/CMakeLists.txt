add_library(
  aicore
  STATIC
  aiactions.cpp
  aisupport.cpp
  caravan.cpp
  citymap.cpp
  cm.cpp
  path_finding.cpp
  pf_tools.cpp
)

target_include_directories(aicore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(aicore PUBLIC common)
