/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstring>

/* dependencies/lua */
extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

/* utility */
#include "log.h"
#include "support.h"

/* common */
#include "events.h"

#include "api_game_specenum.h"

#define API_SPECENUM_INDEX_NAME(type) api_specenum_##type##_index
#define API_SPECENUM_CREATE_TABLE(L, type, name)                            \
  api_specenum_create_table((L), (name), API_SPECENUM_INDEX_NAME(type))

/*************************************************************************/ /**
   Define a the __index (table, key) -> value  metamethod
   Return the enum value whose name is the concatenation of prefix and key.
   The fetched value is written back to the lua table, and further accesses
   will resolve there instead of this function.
 *****************************************************************************/
#define API_SPECENUM_DEFINE_INDEX(type_name, prefix)                        \
  static int(API_SPECENUM_INDEX_NAME(type_name))(lua_State * L)             \
  {                                                                         \
    static char _buf[128];                                                  \
    const char *_key;                                                       \
    enum type_name _value;                                                  \
    luaL_checktype(L, 1, LUA_TTABLE);                                       \
    _key = luaL_checkstring(L, 2);                                          \
    fc_snprintf(_buf, sizeof(_buf), prefix "%s", _key);                     \
    _value = type_name##_by_name(_buf, strcmp);                             \
    if (_value != type_name##_invalid()) {                                  \
      /* T[_key] = _value */                                                \
      lua_pushstring(L, _key);                                              \
      lua_pushinteger(L, _value);                                           \
      lua_rawset(L, 1);                                                     \
      lua_pushinteger(L, _value);                                           \
    } else {                                                                \
      lua_pushnil(L);                                                       \
    }                                                                       \
    return 1;                                                               \
  }

/*************************************************************************/ /**
   Create a module table and set the member lookup function.
 *****************************************************************************/
static void api_specenum_create_table(lua_State *L, const char *name,
                                      lua_CFunction findex)
{
  /* Insert a module table in the global environment,
   * or reuse any preexisting table */
  lua_getglobal(L, name);
  if (lua_isnil(L, -1)) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, name);
  }
  fc_assert_ret(lua_istable(L, -1));
  /* Create a metatable */
  lua_newtable(L); /* stack: module mt */
  lua_pushliteral(L, "__index");
  lua_pushcfunction(L, findex); /* stack: module mt '__index' index */
  lua_rawset(L, -3);            /* stack: module mt */
  lua_setmetatable(L, -2);      /* stack: module */
  lua_pop(L, 1);
}

/*************************************************************************/ /**
   Define the __index function for each exported specenum type.
 *****************************************************************************/
API_SPECENUM_DEFINE_INDEX(event_type, "E_")

/*************************************************************************/ /**
   Load the specenum modules into Lua state L.
 *****************************************************************************/
int api_specenum_open(lua_State *L)
{
  API_SPECENUM_CREATE_TABLE(L, event_type, "E");
  return 0;
}
