function(add_ruleset)
  cmake_parse_arguments(ARG "SERV_FILE" "NAME;README" "" ${ARGN})
  if (ARG_SERV_FILE)
    install(
      FILES "${ARG_NAME}.serv"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  endif()
  if (ARG_README)
    install(
      FILES "${ARG_NAME}/${ARG_README}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/${ARG_NAME}")
  endif()
  install(
    FILES
    ${ARG_NAME}/buildings.ruleset
    ${ARG_NAME}/cities.ruleset
    ${ARG_NAME}/effects.ruleset
    ${ARG_NAME}/game.ruleset
    ${ARG_NAME}/governments.ruleset
    ${ARG_NAME}/nations.ruleset
    ${ARG_NAME}/parser.lua
    ${ARG_NAME}/script.lua
    ${ARG_NAME}/styles.ruleset
    ${ARG_NAME}/techs.ruleset
    ${ARG_NAME}/terrain.ruleset
    ${ARG_NAME}/units.ruleset
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/${ARG_NAME}")
endfunction()

if (FREECIV_ENABLE_SERVER OR FREECIV_ENABLE_RULEDIT)
  # Nations
  add_subdirectory(nation)

  # Rulesets
  add_ruleset(NAME alien SERV_FILE README README.alien)
  add_ruleset(NAME civ1 SERV_FILE)
  add_ruleset(NAME civ2 SERV_FILE)
  add_ruleset(NAME civ2civ3 SERV_FILE README README.civ2civ3)
  add_ruleset(NAME classic SERV_FILE README README.classic)
  add_ruleset(NAME experimental SERV_FILE README README.experimental)
  add_ruleset(NAME multiplayer SERV_FILE README README.multiplayer)

  # Misc
  install(
    FILES default.serv
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  install(
    FILES
    default/ai_effects.ruleset
    default/default.lua
    default/nationlist.ruleset
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/default")
  install(
    FILES override/nation.ruleset
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/override")
endif()
