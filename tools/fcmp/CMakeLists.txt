set(CMAKE_AUTOMOC ON)

add_library(
  freeciv_modpack
  STATIC
  download.cpp
  download.h
  modinst.cpp
  modinst.h
  mpcmdline.cpp
  mpcmdline.h
  mpdb.cpp
  mpdb.h
)

target_link_libraries(freeciv_modpack PUBLIC common)
target_link_libraries(freeciv_modpack PRIVATE cvercmp)
target_link_libraries(freeciv_modpack PRIVATE SQLite::SQLite3)

if (FREECIV_ENABLE_FCMP_CLI)
  add_executable(freeciv21-modpack mpcli.cpp)
  target_link_libraries(freeciv21-modpack PRIVATE freeciv_modpack)
  install(TARGETS freeciv21-modpack
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT tool_fcmp_cli)
endif()

if (FREECIV_ENABLE_FCMP_QT)
  add_executable(
    freeciv21-modpack-qt ${GUI_TYPE}
    mpgui_qt_worker.cpp
    mpgui_qt_worker.h
    mpgui_qt.cpp
    mpgui_qt.h
  )
  target_link_libraries(freeciv21-modpack-qt PRIVATE freeciv_modpack)
  target_link_libraries(freeciv21-modpack-qt PRIVATE Qt5::Widgets)
  install(TARGETS freeciv21-modpack-qt
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT freeciv21)
endif()