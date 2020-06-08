cmake_minimum_required(VERSION 3.10...3.17 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(FreecivBuildOptions)

project(freeciv21 VERSION 3.1.0 LANGUAGES C CXX)

# After project() because the list of languages has to be known
include(FreecivDependencies)
include(FreecivHelpers)

# Put all executables at the root of the build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Include subdirectories with the actual project definitions
add_subdirectory(utility)
add_subdirectory(common)

if (FREECIV_BUILD_LIBCLIENT)
  add_subdirectory(client)
endif()
if (FREECIV_BUILD_LIBSERVER)
  add_subdirectory(server)
endif()

# Always add tools, conditionals are inside
add_subdirectory(tools)
