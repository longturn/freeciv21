set(CMAKE_AUTOMOC ON)

add_library(
  freeciv_modpack
  STATIC
  download.cpp
  modinst.cpp
  mpcmdline.cpp
  mpdb.cpp
)

target_link_libraries(freeciv_modpack PUBLIC common)
target_link_libraries(freeciv_modpack PRIVATE cvercmp)
target_link_libraries(freeciv_modpack PRIVATE SQLite::SQLite3)

if (FREECIV_ENABLE_FCMP_CLI)
  add_executable(freeciv-modpack mpcli.cpp)
  target_link_libraries(freeciv-modpack PRIVATE freeciv_modpack)
  install(TARGETS freeciv-modpack)
endif()

if (FREECIV_ENABLE_FCMP_QT)
  add_executable(
    freeciv-modpack-qt
    mpgui_qt_worker.cpp
    mpgui_qt.cpp
  )
  target_link_libraries(freeciv-modpack-qt PRIVATE freeciv_modpack)
  target_link_libraries(freeciv-modpack-qt PRIVATE Qt5::Widgets)
  install(TARGETS freeciv-modpack-qt)
endif()
