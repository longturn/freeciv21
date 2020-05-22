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
  handchat.c
  hand_gen.c
  infrapts.c
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
