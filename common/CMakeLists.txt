# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.c
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_packets.py
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.c
    "" # client/packhand_gen.h
    "" # client/packhand_gen.c
    "" # server/hand_gen.h
    "" # server/hand_gen.c
  VERBATIM
  DEPENDS
    generate_packets.py
    networking/packets.def
)
