add_subdirectory(ruleutil)
add_subdirectory(shared)

if (FREECIV_ENABLE_FCMP_CLI OR FREECIV_ENABLE_FCMP_QT)
  add_subdirectory(fcmp)
endif()

if (FREECIV_ENABLE_RULEDIT)
  add_subdirectory(ruledit)
endif()

if (FREECIV_ENABLE_CIVMANUAL)
  add_executable(freeciv-manual civmanual.c)
  target_link_libraries(freeciv-manual server)
  target_link_libraries(freeciv-manual client)
  target_link_libraries(freeciv-manual tools_shared)
endif()

if (FREECIV_ENABLE_RULEUP)
  add_executable(freeciv-ruleup ruleup.c)
  target_link_libraries(freeciv-ruleup server)
  target_link_libraries(freeciv-ruleup tools_shared)
  target_link_libraries(freeciv-ruleup tools_ruleutil)
endif()
