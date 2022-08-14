/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */
/* common/scriptcore */
#include "luascript.h"
#include "luascript_signal.h"

#include "api_signal_base.h"

/**
   Connects a callback function to a certain signal.
 */
void api_signal_connect(lua_State *L, const char *signal_name,
                        const char *callback_name)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string);
  LUASCRIPT_CHECK_ARG_NIL(L, callback_name, 3, string);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != nullptr, "Undefined Freeciv21 lua state!");

  luascript_signal_callback(fcl, signal_name, callback_name, true);
}

/**
   Removes a callback function to a certain signal.
 */
void api_signal_remove(lua_State *L, const char *signal_name,
                       const char *callback_name)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string);
  LUASCRIPT_CHECK_ARG_NIL(L, callback_name, 3, string);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != nullptr, "Undefined Freeciv21 lua state!");

  luascript_signal_callback(fcl, signal_name, callback_name, false);
}

/**
   Returns if a callback function to a certain signal is defined.
 */
bool api_signal_defined(lua_State *L, const char *signal_name,
                        const char *callback_name)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string, false);
  LUASCRIPT_CHECK_ARG_NIL(L, callback_name, 3, string, false);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != nullptr, "Undefined Freeciv21 lua state!",
                  false);

  return luascript_signal_callback_defined(fcl, signal_name, callback_name);
}

/**
   Return the name of the signal with the given index.
 */
const char *api_signal_callback_by_index(lua_State *L,
                                         const char *signal_name, int sindex)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_ARG_NIL(L, signal_name, 2, string, nullptr);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != nullptr, "Undefined Freeciv21 lua state!",
                  nullptr);

  return luascript_signal_callback_by_index(fcl, signal_name, sindex);
}

/**
   Return the name of the 'index' callback function of the signal with the
   name 'signal_name'.
 */
const char *api_signal_by_index(lua_State *L, int sindex)
{
  struct fc_lua *fcl;

  LUASCRIPT_CHECK_STATE(L, nullptr);

  fcl = luascript_get_fcl(L);

  LUASCRIPT_CHECK(L, fcl != nullptr, "Undefined Freeciv21 lua state!",
                  nullptr);
  auto callback = luascript_signal_by_index(fcl, sindex);
  // FIXME memory leak
  return callback.isEmpty() ? nullptr : qstrdup(qUtf8Printable(callback));
}
