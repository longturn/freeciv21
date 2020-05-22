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
add_library(
  freeciv_server_gen
  STATIC
  # The generated code calls these directly
  handchat.c
  infrapts.c
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/hand_gen.c
)
target_include_directories(freeciv_server_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(freeciv_server_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(freeciv_server_gen PUBLIC freeciv)

add_library(
  freeciv_server
  STATIC
  actiontools.c
  aiiface.c
  animals.c
  auth.c
  barbarian.c
  citizenshand.c
  cityhand.c
  citytools.c
  cityturn.c
  civserver.c
  commands.c
  connecthand.c
  console.c
  diplhand.c
  diplomats.c
  edithand.c
  fcdb.c
  gamehand.c
  maphand.c
  meta.c
  mood.c
  notify.c
  plrhand.c
  report.c
  rscompat.c
  rssanity.c
  ruleset.c
  sanitycheck.c
  score.c
  sernet.c
  settings.c
  spacerace.c
  srv_log.c
  stdinhand.c
  techtools.c
  unithand.c
  unittools.c
  voting.c
)
target_include_directories(freeciv_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(freeciv_server PUBLIC freeciv_server_gen)
target_link_libraries(freeciv_server PUBLIC freeciv)

add_subdirectory(advisors)
add_subdirectory(generator)
add_subdirectory(savegame)
add_subdirectory(scripting)
add_subdirectory(${CMAKE_SOURCE_DIR}/ai ${CMAKE_CURRENT_BINARY_DIR}/ai)

set_target_properties(freeciv_server PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)
target_link_libraries(freeciv_server PUBLIC advisors)
target_link_libraries(freeciv_server PUBLIC generator)
target_link_libraries(freeciv_server PUBLIC savegame)
target_link_libraries(freeciv_server PUBLIC scripting)
target_link_libraries(freeciv_server PUBLIC ai_default)
target_link_libraries(freeciv_server PUBLIC ai_classic)

add_executable(
  freeciv-server
  srv_main.c
)
target_link_libraries(freeciv-server PRIVATE freeciv_server)
if(FREECIV_HAVE_LIBREADLINE)
  target_link_libraries(freeciv-server PRIVATE ${READLINE_LIBRARY})
endif()
