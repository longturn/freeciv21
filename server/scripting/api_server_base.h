// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// dependencies/sol2
#include "sol/sol.hpp"

// dependencies/lua
extern "C" {
#include "lua.h"
}

// common
#include "luascript_types.h"

struct lua_State;

// Additional methods on the server.
int api_server_player_civilization_score(lua_State *L, Player *pplayer);

bool api_server_was_started(lua_State *L);

bool api_server_save(lua_State *L, const char *filename);

const char *api_server_setting_get(lua_State *L, const char *sett_name);

bool api_play_music(lua_State *L, Player *pplayer, const char *tag);

sol::object api_server_require(sol::this_state state, const char *file_path);
