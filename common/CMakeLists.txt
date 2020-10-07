# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_packets.py
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
    "" # client/packhand_gen.h
    "" # client/packhand_gen.cpp
    "" # server/hand_gen.h
    "" # server/hand_gen.cpp
  VERBATIM
  DEPENDS
    generate_packets.py
    networking/packets.def
)

# Generated file to avoid circular dependencies
add_library(
  common_gen
  STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
)

set_property(
  SOURCE ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
  APPEND PROPERTY COMPILE_FLAGS -fpermissive)

target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/aicore)
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/networking)

target_link_libraries(common_gen PUBLIC utility)

# Main library
add_library(
  common
  STATIC
  achievements.cpp
  actions.cpp
  ai.cpp
  base.cpp
  borders.cpp
  calendar.cpp
  capstr.cpp
  citizens.cpp
  city.cpp
  clientutils.cpp
  combat.cpp
  culture.cpp
  diptreaty.cpp
  disaster.cpp
  effects.cpp
  events.cpp
  extras.cpp
  featured_text.cpp
  fc_cmdhelp.cpp
  fc_interface.cpp
  game.cpp
  government.cpp
  idex.cpp
  improvement.cpp
  map.cpp
  mapimg.cpp
  metaknowledge.cpp
  movement.cpp
  multipliers.cpp
  nation.cpp
  player.cpp
  reqtext.cpp
  requirements.cpp
  research.cpp
  rgbcolor.cpp
  road.cpp
  server_settings.cpp
  spaceship.cpp
  specialist.cpp
  style.cpp
  team.cpp
  tech.cpp
  terrain.cpp
  tile.cpp
  traderoutes.cpp
  unit.cpp
  unitlist.cpp
  unittype.cpp
  version.cpp
  victory.cpp
  vision.cpp
  workertask.cpp
  worklist.cpp
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(
  SOURCE
  achievements.cpp
  actions.cpp
  base.cpp
  citizens.cpp
  city.cpp
  clientutils.cpp
  diptreaty.cpp
  effects.cpp
  events.cpp
  featured_text.cpp
  fc_cmdhelp.cpp
  game.cpp
  government.cpp
  improvement.cpp
  map.cpp
  mapimg.cpp
  nation.cpp
  player.cpp
  reqtext.cpp
  requirements.cpp
  research.cpp
  rgbcolor.cpp
  road.cpp
  specialist.cpp
  style.cpp
  team.cpp
  tech.cpp
  terrain.cpp
  tile.cpp
  traderoutes.cpp
  unit.cpp
  unittype.cpp
  vision.cpp
  APPEND PROPERTY COMPILE_FLAGS -fpermissive)

add_subdirectory(aicore)
add_subdirectory(networking)
add_subdirectory(scriptcore)

target_link_libraries(common PRIVATE aicore networking)
target_link_libraries(common PUBLIC common_gen scriptcore)
