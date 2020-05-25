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
  COMMENT "Generating specenum.h"
)

# Generate freeciv_config.h
configure_file(cmake_freeciv_config.h.in freeciv_config.h)

# Generate fc_config.h
configure_file(cmake_fc_config.h.in fc_config.h)

add_library(civutility STATIC
  astring.c
  bitvector.c
  capability.c
  deprecations.c
  distribute.c
  fc_cmdline.c
  fc_utf8.c
  fcbacktrace.c
  fciconv.c
  fcintl.c
  fcthread.c
  genhash.c
  genlist.c
  inputfile.c
  ioz.c
  iterator.c
  log.c
  mem.c
  netintf.c
  rand.c
  registry.c
  registry_ini.c
  registry_xml.c
  section_file.c
  shared.c
  string_vector.c
  support.c
  timing.c
  md5.c
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/specenum_gen.h
)

if(NOT EMSCRIPTEN)
  target_sources(civutility PRIVATE netfile.c)
  target_link_libraries(civutility PUBLIC CURL::libcurl)
endif()

target_include_directories(civutility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(civutility PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(civutility PRIVATE ${CURL_INCLUDE_DIRS} ${ICU_INCLUDE_DIRS})
target_link_libraries(civutility PUBLIC ${ICU_LIBRARIES} Threads::Threads)
target_compile_definitions(civutility PUBLIC HAVE_CONFIG_H)

if(WIN32)
  target_link_libraries(civutility INTERFACE wsock32 ws2_32)
endif()

# i18n
if(Intl_FOUND)
  target_include_directories(civutility PUBLIC ${Intl_INCLUDE_DIR})
  target_link_libraries(civutility INTERFACE ${Intl_LIBRARY})
endif()
