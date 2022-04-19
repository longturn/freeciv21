set(CMAKE_AUTOMOC ON)

add_executable(
  freeciv21-ruledit ${GUI_TYPE}
  conversion_log.cpp
  conversion_log.h
  edit_utype.cpp
  edit_utype.h
  effect_edit.cpp
  effect_edit.h
  req_edit.cpp
  req_edit.h
  req_vec_fix.cpp
  req_vec_fix.h
  requirers_dlg.cpp
  requirers_dlg.h
  ruledit_qt.cpp
  ruledit_qt.h
  ruledit.cpp
  ruledit.h
  tab_building.cpp
  tab_building.h
  tab_enablers.cpp
  tab_enablers.h
  tab_extras.cpp
  tab_extras.h
  tab_good.cpp
  tab_good.h
  tab_gov.cpp
  tab_gov.h
  tab_misc.cpp
  tab_misc.h
  tab_multiplier.cpp
  tab_multiplier.h
  tab_nation.cpp
  tab_nation.h
  tab_tech.cpp
  tab_tech.h
  tab_terrains.cpp
  tab_terrains.h
  tab_unit.cpp
  tab_unit.h
  univ_value.cpp
  univ_value.h
  validity.cpp
  validity.h
)

target_link_libraries(freeciv21-ruledit server)
target_link_libraries(freeciv21-ruledit tools_ruleutil)
target_link_libraries(freeciv21-ruledit tools_shared)
target_link_libraries(freeciv21-ruledit Qt5::Widgets)
add_dependencies(freeciv21-ruledit freeciv_translations)

install(TARGETS freeciv21-ruledit
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT tool_ruledit)