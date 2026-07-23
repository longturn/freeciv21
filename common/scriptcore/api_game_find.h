// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// dependencies
extern "C" {
#include "lua.h"
}

// common
#include "fc_types.h"
#include "luascript_types.h"

// Object find module.
Player *api_find_player(lua_State *L, int player_id);
Player *api_find_player_by_name(lua_State *L, const char *player_name);

Team *api_find_team(lua_State *L, int team_id);
Team *api_find_team_by_name(lua_State *L, const char *team_name);

City *api_find_city(lua_State *L, Player *pplayer, int city_id);

Unit *api_find_unit(lua_State *L, Player *pplayer, int unit_id);
Unit *api_find_transport_unit(lua_State *L, Player *pplayer,
                              Unit_Type *ptype, Tile *ptile);
Tile *api_find_tile(lua_State *L, int nat_x, int nat_y);
Tile *api_find_tile_by_index(lua_State *L, int tindex);

Government *api_find_government(lua_State *L, int government_id);
Government *api_find_government_by_name(lua_State *L, const char *name_orig);

Nation_Type *api_find_nation_type(lua_State *L, int nation_type_id);
Nation_Type *api_find_nation_type_by_name(lua_State *L,
                                          const char *name_orig);
Action *api_find_action(lua_State *L, action_id act_id);
Action *api_find_action_by_name(lua_State *L, const char *name_orig);
Building_Type *api_find_building_type(lua_State *L, int building_type_id);
Building_Type *api_find_building_type_by_name(lua_State *L,
                                              const char *name_orig);
Unit_Type *api_find_unit_type(lua_State *L, int unit_type_id);
Unit_Type *api_find_unit_type_by_name(lua_State *L, const char *name_orig);
Unit_Type *api_find_role_unit_type(lua_State *L, const char *role_name,
                                   Player *pplayer);
Unit_Class *api_find_unit_class_by_name(lua_State *L, const char *name_orig);
Unit_Class *api_find_unit_class(lua_State *L, int unit_class_id);
Tech_Type *api_find_tech_type(lua_State *L, int tech_type_id);
Tech_Type *api_find_tech_type_by_name(lua_State *L, const char *name_orig);

Terrain *api_find_terrain(lua_State *L, int terrain_id);
Terrain *api_find_terrain_by_name(lua_State *L, const char *name_orig);

Nonexistent *api_find_nonexistent(lua_State *L);
