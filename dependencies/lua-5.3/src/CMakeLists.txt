add_library(
  lua
  STATIC
  lapi.c
  lauxlib.c
  lbaselib.c
  lbitlib.c
  lcode.c
  lcorolib.c
  lctype.c
  ldblib.c
  ldebug.c
  ldo.c
  ldump.c
  lfunc.c
  lgc.c
  linit.c
  liolib.c
  llex.c
  lmathlib.c
  lmem.c
  loadlib.c
  lobject.c
  lopcodes.c
  loslib.c
  lparser.c
  lstate.c
  lstring.c
  lstrlib.c
  ltable.c
  ltablib.c
  ltm.c
  lua.hpp
  lundump.c
  lutf8lib.c
  lvm.c
  lzio.c
)

target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lua PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(lua PUBLIC ${CMAKE_DL_LIBS})
if(NOT MSVC)
  target_link_libraries(lua PUBLIC m)
endif()

if(UNIX)
  # POSIX is not UNIX but close
  target_compile_definitions(lua PUBLIC LUA_USE_POSIX)
endif()
if(FREECIV_HAVE_LIBREADLINE)
  target_compile_definitions(lua PUBLIC LUA_USE_READLINE)
endif()
