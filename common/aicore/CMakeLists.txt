add_library(
  aicore
  STATIC
  aiactions.cpp
  aiactions.h
  aisupport.cpp
  aisupport.h
  caravan.cpp
  caravan.h
  citymap.cpp
  citymap.h
  cm.cpp
  cm.h
  path_finding.cpp
  path_finding.h
  pf_tools.cpp
  pf_tools.h
)

target_include_directories(aicore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(aicore PUBLIC common)