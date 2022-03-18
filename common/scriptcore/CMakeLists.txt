add_library(
  scriptcore STATIC
  api_common_intl.cpp
  api_common_utilities.cpp
  api_game_effects.cpp
  api_game_find.cpp
  api_game_methods.cpp
  api_game_specenum.cpp
  api_signal_base.cpp
  luascript.cpp
  luascript_signal.cpp
)

set_target_properties(scriptcore PROPERTIES AUTORCC ON)
target_sources(scriptcore PRIVATE scriptcore.qrc)

if(WIN32)
  target_compile_options(scriptcore PRIVATE -Wa,-mbig-obj)
endif()

target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(scriptcore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(scriptcore PUBLIC common)
target_link_libraries(scriptcore PUBLIC sol2 ${LUA_LIBRARIES})
target_include_directories(scriptcore PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(scriptcore PRIVATE Qt5::Core)
