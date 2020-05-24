add_subdirectory(ruleutil)
add_subdirectory(shared)

add_subdirectory(fcmp)
add_subdirectory(ruledit)

add_executable(freeciv-manual civmanual.c)
target_link_libraries(freeciv-manual PRIVATE freeciv_server)
target_link_libraries(freeciv-manual PRIVATE freeciv_client)
target_link_libraries(freeciv-manual PRIVATE tools_shared)

add_executable(freeciv-ruleup ruleup.c)
target_link_libraries(freeciv-ruleup PRIVATE freeciv_server)
target_link_libraries(freeciv-ruleup PRIVATE tools_shared)
target_link_libraries(freeciv-ruleup PRIVATE tools_ruleutil)
