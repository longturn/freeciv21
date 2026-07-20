// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "api_signal_base.h"

// dependencies/lua
extern "C" {
#include "lua.h"
}

// common
#include "luascript.h"
#include "luascript_signal.h"

// Qt
#include <QByteArrayAlgorithms> // qstrdup
#include <QString>
#include <QtGlobal> // qUtf8Printable

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
