add_library(
  ai_default
  STATIC
  aiair.cpp
  aidata.cpp
  aidiplomat.cpp
  aiferry.cpp
  aiguard.cpp
  aihand.cpp
  aihunt.cpp
  ailog.cpp
  aiparatrooper.cpp
  aiplayer.cpp
  aisettler.cpp
  aitech.cpp
  aitools.cpp
  aiunit.cpp
  daiactions.cpp
  daicity.cpp
  daidiplomacy.cpp
  daidomestic.cpp
  daieffects.cpp
  daimilitary.cpp
)

target_include_directories(ai_default PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ai_default PRIVATE ai)
target_link_libraries(ai_default PRIVATE server)
