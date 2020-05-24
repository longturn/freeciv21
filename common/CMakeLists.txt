# Normally we'd add_subdirectory but it's prevented by circular deps
add_library(aicore STATIC
  aicore/aisupport.c
  aicore/path_finding.c
  aicore/pf_tools.c
  aicore/cm.c
  aicore/citymap.c
  aicore/caravan.c
  packets_gen.h
)
target_include_directories(aicore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/aicore)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/scriptcore)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_a_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_a_gen.h
  COMMAND
    tolua_exe
    -n common_a
    -o ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_a_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_a_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_common_a.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_common_a.pkg
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_z_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_z_gen.h
  COMMAND
    tolua_exe
    -n common_z
    -o ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_z_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_common_z_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_common_z.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_common_a.pkg
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_game_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_game_gen.h
  COMMAND
    tolua_exe
    -n game
    -o ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_game_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_game_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_game.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_game.pkg
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_signal_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_signal_gen.h
  COMMAND
    tolua_exe
    -n signal
    -o ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_signal_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/scriptcore/tolua_signal_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_signal.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore/tolua_signal.pkg
)
add_library(scriptcore STATIC
  scriptcore/api_common_intl.c
  scriptcore/api_common_utilities.c
  scriptcore/api_game_effects.c
  scriptcore/api_game_find.c
  scriptcore/api_game_methods.c
  scriptcore/api_game_specenum.c
  scriptcore/api_signal_base.c
  scriptcore/luascript.c
  scriptcore/luascript_func.c
  scriptcore/luascript_signal.c
  # Generated
  scriptcore/tolua_common_a_gen.c
  scriptcore/tolua_common_z_gen.c
  scriptcore/tolua_game_gen.c
  scriptcore/tolua_signal_gen.c
)
target_include_directories(scriptcore PUBLIC ${LUA_INCLUDE_DIRS})
target_link_libraries(scriptcore PUBLIC ${LUA_LIBRARY} tolua)
target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/scriptcore)
target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/scriptcore)

add_library(freeciv STATIC
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
  connection.c
  culture.c
  dataio.c
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
  packets.c
  player.c
  requirements.c
  research.c
  rgbcolor.c
  road.c
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
  packets_gen.c
)
target_include_directories(freeciv PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(freeciv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(freeciv PUBLIC civutility)
target_link_libraries(freeciv PUBLIC ${ZLIB_LIBRARIES} civutility aicore scriptcore)

# Circular deps
target_link_libraries(aicore PUBLIC freeciv)
target_link_libraries(scriptcore PUBLIC freeciv)
