find_package(Qt5 5.10 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

find_package(SQLite3 REQUIRED)

add_library(
  freeciv_modpack
  STATIC
  download.c
  modinst.c
  mpcmdline.c
  mpdb.c
)
target_link_libraries(freeciv_modpack PRIVATE freeciv)
target_link_libraries(freeciv_modpack PRIVATE cvercmp)
target_link_libraries(freeciv_modpack PRIVATE SQLite::SQLite3)

add_executable(freeciv-modpack mpcli.c)
target_link_libraries(freeciv-modpack PRIVATE freeciv)
target_link_libraries(freeciv-modpack PRIVATE freeciv_modpack)

add_executable(
  freeciv-modpack-qt
  mpgui_qt_worker.cpp
  mpgui_qt.cpp
)
target_link_libraries(freeciv-modpack-qt PRIVATE freeciv)
target_link_libraries(freeciv-modpack-qt PRIVATE freeciv_modpack)
target_link_libraries(freeciv-modpack-qt PRIVATE Qt5::Widgets)
