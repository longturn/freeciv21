// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "luascript_types.h"

// dependencies/lua
extern "C" {
#include "lua.h"
}

/* Server-only methods added to the modules defined in
 * the common tolua_game.pkg. */

double api_methods_game_seconds_since_turn_change(lua_State *L);
double api_methods_game_seconds_to_turn_change(lua_State *L);

int api_methods_player_trait(lua_State *L, Player *pplayer,
                             const char *tname);
int api_methods_player_trait_base(lua_State *L, Player *pplayer,
                                  const char *tname);
int api_methods_player_trait_current_mod(lua_State *L, Player *pplayer,
                                         const char *tname);

int api_methods_nation_trait_min(lua_State *L, Nation_Type *pnation,
                                 const char *tname);
int api_methods_nation_trait_max(lua_State *L, Nation_Type *pnation,
                                 const char *tname);
int api_methods_nation_trait_default(lua_State *L, Nation_Type *pnation,
                                     const char *tname);

int api_methods_unit_birth_turn_get(lua_State *L, Unit *punit);
