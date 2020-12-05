/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__API_SERVER_BASE_H
#define FC__API_SERVER_BASE_H

/* common/scriptcore */
#include "luascript_types.h"



struct lua_State;

/* Additional methods on the server. */
int api_server_player_civilization_score(lua_State *L, Player *pplayer);

bool api_server_was_started(lua_State *L);

bool api_server_save(lua_State *L, const char *filename);

const char *api_server_setting_get(lua_State *L, const char *sett_name);

bool api_play_music(lua_State *L, Player *pplayer, const char *tag);



#endif /* FC__API_SERVER_BASE_H */
