/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// utility
#include "support.h"

#include "luascript_types.h"

// Signal datastructure.
struct signal {
  QList<QString> callbacks; // connected callbacks
  const char *depr_msg;     // deprecation message to show if handler added
};

typedef fc_lua *(*get_lua)();
template <typename... Args> class Signal {
public:
  Signal(const char *name, get_lua getter, const char *deprecated = nullptr)
      : name(name), getter(getter)
  {
    auto fcl = getter();
    std::lock_guard<std::mutex> l(fcl->mu);

    if (fcl->signals_hash.constFind(name) != fcl->signals_hash.constEnd()) {
      qFatal("Attempting to create a second signal named '%s'", name);
    }

    struct signal sig = {{}, deprecated};
    fcl->signals_hash.insert(name, sig);
    fcl->signal_names.push_back(name);
  }

  void operator()(Args... args) const
  {
    auto fcl = getter();

    sol::state_view lua(fcl->state);

    for (const auto &handler : fcl->signals_hash[name].callbacks) {
      lua[handler](std::forward<Args>(args)...);
    }
  }

private:
  QString name;
  get_lua getter;
};

void luascript_signal_callback(struct fc_lua *fcl, const char *signal_name,
                               const char *callback_name, bool create);
bool luascript_signal_callback_defined(struct fc_lua *fcl,
                                       const char *signal_name,
                                       const char *callback_name);

QString luascript_signal_by_index(struct fc_lua *fcl, int sindex);
QString luascript_signal_callback_by_index(struct fc_lua *fcl,
                                           const char *signal_name,
                                           int sindex);
