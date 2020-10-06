# Generate specenum_gen.h
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/specenum_gen.h
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_specenum.py
    ${CMAKE_CURRENT_BINARY_DIR}/specenum_gen.h
  VERBATIM
  DEPENDS
    generate_specenum.py
)

# Generate version_gen.h
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/version_gen.h
  COMMAND
    ${CMAKE_SOURCE_DIR}/gen_headers/generate_version_header.sh
    ${CMAKE_CURRENT_BINARY_DIR}/version_gen.h
  VERBATIM
  DEPENDS
    ${CMAKE_SOURCE_DIR}/gen_headers/generate_version_header.sh
)

# Configuration headers
set(BUG_URL ${FREECIV_BUG_URL})
# The first one is intended for public usage but we don't support shared
# libfreeciv
configure_file(cmake_freeciv_config.h.in freeciv_config.h)
configure_file(cmake_fc_config.h.in fc_config.h)

add_library(
  utility
  STATIC
  astring.cpp
  bitvector.cpp
  bugs.cpp
  capability.cpp
  deprecations.cpp
  distribute.cpp
  fc_cmdline.cpp
  fc_dirent.cpp
  fc_utf8.cpp
  fcbacktrace.cpp
  fciconv.cpp
  fcintl.cpp
  fcthread.cpp
  genhash.cpp
  genlist.cpp
  inputfile.cpp
  ioz.cpp
  iterator.cpp
  log.cpp
  mem.cpp
  netintf.cpp
  rand.cpp
  registry.cpp
  registry_ini.cpp
  registry_xml.cpp
  section_file.cpp
  shared.cpp
  string_vector.cpp
  support.cpp
  timing.cpp
  md5.cpp
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/specenum_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/version_gen.h
)
if (HAVE_CURL)
  target_sources(utility PRIVATE netfile.cpp)
endif()

target_include_directories(utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(utility PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(utility PRIVATE ${CURL_INCLUDE_DIRS})

target_link_libraries(utility PUBLIC Threads::Threads)
target_link_libraries(utility PRIVATE ${CURL_LIBRARIES})
target_link_libraries(utility PRIVATE ICU::uc)

# config.h is required, but this definition is still required for it to be used.
target_compile_definitions(utility PUBLIC HAVE_CONFIG_H)

# Suppress warnings from specenum_gen.h
freeciv_add_flag_if_supported(
  utility -Wno-tautological-constant-out-of-range-compare)

# Make it build as C++
# The files below have stuff like casts from void* to something else that break
# in C++. Add -fpermissive so GCC is happier.
set_property(
    SOURCE
    genlist.cpp
    inputfile.cpp
    ioz.cpp
    log.cpp
    mem.cpp
    netfile.cpp
    netintf.cpp
    registry_ini.cpp
    section_file.cpp
    shared.cpp
    string_vector.cpp
    support.cpp
    APPEND PROPERTY COMPILE_FLAGS -fpermissive)

if(FREECIV_ENABLE_NLS)
  target_include_directories(utility PUBLIC ${Intl_INCLUDE_DIRS})
  target_link_libraries(utility PUBLIC ${Intl_LIBRARIES})
endif()

if (FREECIV_HAVE_ICONV)
  target_link_libraries(utility PRIVATE Iconv::Iconv)
endif()
