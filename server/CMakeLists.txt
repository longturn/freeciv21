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

add_library(
  server_gen
  STATIC
  # The generated code calls these directly
  cityhand.c
  handchat.c
  infrapts.c
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/hand_gen.c
)

target_include_directories(server_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(server_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(server_gen PUBLIC common)

add_library(
  server
  STATIC
  actiontools.c
  aiiface.c
  animals.c
  auth.c
  barbarian.c
  citizenshand.c
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
  srv_main.c
  stdinhand.c
  techtools.c
  unithand.c
  unittools.c
  voting.c
)

target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Required for interface (the corresponding libraries are PRIVATE dependencies)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/advisors)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/generator)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/savegame)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/scripting)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/ai)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/ai/default)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/ai/classic)

target_link_libraries(server PUBLIC server_gen)

add_subdirectory(advisors)
add_subdirectory(generator)
add_subdirectory(savegame)
add_subdirectory(scripting)
add_subdirectory(${CMAKE_SOURCE_DIR}/ai ${CMAKE_CURRENT_BINARY_DIR}/ai)

# Keep them private because they use INTERFACE_SOURCES
target_link_libraries(server PRIVATE advisors)
target_link_libraries(server PRIVATE generator)
target_link_libraries(server PRIVATE savegame)
target_link_libraries(server PRIVATE scripting)
target_link_libraries(server PRIVATE ai)
target_link_libraries(server PRIVATE ai_classic)

if(FREECIV_HAVE_LIBREADLINE)
  target_link_libraries(server PRIVATE Readline::readline)
endif()

# Create an empty file to build the server from. srv_main.c is needed by ruledit
# so it has to be in the freeciv_server library.
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/null.c)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.c "")
endif()
add_executable(freeciv-server null.c)
target_link_libraries(freeciv-server server)
install(TARGETS freeciv-server)
