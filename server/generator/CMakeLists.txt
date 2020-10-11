add_library(
  generator
  STATIC
  fracture_map.cpp
  height_map.cpp
  mapgen.cpp
  mapgen_topology.cpp
  mapgen_utils.cpp
  startpos.cpp
  temperature_map.cpp
)

target_include_directories(generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(generator PUBLIC server)
