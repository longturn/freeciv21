add_library(
  advisors
  STATIC
  advbuilding.c
  advchoice.c
  advcity.c
  advdata.c
  advgoto.c
  advruleset.c
  advspace.c
  advtools.c
  autoexplorer.c
  autosettlers.c
  infracache.c
)
target_include_directories(advisors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(advisors PUBLIC freeciv_server)