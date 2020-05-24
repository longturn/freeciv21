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

# Generated file to avoid circular dependencies
add_library(
  common_gen
  STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.c
)

target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/aicore)
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/networking)

target_link_libraries(common_gen PUBLIC utility)

# Main library
add_library(
  common
  STATIC
  achievements.c
  actions.c
  ai.c
  base.c
  borders.c
  calendar.c
  capstr.c
  citizens.c
  city.c
  clientutils.c
  combat.c
  culture.c
  diptreaty.c
  disaster.c
  effects.c
  events.c
  extras.c
  featured_text.c
  fc_cmdhelp.c
  fc_interface.c
  game.c
  government.c
  idex.c
  improvement.c
  map.c
  mapimg.c
  metaknowledge.c
  movement.c
  multipliers.c
  nation.c
  player.c
  reqtext.c
  requirements.c
  research.c
  rgbcolor.c
  road.c
  server_settings.c
  spaceship.c
  specialist.c
  style.c
  team.c
  tech.c
  terrain.c
  tile.c
  traderoutes.c
  unit.c
  unitlist.c
  unittype.c
  version.c
  victory.c
  vision.c
  workertask.c
  worklist.c
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(aicore)
add_subdirectory(networking)
add_subdirectory(scriptcore)

target_link_libraries(common PRIVATE aicore networking)
target_link_libraries(common PUBLIC common_gen scriptcore)
