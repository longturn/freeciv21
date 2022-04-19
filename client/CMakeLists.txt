# Generate the packet handling code
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    "" # common/packets_gen.h
    "" # common/packets_gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
    "" # server/hand_gen.h
    "" # server/hand_gen.c
  VERBATIM
  DEPENDS
    ${CMAKE_SOURCE_DIR}/common/generate_packets.py
    ${CMAKE_SOURCE_DIR}/common/networking/packets.def
)

add_library(
  client_gen
  STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/packhand_gen.h
)

target_include_directories(client_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(client_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(client_gen PUBLIC common)

if (AUDIO_SDL)
  set(FREECIV_AUDIO audio_sdl.cpp audio_sdl.h)
endif()

add_library(
  client
  STATIC
  attribute.cpp
  attribute.h
  audio_none.cpp
  audio_none.h
  audio.cpp
  audio.h
  chatline_common.cpp
  chatline_common.h
  citybar.cpp
  citybar.h
  citydlg_common.cpp
  citydlg_common.h
  cityrepdata.cpp
  cityrepdata.h
  client_main.cpp
  client_main.h
  climap.cpp
  climap.h
  climisc.cpp
  climisc.h
  clinet.cpp
  clinet.h
  colors_common.cpp
  colors_common.h
  connectdlg_common.cpp
  connectdlg_common.h
  control.cpp
  control.h
  editor.cpp
  editor.h
  global_worklist.cpp
  global_worklist.h
  goto.cpp
  goto.h
  governor.cpp
  governor.h
  layer_background.cpp
  layer_background.h
  layer_base_flags.cpp
  layer_base_flags.h
  layer_darkness.cpp
  layer_darkness.h
  layer_special.cpp
  layer_special.h
  layer_terrain.cpp
  layer_terrain.h
  layer.cpp
  layer.h
  luaconsole_common.cpp
  luaconsole_common.h
  mapctrl_common.cpp
  mapctrl_common.h
  mapview_common.cpp
  mapview_common.h
  messagewin_common.cpp
  messagewin_common.h
  music.cpp
  music.h
  options.cpp
  options.h
  overview_common.cpp
  overview_common.h
  plrdlg_common.cpp
  plrdlg_common.h
  repodlgs_common.cpp
  repodlgs_common.h
  reqtree.cpp
  reqtree.h
  text.cpp
  text.h
  themes_common.cpp
  themes_common.h
  tilespec.cpp
  tilespec.h
  update_queue.cpp
  update_queue.h
  voteinfo.cpp
  voteinfo.h
  ${FREECIV_AUDIO}
)
if(NOT EMSCRIPTEN)
  target_sources(client PRIVATE servers.cpp servers.h)
endif()

target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT EMSCRIPTEN)
  target_include_directories(client PUBLIC ${SDL2_INCLUDE_DIR})
  target_include_directories(client PUBLIC ${SDL2_MIXER_INCLUDE_DIR})
endif()

add_subdirectory(luascript)

target_link_libraries(client PUBLIC Qt5::Widgets)
target_link_libraries(client PUBLIC Qt5::Svg)
target_link_libraries(client PUBLIC ${SDL2_MIXER_LIBRARIES} ${SDL2_LIBRARY})

target_link_libraries(client PUBLIC client_gen)
target_link_libraries(client PUBLIC cvercmp)
target_link_libraries(client PUBLIC luascript)

add_compile_definitions(NOMINMAX)

if(FREECIV_ENABLE_CLIENT)
  add_subdirectory(gui-qt)
  add_executable(freeciv21-client ${GUI_TYPE} packhand.cpp packhand.h) # packhand.c depends on gui-qt
  if(EMSCRIPTEN)
    target_link_options(freeciv21-client PRIVATE
                        -sASSERTIONS=1
                        -sWEBSOCKET_URL=wss://
                        -sTOTAL_MEMORY=52428800
                        -sALLOW_MEMORY_GROWTH=1
                        "-sEXPORTED_RUNTIME_METHODS=['UTF16ToString','stringToUTF16','callMain','printErr']"
                        --bind
                        -sUSE_SDL_MIXER=2
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/flags@data/flags"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/helpdata.txt@data/helpdata.txt"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/misc@data/misc"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/buildings@data/buildings"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/wonders@data/wonders"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/hexemplio.tilespec@data/hexemplio.tilespec"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/hexemplio@data/hexemplio"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/amplio2.tilespec@data/amplio2.tilespec"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/amplio2@data/amplio2"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/themes/gui-qt/Classic@data/themes/gui-qt/Classic"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/themes/gui-qt/icons@data/themes/gui-qt/icons"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/stdmusic.musicspec@data/stdmusic.musicspec"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/stdmusic@data/stdmusic"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/stdsounds@data/stdsounds"
                        "--preload-file=${CMAKE_SOURCE_DIR}/data/stdsounds.soundspec@data/stdsounds.soundspec"
                        "--exclude-file=*CMakeLists*"
                        "--exclude-file=*.icns"
                        "--exclude-file=*.serv"
                        "--exclude-file=*.svg"
    )
  endif()
  target_link_libraries(freeciv21-client client)
  target_link_libraries(freeciv21-client gui-qt)
  add_dependencies(freeciv21-client freeciv_translations)
  install(TARGETS freeciv21-client
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT freeciv21)
endif()