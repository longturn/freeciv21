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

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
  # Built-in compression support available in CMake
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
      DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scenarios"
      COMPONENT freeciv21)
  endforeach()

  list(TRANSFORM scenarios PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
  list(TRANSFORM scenarios APPEND ".sav.${compression_ext}")
  add_custom_target(scenarios ALL DEPENDS "${scenarios}")
else()
  # No built-in compression, fallback to gzip if available
  find_program(GZIP_EXECUTABLE gzip)
  if (GZIP_EXECUTABLE)
    # Compress with gzip
    set(compression_command "${GZIP_EXECUTABLE}")
    set(compression_ext gz)

    foreach(scenario ${scenarios})
      add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${scenario}.sav.${compression_ext}"
        COMMAND "${GZIP_EXECUTABLE}" -c9
          <${CMAKE_CURRENT_SOURCE_DIR}/${scenario}.sav
          >${CMAKE_CURRENT_BINARY_DIR}/${scenario}.sav.${compression_ext}
        DEPENDS
          "${CMAKE_CURRENT_SOURCE_DIR}/${scenario}.sav"
          "${CMAKE_CURRENT_SOURCE_DIR}/compress.cmake"
        COMMENT "Compressing ${scenario}.sav")
      install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${scenario}.sav.${compression_ext}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scenarios"
        COMPONENT freeciv21)
    endforeach()

    list(TRANSFORM scenarios PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
    list(TRANSFORM scenarios APPEND ".sav.${compression_ext}")
    add_custom_target(scenarios ALL DEPENDS "${scenarios}")
  else()
    # No compression available, install uncompressed files
    foreach(scenario ${scenarios})
      install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/${scenario}.sav"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scenarios"
        COMPONENT freeciv21)
    endforeach()
  endif()
endif()
