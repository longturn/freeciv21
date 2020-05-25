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
target_link_libraries(lua PRIVATE m)
# Replace FindLUA variables
set(LUA_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "" FORCE)
set(LUA_LIBRARY lua CACHE STRING "" FORCE)

add_library(tolua IMPORTED STATIC)
set_target_properties(tolua PROPERTIES
  IMPORTED_LOCATION /usr/lib/libtolua.a
  INTERFACE_LINK_DEPENDS lua)
