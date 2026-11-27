// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "luascript_types.h"

// dependencies/lua
extern "C" {
#include "lua.h"
}

struct lua_State;

void api_notify_embassies_msg(lua_State *L, Player *pplayer, Tile *ptile,
                              int event, const char *message);
void api_notify_event_msg(lua_State *L, Player *pplayer, Tile *ptile,
                          int event, const char *message);
void api_notify_research_msg(lua_State *L, Player *pplayer, bool include_plr,
                             int event, const char *message);
void api_notify_research_embassies_msg(lua_State *L, Player *pplayer,
                                       int event, const char *message);
