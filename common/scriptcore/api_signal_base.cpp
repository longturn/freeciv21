/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
/* common/scriptcore */

// Sol
#include "sol/sol.hpp"

#include "luascript.h"
#include "luascript_signal.h"

#include "api_signal_base.h"

namespace {

/**
   Connects a callback function to a certain signal.
 */
void signal_connect(sol::this_state s, const char *signal_name,
                    const char *callback_name)
{
  struct fc_lua *fcl = luascript_get_fcl(s.lua_state());

  LUASCRIPT_CHECK(s, fcl != nullptr, "Undefined Freeciv21 lua state!");

  luascript_signal_callback(fcl, signal_name, callback_name, true);
}

/**
   Removes a callback function to a certain signal.
 */
void signal_remove(sol::this_state s, const char *signal_name,
                   const char *callback_name)
{
  struct fc_lua *fcl = luascript_get_fcl(s.lua_state());

  LUASCRIPT_CHECK(s, fcl != nullptr, "Undefined Freeciv21 lua state!");

  luascript_signal_callback(fcl, signal_name, callback_name, false);
}

/**
   Returns if a callback function to a certain signal is defined.
 */
bool signal_defined(sol::this_state s, const char *signal_name,
                    const char *callback_name)
{
  struct fc_lua *fcl = luascript_get_fcl(s.lua_state());

  LUASCRIPT_CHECK(s, fcl != nullptr, "Undefined Freeciv21 lua state!",
                  false);

  return luascript_signal_callback_defined(fcl, signal_name, callback_name);
}

/**
   Return the name of the signal with the given index.
 */
QString signal_callback_by_index(sol::this_state s, const char *signal_name,
                                 int sindex)
{
  struct fc_lua *fcl = luascript_get_fcl(s.lua_state());

  LUASCRIPT_CHECK(s, fcl != nullptr, "Undefined Freeciv21 lua state!",
                  nullptr);

  return luascript_signal_callback_by_index(fcl, signal_name, sindex);
}

/**
   Return the name of the 'index' callback function of the signal with the
   name 'signal_name'.
 */
const char *signal_by_index(sol::this_state s, int sindex)
{
  struct fc_lua *fcl = luascript_get_fcl(s.lua_state());

  LUASCRIPT_CHECK(s, fcl != nullptr, "Undefined Freeciv21 lua state!",
                  nullptr);
  auto callback = luascript_signal_by_index(fcl, sindex);
  // FIXME memory leak
  return callback.isEmpty() ? nullptr : qstrdup(qUtf8Printable(callback));
}

} // namespace

void setup_lua_signal(sol::state_view lua)
{
  auto signal = lua["signal"].get_or_create<sol::table>();

  signal.set("connect", signal_connect);
  signal.set("remove", signal_remove);
  signal.set("defined", signal_defined);

  auto find = lua["find"].get_or_create<sol::table>();
  find.set("signal", signal_by_index);
  find.set("signal_callback", signal_callback_by_index);

  lua.script(R"(
function signal.list()
  local signal_id = 0;
  local signal_name = nil;

  log.normal("List of signals:");
  repeat
    local signal_name = find.signal(signal_id);
    if (signal_name) then
      local callback_id = 0;
      local callback_name = nil;

      log.normal("- callbacks for signal '%s':", signal_name);
      repeat
        local callback_name = find.signal_callback(signal_name, callback_id);
        if (callback_name) then
          log.normal("   [%3d] '%s'", callback_id, callback_name);
        end
        callback_id = callback_id + 1;
      until (callback_name == nil);

      signal_id = signal_id + 1;
    end
  until (signal_name == nil);
end

function signal.replace(signal_name, callback_name)
  if signal.defined(signal_name, callback_name) then
    signal.remove(signal_name, callback_name)
  end
  signal.connect(signal_name, callback_name)
end
  )");
}
