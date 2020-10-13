add_library(
  networking
  STATIC
  connection.cpp
  dataio_json.cpp
  dataio_raw.cpp
  packets.cpp
  packets_json.cpp
)

target_include_directories(networking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(networking PUBLIC Qt5::Core)
target_link_libraries(networking PUBLIC common)
target_link_libraries(networking PUBLIC ZLIB::ZLIB)
