// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// dependencies
extern "C" {
#include "lua.h"
}

// common
#include "luascript_types.h"

struct lua_State;

int api_effects_world_bonus(lua_State *L, const char *effect_type);
int api_effects_player_bonus(lua_State *L, Player *pplayer,
                             const char *effect_type);
int api_effects_city_bonus(lua_State *L, City *pcity,
                           const char *effect_type);
