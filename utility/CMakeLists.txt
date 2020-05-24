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
# The first one is intended for public usage but we don't support shared
# libfreeciv
configure_file(cmake_freeciv_config.h.in freeciv_config.h)
configure_file(cmake_fc_config.h.in fc_config.h)
