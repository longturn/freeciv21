# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
  COMMAND
    ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_packets.py
    ${CMAKE_CURRENT_SOURCE_DIR}/networking/packets.def
    --mode common
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
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
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/packets_gen.cpp
)

target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/aicore)
target_include_directories(common_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/networking)

target_link_libraries(common_gen PUBLIC Qt6::Core)
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
  fc_interface.cpp
  featured_text.cpp
  game.cpp
  government.cpp
  helpdata.cpp
  idex.cpp
  improvement.cpp
  map.cpp
  mapimg.cpp
  metaknowledge.cpp
  movement.cpp
  multipliers.cpp
  nation.cpp
  path.cpp
  path_finder.cpp
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
  unit_utils.cpp
  unitlist.cpp
  unittype.cpp
  victory.cpp
  vision.cpp
  workertask.cpp
  worklist.cpp
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(aicore)
add_subdirectory(networking)
add_subdirectory(scriptcore)

# Qt6::Gui for map images -- could be made optional
target_link_libraries(common PUBLIC Qt6::Core Qt6::Gui Qt6::Network)
target_link_libraries(common PRIVATE aicore networking)
target_link_libraries(common PUBLIC common_gen scriptcore)
