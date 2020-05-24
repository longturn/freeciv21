find_package(Qt5 5.10 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

add_executable(
  freeciv-ruledit
  conversion_log.cpp
  edit_utype.cpp
  effect_edit.cpp
  req_edit.cpp
  requirers_dlg.cpp
  ruledit.cpp
  ruledit_qt.cpp
  tab_building.cpp
  tab_enablers.cpp
  tab_extras.cpp
  tab_good.cpp
  tab_gov.cpp
  tab_misc.cpp
  tab_multiplier.cpp
  tab_nation.cpp
  tab_tech.cpp
  tab_terrains.cpp
  tab_unit.cpp
  univ_value.c
  validity.c
)
target_link_libraries(freeciv-ruledit PRIVATE freeciv)
target_link_libraries(freeciv-ruledit PRIVATE freeciv_server)
target_link_libraries(freeciv-ruledit PRIVATE tools_ruleutil)
target_link_libraries(freeciv-ruledit PRIVATE tools_shared)
target_link_libraries(freeciv-ruledit PRIVATE Qt5::Widgets)
