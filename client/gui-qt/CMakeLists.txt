set(CMAKE_AUTOMOC ON)

add_library(
  gui-qt
  STATIC
  canvas.cpp
  chatline.cpp
  citydlg.cpp
  cityrep.cpp
  colors.cpp
  connectdlg.cpp
  dialogs.cpp
  diplodlg.cpp
  economyreport.cpp
  fc_client.cpp
  finddlg.cpp
  fonts.cpp
  gotodlg.cpp
  graphics.cpp
  gui_main.cpp
  helpdlg.cpp
  hudwidget.cpp
  infradlg.cpp
  inteldlg.cpp
  luaconsole.cpp
  mapctrl.cpp
  mapview.cpp
  menu.cpp
  messagedlg.cpp
  messagewin.cpp
  optiondlg.cpp
  pages.cpp
  plrdlg.cpp
  qtg_cxxside.cpp
  ratesdlg.cpp
  repodlgs.cpp
  shortcuts.cpp
  sidebar.cpp
  sciencedlg.cpp
  spaceshipdlg.cpp
  sprite.cpp
  themes.cpp
  unitreport.cpp
  voteinfo_bar.cpp
  wldlg.cpp
)

target_include_directories(gui-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gui-qt PRIVATE client)
target_link_libraries(gui-qt PRIVATE Qt5::Widgets)
