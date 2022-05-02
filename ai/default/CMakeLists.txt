add_library(
  ai_default
  STATIC
  aiair.cpp
  aiair.h
  aidata.cpp
  aidata.h
  aidiplomat.cpp
  aidiplomat.h
  aiferry.cpp
  aiferry.h
  aiguard.cpp
  aiguard.h
  aihand.cpp
  aihand.h
  aihunt.cpp
  aihunt.h
  ailog.cpp
  ailog.h
  aiparatrooper.cpp
  aiparatrooper.h
  aiplayer.cpp
  aiplayer.h
  aisettler.cpp
  aisettler.h
  aitech.cpp
  aitech.h
  aitools.cpp
  aitools.h
  aiunit.cpp
  aiunit.h
  daiactions.cpp
  daiactions.h
  daicity.cpp
  daicity.h
  daidiplomacy.cpp
  daidiplomacy.h
  daidomestic.cpp
  daidomestic.h
  daieffects.cpp
  daieffects.h
  daimilitary.cpp
  daimilitary.h
)

target_include_directories(ai_default PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ai_default PRIVATE ai)
target_link_libraries(ai_default PRIVATE server)
