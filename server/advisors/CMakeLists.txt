add_library(
  advisors
  STATIC
  advbuilding.cpp
  advchoice.cpp
  advcity.cpp
  advdata.cpp
  advgoto.cpp
  advruleset.cpp
  advspace.cpp
  advtools.cpp
  autoexplorer.cpp
  autosettlers.cpp
  infracache.cpp
)

target_include_directories(advisors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(advisors PUBLIC ai)
target_link_libraries(advisors PUBLIC server)
