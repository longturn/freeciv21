add_library(
  lua
  STATIC
  lapi.c
  lauxlib.c
  lbaselib.c
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

if (UNIX AND NOT EMSCRIPTEN)
  # POSIX is not UNIX but close
  # Emscripten claims to be UNIX but doesn't have _longjmp, so rely only on
  # standard C features. This means that a few Lua functions will be
  # unsupported.
  target_compile_definitions(lua PUBLIC LUA_USE_POSIX)
elseif (EMSCRIPTEN)
  # Without optimizations, Lua easily overflows the V8 stack
  target_compile_options(lua PRIVATE -O1)
endif()
if(FREECIV_HAVE_LIBREADLINE)
  target_compile_definitions(lua PUBLIC LUA_USE_READLINE)
endif()
