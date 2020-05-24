# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/hand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/hand_gen.c
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    "" # common/packets_gen.h
    "" # common/packets_gen.c
    "" # client/packhand_gen.h
    "" # client/packhand_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/hand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/hand_gen.c
  VERBATIM
  DEPENDS
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    ${CMAKE_SOURCE_DIR}/common/networking/packets.def
)
