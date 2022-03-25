/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

/*****************************************************************************
  Signals implementation.

  New signal types can be declared with script_signal_create. Each
  signal should have a unique name string.
  All signal declarations are in signals_create, for convenience.

  A signal may have any number of Lua callback functions connected to it
  at any given time.

  A signal emission invokes all associated callbacks in the order they were
  connected:

  * A callback can stop the current signal emission, preventing the callbacks
    connected after it from being invoked.

  * A callback can detach itself from its associated signal.

  Lua callbacks functions are able to do these via their return values.

  All Lua callback functions can return a value. Example:
    return false

  If the value is 'true' the current signal emission will be stopped.
*****************************************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <string>
#include <unordered_map>
#include <vector>

// Sol
#include "sol/sol.hpp"

// utility
#include "deprecations.h"

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"

#include "luascript_signal.h"

/**
   Connects a callback function to a certain signal.
 */
void luascript_signal_callback(struct fc_lua *fcl, const char *signal_name,
                               const char *callback_name, bool create)
{
  QString callback_found;

  fc_assert_ret(fcl != nullptr);

  auto it = fcl->signals_hash.find(signal_name);
  if (it == fcl->signals_hash.end()) {
    luascript_error(fcl->state, "Signal \"%s\" does not exist.",
                    signal_name);
    return;
  }

  auto &sig = *it;

  for (auto callback : qAsConst(sig.callbacks)) {
    if (callback == callback_name) {
      callback_found = callback;
      break;
    }
  }

  if (sig.depr_msg != nullptr) {
    qCWarning(deprecations_category, "%s", sig.depr_msg);
  }

  if (create) {
    if (!callback_found.isEmpty()) {
      luascript_error(fcl->state,
                      "Signal \"%s\" already has a callback "
                      "called \"%s\".",
                      signal_name, callback_name);
    } else {
      sig.callbacks.append(callback_name);
    }
  } else {
    if (!callback_found.isEmpty()) {
      sig.callbacks.removeAll(callback_found);
    }
  }
}

/**
   Returns if a callback function to a certain signal is defined.
 */
bool luascript_signal_callback_defined(struct fc_lua *fcl,
                                       const char *signal_name,
                                       const char *callback_name)
{
  fc_assert_ret_val(fcl != nullptr, false);

  auto it = fcl->signals_hash.find(signal_name);
  if (it == fcl->signals_hash.end()) {
    return false;
  }

  struct signal sig = it.value();

  // check for a duplicate callback
  for (auto callback : qAsConst(sig.callbacks)) {
    if (callback == callback_name) {
      return true;
    }
  }

  return false;
}

/**
   Return the name of the signal with the given index.
 */
QString luascript_signal_by_index(struct fc_lua *fcl, int sindex)
{
  fc_assert_ret_val(fcl != nullptr, QString());

  return sindex < fcl->signal_names.size() ? fcl->signal_names.at(sindex)
                                           : QString();
}

/**
   Return the name of the 'index' callback function of the signal with the
   name 'signal_name'.
 */
QString luascript_signal_callback_by_index(struct fc_lua *fcl,
                                           const char *signal_name,
                                           int sindex)
{
  fc_assert_ret_val(fcl != nullptr, nullptr);

  auto it = fcl->signals_hash.find(signal_name);
  if (it == fcl->signals_hash.end()) {
    return nullptr;
  }

  struct signal sig = it.value();

  if (sindex < sig.callbacks.size()) {
    return sig.callbacks.at(sindex);
  }

  return nullptr;
}
