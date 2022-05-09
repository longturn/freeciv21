set(CMAKE_AUTOMOC ON)

add_executable(
  freeciv21-ruledit ${GUI_TYPE}
  conversion_log.cpp
  edit_utype.cpp
  effect_edit.cpp
  req_edit.cpp
  req_vec_fix.cpp
  requirers_dlg.cpp
  ruledit_qt.cpp
  ruledit.cpp
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
