set(CMAKE_AUTOMOC ON)

add_executable(
  freeciv21-ruledit
  conversion_log.cpp
  req_edit.cpp
  req_vec_fix.cpp
  requirers_dlg.cpp
  ruledit.cpp
  tab_enablers.cpp
  tab_gov.cpp
  tab_nation.cpp
  tab_unit.cpp
  edit_utype.cpp
  requirers_dlg.cpp
  ruledit_qt.cpp
  tab_extras.cpp
  tab_misc.cpp
  tab_tech.cpp
  effect_edit.cpp
  req_vec_fix.cpp
  tab_building.cpp
  tab_good.cpp
  tab_multiplier.cpp
  tab_terrains.cpp
  univ_value.cpp
  validity.cpp
)

target_link_libraries(freeciv21-ruledit server)
target_link_libraries(freeciv21-ruledit tools_ruleutil)
target_link_libraries(freeciv21-ruledit tools_shared)
target_link_libraries(freeciv21-ruledit Qt5::Widgets)
add_dependencies(freeciv21-ruledit freeciv_translations)

install(TARGETS freeciv21-ruledit
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT tool_ruledit)
