set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_compile_definitions(NOMINMAX)

# Generate files
tolua_generate(
  INPUT luascript/tolua_client.pkg
  HEADER tolua_client_gen.h
  SOURCE tolua_client_gen.cpp
  PACKAGE_NAME client
)
add_compile_definitions(LUA_LIB)

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

add_executable(freeciv21-client ${GUI_TYPE})
target_sources(
  freeciv21-client
  PRIVATE
  attribute.cpp
  audio_none.cpp
  audio.cpp
  canvas.cpp
  chatline.cpp
  chatline_common.cpp
  citybar.cpp
  citydlg.cpp
  citydlg_common.cpp
  cityrep.cpp
  cityrepdata.cpp
  client_main.cpp
  climap.cpp
  climisc.cpp
  clinet.cpp
  colorizer.cpp
  colors_common.cpp
  connectdlg.cpp
  connectdlg_common.cpp
  control.cpp
  dialogs.cpp
  diplodlg.cpp
  economyreport.cpp
  editor.cpp
  endgamereport.cpp
  fc_client.cpp
  fonts.cpp
  global_worklist.cpp
  goto.cpp
  gotodlg.cpp
  governor.cpp
  gui_main.cpp
  helpdlg.cpp
  hudwidget.cpp
  icons.cpp
  layer_background.cpp
  layer_base_flags.cpp
  layer_darkness.cpp
  layer_special.cpp
  layer_terrain.cpp
  layer_units.cpp
  layer.cpp
  luaconsole.cpp
  luaconsole_common.cpp
  luascript/api_client_base.cpp
  luascript/script_client.cpp
  mapctrl.cpp
  mapctrl_common.cpp
  mapview.cpp
  mapview_common.cpp
  mapview_geometry.cpp
  map_updates_handler.cpp
  menu.cpp
  messageoptions.cpp
  messagewin.cpp
  messagewin_common.cpp
  minimap.cpp
  minimap_panel.cpp
  music.cpp
  notifyreport.cpp
  options.cpp
  optiondlg.cpp
  overview_common.cpp
  packhand.cpp
  page_game.cpp
  page_load.cpp
  page_main.cpp
  page_network.cpp
  page_pregame.cpp
  page_scenario.cpp
  plrdlg.cpp
  plrdlg_common.cpp
  pregameoptions.cpp
  ratesdlg.cpp
  renderer.cpp
  repodlgs_common.cpp
  reqtree.cpp
  sciencedlg.cpp
  shortcuts.cpp
  spaceshipdlg.cpp
  sprite.cpp
  text.cpp
  themes.cpp
  themes_common.cpp
  tilespec.cpp
  tileset_debugger.cpp
  tooltips.cpp
  top_bar.cpp
  tradecalculation.cpp
  turn_done_button.cpp
  unitreport.cpp
  unitselect.cpp
  update_queue.cpp
  voteinfo.cpp
  voteinfo_bar.cpp
  widgetdecorations.cpp
  unithudselector.cpp

  tileset/drawn_sprite.cpp
  utils/improvement_seller.cpp
  utils/unit_quick_menu.cpp
  widgets/city/city_icon_widget.cpp
  widgets/city/upkeep_widget.cpp

  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_client_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/tolua_client_gen.h
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
)

target_include_directories(freeciv21-client PRIVATE include)
target_include_directories(freeciv21-client PRIVATE luascript)
target_include_directories(freeciv21-client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(freeciv21-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(freeciv21-client PRIVATE common)
target_link_libraries(freeciv21-client PRIVATE cvercmp)
target_link_libraries(freeciv21-client PRIVATE tolua)

target_link_libraries(freeciv21-client PRIVATE Qt5::Widgets)
target_link_libraries(freeciv21-client PRIVATE Qt5::Svg)
target_link_libraries(freeciv21-client PRIVATE ${SDL2_MIXER_LIBRARIES} ${SDL2_LIBRARY})

if(NOT EMSCRIPTEN)
  target_sources(freeciv21-client PRIVATE servers.cpp)
endif()

if (AUDIO_SDL)
  target_sources(freeciv21-client PRIVATE audio_sdl.cpp)
  if (NOT EMSCRIPTEN)
    # SDL comes with the -s option on emscripten
    target_include_directories(freeciv21-client PRIVATE ${SDL2_INCLUDE_DIR})
    target_include_directories(freeciv21-client PRIVATE ${SDL2_MIXER_INCLUDE_DIR})
  endif()
endif()

if(APPLE)
  target_link_libraries(freeciv21-client PRIVATE "-framework AudioUnit" "-framework CoreAudio" "-framework ForceFeedback" "-framework AudioToolbox")
  qt5_import_plugins(freeciv21-client INCLUDE Qt5::QCocoaIntegrationPlugin Qt5::QSvgPlugin)
endif()

if(EMSCRIPTEN)
  qt5_import_plugins(freeciv21-client INCLUDE Qt5::QWasmIntegrationPlugin)
  target_link_options(freeciv21-client PRIVATE
                      -sASSERTIONS=1
                      -sWEBSOCKET_URL=wss://
                      -sTOTAL_MEMORY=52428800
                      -sALLOW_MEMORY_GROWTH=1
                      "-sEXPORTED_RUNTIME_METHODS=['UTF16ToString','stringToUTF16','callMain','printErr']"
                      --bind
                      -sUSE_SDL_MIXER=2
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/tilesets/flags@data/tilesets/flags"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/helpdata.txt@data/helpdata.txt"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/misc@data/misc"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/tilesets/buildings@data/tilesets/buildings"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/tilesets/wonders@data/tilesets/wonders"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/hexemplio.tilespec@data/hexemplio.tilespec"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/hexemplio@data/hexemplio"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/amplio2.tilespec@data/amplio2.tilespec"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/amplio2@data/amplio2"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/themes/gui-qt/Classic@data/themes/gui-qt/Classic"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/themes/gui-qt/icons@data/themes/gui-qt/icons"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/stdmusic.musicspec@data/stdmusic.musicspec"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/stdmusic@data/stdmusic"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/stdsounds@data/stdsounds"
                      "--preload-file=${CMAKE_SOURCE_DIR}/data/stdsounds.soundspec@data/stdsounds.soundspec"
                      "--exclude-file=*CMakeLists*"
                      "--exclude-file=*.icns"
                      "--exclude-file=*.serv"
                      "--exclude-file=*.svg"
  )
endif()

add_dependencies(freeciv21-client freeciv_translations)
install(TARGETS freeciv21-client
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT freeciv21)
