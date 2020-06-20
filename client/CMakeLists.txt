# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.c
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    "" # common/packets_gen.h
    "" # common/packets_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.c
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
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.c
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
)

target_include_directories(client_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(client_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(client_gen PUBLIC common)

add_library(
  client
  STATIC
  attribute.c
  citydlg_common.c
  cityrepdata.c
  client_main.c
  chatline_common.c
  connectdlg_common.c
  climisc.c
  climap.c
  clinet.c
  colors_common.c
  control.c
  editor.c
  global_worklist.c
  gui_interface.c
  goto.c
  helpdata.c
  luaconsole_common.c
  mapctrl_common.c
  mapview_common.c
  messagewin_common.c
  music.c
  overview_common.c
  plrdlg_common.c
  options.c
  repodlgs_common.c
  reqtree.c
  servers.c
  text.c
  themes_common.c
  tilespec.c
  unitselect_common.c
  update_queue.c
  voteinfo.c
  zoom.c
  audio.c
  audio_none.c
)

target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(agents)
add_subdirectory(luascript)

target_link_libraries(client PUBLIC agents)
target_link_libraries(client PUBLIC client_gen)
target_link_libraries(client PUBLIC cvercmp)
target_link_libraries(client PUBLIC luascript)

if(FREECIV_ENABLE_CLIENT)
  add_subdirectory(gui-qt)
  add_executable(freeciv-qt packhand.c) # packhand.c depends on gui-qt
  target_link_libraries(freeciv-qt client)
  target_link_libraries(freeciv-qt gui-qt)
  install(TARGETS freeciv-qt)
endif()
