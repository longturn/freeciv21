tolua_generate(
  INPUT tolua_common_a.pkg
  HEADER tolua_common_a_gen.h
  SOURCE tolua_common_a_gen.cpp
  PACKAGE_NAME common_a
)
tolua_generate(
  INPUT tolua_game.pkg
  HEADER tolua_game_gen.h
  SOURCE tolua_game_gen.cpp
  PACKAGE_NAME game
)
tolua_generate(
  INPUT tolua_signal.pkg
  HEADER tolua_signal_gen.h
  SOURCE tolua_signal_gen.cpp
  PACKAGE_NAME signal
)

add_library(
  scriptcore
  STATIC
  api_common_intl.cpp
  api_common_utilities.cpp
  api_game_effects.cpp
  api_game_find.cpp
  api_game_methods.cpp
  api_game_specenum.cpp
  api_signal_base.cpp
  luascript.cpp
  luascript_func.cpp
  luascript_signal.cpp
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_common_a_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_common_a_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_game_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_game_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_signal_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_signal_gen.h
)

set_target_properties(scriptcore PROPERTIES AUTORCC ON)
target_sources(scriptcore PRIVATE scriptcore.qrc)

target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scriptcore PUBLIC common)
target_link_libraries(scriptcore PUBLIC tolua)
target_link_libraries(scriptcore PUBLIC sol2)
target_link_libraries(scriptcore PRIVATE Qt5::Core)
