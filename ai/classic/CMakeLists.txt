add_library(
  ai_classic
  STATIC
  classicai.cpp
)

target_include_directories(ai_classic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ai_classic PRIVATE advisors)
target_link_libraries(ai_classic PRIVATE ai_default)
