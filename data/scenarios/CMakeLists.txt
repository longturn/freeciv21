set(scenarios
    british-isles
    earth-small
    earth-large
    europe
    europe_1900_WWI
    hagworld
    france
    italy
    iberian-peninsula
    north_america
    japan
    tutorial
    tileset-demo
)
set(compression GZip)
set(compression_ext gz)

foreach(scenario ${scenarios})
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${scenario}.sav.${compression_ext}"
    COMMAND "${CMAKE_COMMAND}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/compress.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/${scenario}.sav.${compression_ext}"
      "${CMAKE_CURRENT_SOURCE_DIR}/${scenario}.sav"
      ${compression}
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/${scenario}.sav"
      "${CMAKE_CURRENT_SOURCE_DIR}/compress.cmake"
    COMMENT "Compressing ${scenario}.sav")
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${scenario}.sav.${compression_ext}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scenarios")
endforeach()

list(TRANSFORM scenarios PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
list(TRANSFORM scenarios APPEND ".sav.${compression_ext}")
add_custom_target(scenarios ALL DEPENDS "${scenarios}")
