add_library(
  ai_default
  STATIC
  aiair.c
  aidata.c
  aidiplomat.c
  aiferry.c
  aiguard.c
  aihand.c
  aihunt.c
  ailog.c
  aiparatrooper.c
  aiplayer.c
  aisettler.c
  aitech.c
  aitools.c
  aiunit.c
  daiactions.c
  daicity.c
  daidiplomacy.c
  daidomestic.c
  daieffects.c
  daimilitary.c
)
target_include_directories(ai_default PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ai_default PUBLIC advisors generator)