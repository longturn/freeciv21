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

function(add_tileset)
  cmake_parse_arguments(ARG "" "NAME;README" "" ${ARGN})
  install(
    FILES ${ARG_NAME}.tilespec
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  install(
    DIRECTORY ${ARG_NAME}
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    FILES_MATCHING PATTERN *.png PATTERN *.spec PATTERN README.*)
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

if (FREECIV_ENABLE_CLIENT)
  # Tilesets
  add_tileset(NAME amplio)
  add_tileset(NAME amplio2)
  add_tileset(NAME cimpletoon)
  add_tileset(NAME hex2t)
  add_tileset(NAME hexemplio)
  add_tileset(NAME isophex)
  add_tileset(NAME isotrident)
  add_tileset(NAME trident)

  # Misc
  install(
    FILES helpdata.txt
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
  install(
    DIRECTORY buildings
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    FILES_MATCHING PATTERN *.png)
  install(
    DIRECTORY flags
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    FILES_MATCHING PATTERN *.png)
  install(
    DIRECTORY misc
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    FILES_MATCHING PATTERN *.png PATTERN *.tilespec PATTERN *.spec)
  install(
    DIRECTORY override
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    FILES_MATCHING PATTERN *.png PATTERN *.spec)
  install(
    DIRECTORY themes/gui-qt
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/themes"
    FILES_MATCHING PATTERN *.jpg PATTERN *.png PATTERN *.qss)
  install(
    DIRECTORY wonders
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    FILES_MATCHING PATTERN *.png)
endif()

if (FREECIV_ENABLE_RULEDIT)
  install(
    FILES ruledit/comments-3.1.txt
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ruledit")
endif()
