add_library(
  generator
  STATIC
  fracture_map.c
  height_map.c
  mapgen.c
  mapgen_topology.c
  mapgen_utils.c
  startpos.c
  temperature_map.c
)
target_include_directories(generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(generator freeciv_server)