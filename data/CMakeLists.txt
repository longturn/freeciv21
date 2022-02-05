set(RULESET_LIST)

function(add_ruleset)
  cmake_parse_arguments(ARG "SERV_FILE" "NAME;README" "" ${ARGN})
  if (ARG_SERV_FILE)
    install(
      FILES "${ARG_NAME}.serv"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
      COMPONENT freeciv21)
  endif()
  if (ARG_README)
    install(
      FILES "${ARG_NAME}/${ARG_README}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/${ARG_NAME}"
      COMPONENT freeciv21)
  endif()
  list(APPEND RULESET_LIST ${ARG_NAME})
  set(RULESET_LIST "${RULESET_LIST}" PARENT_SCOPE)
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
    "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/${ARG_NAME}"
    COMPONENT freeciv21)
endfunction()

function(add_tileset)
  cmake_parse_arguments(ARG "" "NAME;README" "" ${ARGN})
  install(
    FILES ${ARG_NAME}.tilespec
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21)
  install(
    DIRECTORY ${ARG_NAME}
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
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
  add_ruleset(NAME royale SERV_FILE README README.royale)

  # Misc
  install(
    FILES default.serv
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21)
  install(
    FILES
    default/ai_effects.ruleset
    default/default.lua
    default/nationlist.ruleset
    DESTINATION
    "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/default"
    COMPONENT freeciv21)
  install(
    FILES override/nation.ruleset
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/override"
    COMPONENT freeciv21)

  # Ruleset tests
  if (BUILD_TESTING AND FREECIV_ENABLE_SERVER)
    # The stub ruleset isn't installed, but we test it
    list(APPEND RULESET_LIST stub)
    add_subdirectory(tests)
  endif()
endif()

if (FREECIV_ENABLE_SERVER)
  # Scenarios
  add_subdirectory(scenarios)
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
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21)
  install(
    DIRECTORY buildings
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.png)
  install(
    DIRECTORY flags
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.png)
  install(
    DIRECTORY misc
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.png PATTERN *.tilespec PATTERN *.spec)
  install(
    DIRECTORY override
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.png PATTERN *.spec)
  install(
    DIRECTORY themes/gui-qt
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/themes"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.jpg PATTERN *.png PATTERN *.qss)
  install(
    DIRECTORY wonders
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.png)
  install(
    FILES stdsounds.soundspec
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21)
  install(
    DIRECTORY stdsounds
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.ogg)
  install(
    FILES stdmusic.musicspec
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21)
  install(
    DIRECTORY stdmusic
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
    COMPONENT freeciv21
    FILES_MATCHING PATTERN *.ogg)
endif()

if (FREECIV_ENABLE_RULEDIT)
  install(
    FILES ruledit/comments-3.1.txt
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ruledit"
    COMPONENT tool_ruledit)
endif()
