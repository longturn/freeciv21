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
#include "genlist.h"

// common
#include "achievements.h"
#include "actions.h"
#include "city.h"
#include "connection.h"
#include "events.h"
#include "fc_types.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "nation.h"
#include "player.h"
#include "tech.h"
#include "terrain.h"
#include "tile.h"
#include "unit.h"
#include "unittype.h"

struct fc_lua;

typedef void (*luascript_log_func_t)(struct fc_lua *fcl, QtMsgType level,
                                     const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

struct fc_lua {
  lua_State *state;

  luascript_log_func_t output_fct;
  // This is needed for server 'lua' and 'luafile' commands.
  struct connection *caller;

  QHash<QString, struct luascript_func *> funcs;

  std::mutex mu;

  QHash<QString, struct signal> signals_hash;
  QVector<QString> signal_names;
};

typedef enum direction8 Direction;

template <class T> struct ent_handle {
  int id;

  inline ent_handle(const T &t);
  inline ent_handle(int id) : id(id) {}
  inline ent_handle(const T *t) : ent_handle(*t) {}
  inline T &operator*() { return *get(); }
  inline const T &operator*() const { return *get(); }
  inline T *operator->() { return get(); }
  inline T *get() const;
};

template <>
inline ent_handle<unit>::ent_handle(const unit &u) : ent_handle(u.id)
{
}

unit *game_unit_by_number(int);
template <> inline unit *ent_handle<unit>::get() const
{
  return game_unit_by_number(id);
}

template <>
inline ent_handle<city>::ent_handle(const city &c) : ent_handle(c.id)
{
}

city *idex_lookup_city(world *, int);
template <> inline city *ent_handle<city>::get() const
{
  return idex_lookup_city(&wld, id);
}

template <>
inline ent_handle<player>::ent_handle(const player &p)
    : ent_handle(player_number(&p))
{
}

player *player_by_number(const int);
template <> inline player *ent_handle<player>::get() const
{
  return player_by_number(id);
}

namespace sol {
template <class T> struct unique_usertype_traits<ent_handle<T>> {
  typedef T type;
  typedef ent_handle<T> actual_type;
  static const bool value = true;

  static bool is_null(const actual_type &ptr)
  {
    return ptr.get() == nullptr;
  }

  static type *get(const actual_type &ptr) { return ptr.get(); }
};

namespace stack {
template <> struct unqualified_pusher<::unit *> {
  inline int push(lua_State *L, const ::unit *u)
  {
    return stack::push(L, ent_handle(u));
  }
};

template <> struct unqualified_pusher<::city *> {
  inline int push(lua_State *L, const ::city *c)
  {
    return stack::push(L, ent_handle(c));
  }
};

template <> struct unqualified_pusher<::player *> {
  inline int push(lua_State *L, const ::player *p)
  {
    return stack::push(L, ent_handle(p));
  }
};

} // namespace stack

} // namespace sol
