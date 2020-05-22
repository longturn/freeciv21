add_library(
  ai_classic
  STATIC
  classicai.c
)
target_include_directories(ai_classic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ai_classic PUBLIC ai ai_default)