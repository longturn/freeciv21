add_library(
  agents
  STATIC
  agents.cpp
  cma_core.cpp
  cma_fec.cpp
  sha.cpp
)

target_include_directories(agents PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(agents PUBLIC client)
