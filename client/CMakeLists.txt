add_library(
  cma_core
  STATIC
  agents/agents.c
  agents/cma_core.c
  agents/cma_fec.c
  agents/sha.c
)
target_include_directories(cma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/agents)
target_link_libraries(cma_core PUBLIC freeciv)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/luascript)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/luascript/tolua_client_gen.h
    ${CMAKE_CURRENT_BINARY_DIR}/luascript/tolua_client_gen.c
  COMMAND
    tolua_exe
    -n client
    -o ${CMAKE_CURRENT_BINARY_DIR}/luascript/tolua_client_gen.c
    -H ${CMAKE_CURRENT_BINARY_DIR}/luascript/tolua_client_gen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/luascript/tolua_client.pkg
  VERBATIM
  DEPENDS
    tolua_exe
    ${CMAKE_CURRENT_SOURCE_DIR}/luascript/tolua_client.pkg
)
add_library(
  luascript
  STATIC
  luascript/api_client_base.c
  luascript/script_client.c
  # Generated
  ${CMAKE_CURRENT_BINARY_DIR}/luascript/tolua_client_gen.c
)
target_include_directories(luascript PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/luascript)
target_include_directories(luascript PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/luascript)
target_link_libraries(luascript PUBLIC freeciv)

add_library(
  freeciv_client_gen
  STATIC
  packhand_gen.h
  packhand_gen.c
)
target_include_directories(freeciv_client_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(freeciv_client_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(freeciv_client_gen PUBLIC freeciv)

add_library(
  freeciv_client
  STATIC
  attribute.c
  citydlg_common.c
  cityrepdata.c
  client_main.c
  chatline_common.c
  connectdlg_common.c
  climisc.c
  climap.c
  clinet.c
  colors_common.c
  control.c
  editor.c
  global_worklist.c
  gui_interface.c
  goto.c
  helpdata.c
  luaconsole_common.c
  mapctrl_common.c
  mapview_common.c
  messagewin_common.c
  music.c
  overview_common.c
  packhand.c
  plrdlg_common.c
  options.c
  repodlgs_common.c
  reqtree.c
  servers.c
  text.c
  themes_common.c
  tilespec.c
  unitselect_common.c
  update_queue.c
  voteinfo.c
  zoom.c
  audio.c
  audio_none.c
)
target_include_directories(freeciv_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(freeciv_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(freeciv_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/luascript) # Terrible
target_include_directories(freeciv_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/agents) # Terrible
target_link_libraries(freeciv_client PUBLIC freeciv freeciv_client_gen cvercmp)

target_link_libraries(cma_core PUBLIC freeciv_client)
target_link_libraries(luascript PUBLIC freeciv_client)

add_subdirectory(gui-qt)
target_link_libraries(freeciv_client PUBLIC freeciv_client_qt)

add_executable(freeciv-qt dummycxx.cpp)
target_link_libraries(freeciv-qt PRIVATE freeciv_client_qt)
if (EMSCRIPTEN)
  target_link_options(freeciv-qt PRIVATE
    --bind
    -Wl,--no-check-features
    "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0"
    "SHELL:-s WASM=1"
    -lidbfs.js
    --emrun
    "SHELL:-s USE_PTHREADS=0"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@data"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/alien\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/cimpletoon\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/civ\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/classic\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/default\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/experimental\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/graphics"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/icons"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/nation"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/multiplayer\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/ruledit"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/sandbox\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/scenarios"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/stdsounds"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/stdmusic"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/themes/gtk\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/themes/gui-sdl2\*"
    "SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/data/webperimental\*"
    "SHELL:--exclude-file \*Makefile\*"
    "SHELL:--exclude-file \*README\*"
    "SHELL:--exclude-file \*.icns"
    "SHELL:--exclude-file \*.serv"
    "SHELL:--exclude-file \*.svg"
    "SHELL:--exclude-file \*.xml"
    "SHELL:-s ASSERTIONS=1"
    "SHELL:-s ASYNCIFY=1"
    "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS='[\"stringToUTF16\", \"UTF16ToString\", \"stringToUTF8\", \"UTF8ToString\", \"setValue\"]'"
    "SHELL:-s TOTAL_MEMORY=33554432"
  )
endif()
