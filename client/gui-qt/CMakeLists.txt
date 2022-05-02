set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
include_directories( ${CMAKE_BINARY_DIR} )
add_library(
  gui-qt
  STATIC
  canvas.cpp
  canvas.h
  chatline.cpp
  chatline.h
  citydlg.cpp
  citydlg.h
  cityrep.cpp
  cityrep.h
  civstatus.cpp
  civstatus.h
  colors.cpp
  colors.h
  connectdlg.cpp
  connectdlg.h
  dialogs.cpp
  dialogs.h
  diplodlg.cpp
  diplodlg.h
  economyreport.cpp
  economyreport.h
  endgamereport.cpp
  endgamereport.h
  fc_client.cpp
  fc_client.h
  fonts.cpp
  fonts.h
  gotodlg.cpp
  gotodlg.h
  gui_main.cpp
  gui_main.h
  helpdlg.cpp
  helpdlg.h
  hudwidget.cpp
  hudwidget.h
  icons.cpp
  icons.h
  idlecallback.cpp
  idlecallback.h
  listener.h
  luaconsole.cpp
  luaconsole.h
  mapctrl.cpp
  mapctrl.h
  mapview.cpp
  mapview.h
  menu.cpp
  menu.h
  messageoptions.cpp
  messageoptions.h
  messagewin.cpp
  messagewin.h
  minimap.cpp
  minimap.h
  notifyreport.cpp
  notifyreport.h
  optiondlg.cpp
  optiondlg.h
  page_game.cpp
  page_game.h
  page_load.cpp
  page_load.h
  page_main.cpp
  page_main.h
  page_network.cpp
  page_network.h
  page_pregame.cpp
  page_pregame.h
  page_scenario.cpp
  page_scenario.h
  plrdlg.cpp
  plrdlg.h
  pregameoptions.cpp
  pregameoptions.h
  qtg_cxxside.h
  ratesdlg.cpp
  ratesdlg.h
  sciencedlg.cpp
  sciencedlg.h
  shortcuts.cpp
  shortcuts.h
  spaceshipdlg.cpp
  spaceshipdlg.h
  sprite.cpp
  sprite.h
  themes.cpp
  tileset_debugger.cpp
  tileset_debugger.h
  tooltips.cpp
  tooltips.h
  top_bar.cpp
  top_bar.h
  tradecalculation.cpp
  tradecalculation.h
  unitreport.cpp
  unitreport.h
  unitselect.cpp
  unitselect.h
  voteinfo_bar.cpp
  voteinfo_bar.h
  widgetdecorations.cpp
  widgetdecorations.h
)

target_include_directories(gui-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gui-qt PRIVATE client)
target_link_libraries(gui-qt PRIVATE Qt5::Widgets)
target_link_libraries(gui-qt PRIVATE Qt5::Svg)

if(APPLE)
  target_link_libraries(gui-qt PRIVATE "-framework AudioUnit" "-framework CoreAudio" "-framework ForceFeedback" "-framework AudioToolbox")
  qt5_import_plugins(gui-qt
    INCLUDE Qt5::QCocoaIntegrationPlugin Qt5::QSvgPlugin
  )
endif()
if(EMSCRIPTEN)
  qt5_import_plugins(gui-qt INCLUDE Qt5::QWasmIntegrationPlugin)
endif()
