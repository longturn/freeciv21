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
  # The generated code calls these directly
  capstr.cpp
  capstr.h
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
)

target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/aicore)
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/networking)

target_link_libraries(common_gen PUBLIC Qt5::Core)
target_link_libraries(common_gen PUBLIC utility)

# Main library
add_library(
  common
  STATIC
  achievements.cpp
  achievements.h
  actions.cpp
  actions.h
  ai.cpp
  ai.h
  base.cpp
  base.h
  borders.cpp
  borders.h
  calendar.cpp
  calendar.h
  chat.h
  citizens.cpp
  citizens.h
  city.cpp
  city.h
  clientutils.cpp
  clientutils.h
  combat.cpp
  combat.h
  culture.cpp
  culture.h
  diptreaty.cpp
  diptreaty.h
  disaster.cpp
  disaster.h
  effects.cpp
  effects.h
  events.cpp
  events.h
  explanation.h
  extras.cpp
  extras.h
  fc_interface.cpp
  fc_interface.h
  fc_types.h
  featured_text.cpp
  featured_text.h
  game.cpp
  game.h
  government.cpp
  government.h
  helpdata.cpp
  helpdata.h
  idex.cpp
  idex.h
  improvement.cpp
  improvement.h
  map.cpp
  map.h
  mapimg.cpp
  mapimg.h
  map_types.h
  metaknowledge.cpp
  metaknowledge.h
  movement.cpp
  movement.h
  multipliers.cpp
  multipliers.h
  name_translation.h
  nation.cpp
  nation.h
  path.cpp
  path.h
  path_finder.cpp
  path_finder.h
  player.cpp
  player.h
  reqtext.cpp
  reqtext.h
  requirements.cpp
  requirements.h
  research.cpp
  research.h
  rgbcolor.cpp
  rgbcolor.h
  road.cpp
  road.h
  server_settings.cpp
  server_settings.h
  spaceship.cpp
  spaceship.h
  specialist.cpp
  specialist.h
  style.cpp
  style.h
  team.cpp
  team.h
  tech.cpp
  tech.h
  terrain.cpp
  terrain.h
  tile.cpp
  tile.h
  traderoutes.cpp
  traderoutes.h
  traits.h
  unit.cpp
  unit.h
  unit_utils.cpp
  unit_utils.h
  unitlist.cpp
  unitlist.h
  unittype.cpp
  unittype.h
  version.cpp
  version.h
  victory.cpp
  victory.h
  vision.cpp
  vision.h
  workertask.cpp
  workertask.h
  worklist.cpp
  worklist.h
  world_object.h
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(aicore)
add_subdirectory(networking)
add_subdirectory(scriptcore)

target_link_libraries(common PUBLIC Qt5::Core Qt5::Network)
target_link_libraries(common PRIVATE aicore networking)
target_link_libraries(common PUBLIC common_gen scriptcore)