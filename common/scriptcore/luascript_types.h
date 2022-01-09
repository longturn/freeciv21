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

// Classes.
/* If a new class is defined, an entry should be added to the enum api_types
 * below and the class name should be added to the api_types list in
 * tolua_common_z.pkg. */
typedef struct player Player;
typedef struct player_ai Player_ai;
typedef struct city City;
typedef struct unit Unit;
typedef struct tile Tile;
typedef struct government Government;
typedef struct nation_type Nation_Type;
typedef struct impr_type Building_Type;
typedef struct unit_type Unit_Type;
typedef struct advance Tech_Type;
typedef struct terrain Terrain;
typedef struct connection Connection;
typedef enum direction8 Direction;
typedef struct disaster_type Disaster;
typedef struct achievement Achievement;
typedef struct action Action;

typedef void Nonexistent;

/* List Classes.
 * NOTE: These should not to be exposed since the pointers are not safe. They
 *       are only used by the API internally.
 *       Separate types makes use from lua type safe. */
typedef const struct unit_list_link Unit_List_Link;
typedef const struct city_list_link City_List_Link;

#define SPECENUM_NAME api_types
#define SPECENUM_VALUE0 API_TYPE_INT
#define SPECENUM_VALUE0NAME "Int"
#define SPECENUM_VALUE1 API_TYPE_BOOL
#define SPECENUM_VALUE1NAME "Bool"
#define SPECENUM_VALUE2 API_TYPE_STRING
#define SPECENUM_VALUE2NAME "String"
#define SPECENUM_VALUE3 API_TYPE_PLAYER
#define SPECENUM_VALUE3NAME "Player"
#define SPECENUM_VALUE4 API_TYPE_CITY
#define SPECENUM_VALUE4NAME "City"
#define SPECENUM_VALUE5 API_TYPE_UNIT
#define SPECENUM_VALUE5NAME "Unit"
#define SPECENUM_VALUE6 API_TYPE_TILE
#define SPECENUM_VALUE6NAME "Tile"
#define SPECENUM_VALUE7 API_TYPE_GOVERNMENT
#define SPECENUM_VALUE7NAME "Government"
#define SPECENUM_VALUE8 API_TYPE_BUILDING_TYPE
#define SPECENUM_VALUE8NAME "Building_Type"
#define SPECENUM_VALUE9 API_TYPE_NATION_TYPE
#define SPECENUM_VALUE9NAME "Nation_Type"
#define SPECENUM_VALUE10 API_TYPE_UNIT_TYPE
#define SPECENUM_VALUE10NAME "Unit_Type"
#define SPECENUM_VALUE11 API_TYPE_TECH_TYPE
#define SPECENUM_VALUE11NAME "Tech_Type"
#define SPECENUM_VALUE12 API_TYPE_TERRAIN
#define SPECENUM_VALUE12NAME "Terrain"
#define SPECENUM_VALUE13 API_TYPE_CONNECTION
#define SPECENUM_VALUE13NAME "Connection"
#define SPECENUM_VALUE14 API_TYPE_DIRECTION
#define SPECENUM_VALUE14NAME "Direction"
#define SPECENUM_VALUE15 API_TYPE_DISASTER
#define SPECENUM_VALUE15NAME "Disaster"
#define SPECENUM_VALUE16 API_TYPE_ACHIEVEMENT
#define SPECENUM_VALUE16NAME "Achievement"
#define SPECENUM_VALUE17 API_TYPE_ACTION
#define SPECENUM_VALUE17NAME "Action"
#include "specenum_gen.h"

// Define compatibility functions to translate between tolua and sol2. See:
// https://sol2.readthedocs.io/en/latest/api/stack.html#extension-points
#include "sol/forward.hpp"
#include "tolua.h"

#define DEFINE_SOL_TOLUA_COMPAT(TYPE_)                                      \
  inline TYPE_ *sol_lua_check(sol::types<TYPE_ *>, lua_State *L, int index, \
                              sol::stack::record &)                         \
  {                                                                         \
    return (TYPE_ *) tolua_tousertype(L, index, 0);                         \
  }                                                                         \
                                                                            \
  template <typename Handler>                                               \
  inline bool sol_lua_check(sol::types<TYPE_ *> expected, lua_State *L,     \
                            int index, Handler &&handler,                   \
                            sol::stack::record &)                           \
  {                                                                         \
    tolua_Error err;                                                        \
    if (tolua_isusertype(L, index, #TYPE_, 0, &err))                        \
      return true;                                                          \
    /* Use LUA_TUSERDATA instead of sol::type::userdata to avoid including  \
     * sol.hpp everywhere. */                                               \
    handler(L, index, (sol::type) LUA_TUSERDATA,                            \
            (sol::type) lua_type(L, index),                                 \
            "could not convert to " #TYPE_);                                \
    return false;                                                           \
  }                                                                         \
                                                                            \
  inline int sol_lua_push(sol::types<TYPE_ *>, lua_State *L, TYPE_ *object) \
  {                                                                         \
    tolua_pushusertype(L, (void *) object, #TYPE_);                         \
    return 1;                                                               \
  }

DEFINE_SOL_TOLUA_COMPAT(Connection)
DEFINE_SOL_TOLUA_COMPAT(Player)

#undef DEFINE_SOL_TOLUA_COMPAT
