find_package(Qt5 5.10 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

add_library(
  freeciv_client_qt
  canvas.cpp
  chatline.cpp
  citydlg.cpp
  cityrep.cpp
  colors.cpp
  connectdlg.cpp
  dialogs.cpp
  diplodlg.cpp
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
  spaceshipdlg.cpp
  sprite.cpp
  themes.cpp
  voteinfo_bar.cpp
  wldlg.cpp
)
target_include_directories(freeciv_client_qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(freeciv_client_qt PRIVATE freeciv_client cma_core luascript Qt5::Widgets)