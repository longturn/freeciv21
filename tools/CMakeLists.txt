if (FREECIV_ENABLE_RULEDIT OR FREECIV_ENABLE_RULEUP)
  add_subdirectory(ruleutil)
endif()
if (FREECIV_ENABLE_CIVMANUAL OR FREECIV_ENABLE_RULEUP)
  add_subdirectory(shared)
endif()

if (FREECIV_ENABLE_FCMP_CLI OR FREECIV_ENABLE_FCMP_QT)
  add_subdirectory(fcmp)
endif()

if (FREECIV_ENABLE_RULEDIT)
  add_subdirectory(ruledit)
endif()

if (FREECIV_ENABLE_CIVMANUAL)
  add_executable(freeciv21-manual civmanual.cpp)
  target_link_libraries(freeciv21-manual server)
  target_link_libraries(freeciv21-manual client)
  target_link_libraries(freeciv21-manual tools_shared)
  install(TARGETS freeciv21-manual
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT tool_manual)
endif()

if (FREECIV_ENABLE_RULEUP)
  add_executable(freeciv21-ruleup ruleup.cpp)
  target_link_libraries(freeciv21-ruleup server)
  target_link_libraries(freeciv21-ruleup tools_shared)
  target_link_libraries(freeciv21-ruleup tools_ruleutil)
  install(TARGETS freeciv21-ruleup
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT tool_ruleup)
endif()

