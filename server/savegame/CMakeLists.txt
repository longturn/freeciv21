add_library(savegame INTERFACE)
target_sources(
  savegame
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/savecompat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/savegame2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/savegame3.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/savemain.cpp
)

target_include_directories(savegame INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
