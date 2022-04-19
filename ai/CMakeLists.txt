add_library(
  ai
  STATIC
  aitraits.cpp
  aitraits.h
  classic
  difficulty.cpp
  difficulty.h
  handicaps.cpp
  handicaps.h
)

target_include_directories(ai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ai PUBLIC common)

add_subdirectory(default)
add_subdirectory(classic)