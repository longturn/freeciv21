tolua_generate(
  INPUT tolua_common_a.pkg
  HEADER tolua_common_a_gen.h
  SOURCE tolua_common_a_gen.c
  PACKAGE_NAME common_a
)
tolua_generate(
  INPUT tolua_common_z.pkg
  HEADER tolua_common_z_gen.h
  SOURCE tolua_common_z_gen.c
  PACKAGE_NAME common_z
)
tolua_generate(
  INPUT tolua_game.pkg
  HEADER tolua_game_gen.h
  SOURCE tolua_game_gen.c
  PACKAGE_NAME game
)
tolua_generate(
  INPUT tolua_signal.pkg
  HEADER tolua_signal_gen.h
  SOURCE tolua_signal_gen.c
  PACKAGE_NAME signal
)

add_library(
  scriptcore
  STATIC
  api_common_intl.c
  api_common_utilities.c
  api_game_effects.c
  api_game_find.c
  api_game_methods.c
  api_game_specenum.c
  api_signal_base.c
  luascript.c
  luascript_func.c
  luascript_signal.c
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_common_a_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_common_a_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_common_z_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_common_z_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_game_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_game_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_signal_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_signal_gen.h
)

target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scriptcore PUBLIC common)
target_link_libraries(scriptcore PUBLIC tolua)
