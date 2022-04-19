add_library(
  networking
  STATIC
  connection.cpp
  connection.h
  dataio_raw.cpp
  dataio_raw.h
  packets.cpp
  packets.h
)

target_include_directories(networking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(networking PUBLIC Qt5::Core)
target_link_libraries(networking PUBLIC common)
if (NOT EMSCRIPTEN)
  target_link_libraries(networking PUBLIC ZLIB::ZLIB)
endif()