# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    "" # common/packets_gen.h
    "" # common/packets_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
    "" # server/hand_gen.h
    "" # server/hand_gen.c
  VERBATIM
  DEPENDS
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    ${CMAKE_SOURCE_DIR}/common/networking/packets.def
)

add_library(
  client_gen
  STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
)

target_include_directories(client_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(client_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(client_gen PUBLIC common)

if (AUDIO_SDL)
  set(FREECIV_AUDIO audio_sdl.cpp)
endif()

add_library(
  client
  STATIC
  attribute.cpp
  citydlg_common.cpp
  cityrepdata.cpp
  client_main.cpp
  chatline_common.cpp
  connectdlg_common.cpp
  climisc.cpp
  climap.cpp
  clinet.cpp
  colors_common.cpp
  control.cpp
  editor.cpp
  global_worklist.cpp
  gui_interface.cpp
  goto.cpp
  helpdata.cpp
  luaconsole_common.cpp
  mapctrl_common.cpp
  mapview_common.cpp
  messagewin_common.cpp
  music.cpp
  overview_common.cpp
  plrdlg_common.cpp
  options.cpp
  repodlgs_common.cpp
  reqtree.cpp
  servers.cpp
  text.cpp
  themes_common.cpp
  tilespec.cpp
  unitselect_common.cpp
  update_queue.cpp
  voteinfo.cpp
  zoom.cpp
  audio.cpp
  audio_none.cpp
  ${FREECIV_AUDIO}
)

target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(client PUBLIC ${SDL2_INCLUDE_DIR})
target_include_directories(client PUBLIC ${SDL2_MIXER_INCLUDE_DIR})

add_subdirectory(agents)
add_subdirectory(luascript)

target_link_libraries(client PUBLIC Qt5::Widgets)
target_link_libraries(client PUBLIC ${SDL2_MIXER_LIBRARIES} ${SDL2_LIBRARY})

target_link_libraries(client PUBLIC agents)
target_link_libraries(client PUBLIC client_gen)
target_link_libraries(client PUBLIC cvercmp)
target_link_libraries(client PUBLIC luascript)

if(FREECIV_ENABLE_CLIENT)
  add_subdirectory(gui-qt)
  add_executable(freeciv-qt packhand.cpp) # packhand.c depends on gui-qt
  target_link_libraries(freeciv-qt client )
  target_link_libraries(freeciv-qt gui-qt)
  add_dependencies(freeciv-qt freeciv_translations)
  install(TARGETS freeciv-qt)
endif()
