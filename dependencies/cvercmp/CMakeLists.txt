add_library(
  cvercmp
  STATIC
  cvercmp.c
)

target_include_directories(cvercmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
