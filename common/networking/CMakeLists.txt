add_library(
  networking
  STATIC
  connection.c
  dataio_json.c
  dataio_raw.c
  packets.c
  packets_json.c
)

target_include_directories(networking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(networking PUBLIC common)
target_link_libraries(networking PUBLIC ZLIB::ZLIB)
