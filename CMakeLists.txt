cmake_minimum_required(VERSION 3.10...3.17 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(FreecivBuildOptions)
include(FreecivDependencies)

project(freeciv21 VERSION 3.1.0 LANGUAGES C CXX)

# Include subdirectories with the actual project definitions
add_subdirectory(utility)
add_subdirectory(common)

if(FREECIV_ENABLE_CLIENT)
  add_subdirectory(client)
endif()
if(FREECIV_ENABLE_SERVER)
  add_subdirectory(server)
endif()
