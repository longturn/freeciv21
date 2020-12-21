# Compresses a single file. Expects to be called with:
#
#    cmake -P compress.cmake output input compression

file(ARCHIVE_CREATE
  OUTPUT "${CMAKE_ARGV3}"
  PATHS "${CMAKE_ARGV4}"
  FORMAT raw
  COMPRESSION ${CMAKE_ARGV5}
  COMPRESSION_LEVEL 9)
