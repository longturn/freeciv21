add_library(
  agents
  STATIC
  agents.c
  cma_core.c
  cma_fec.c
  sha.c
)

target_include_directories(agents PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(agents PUBLIC client)
