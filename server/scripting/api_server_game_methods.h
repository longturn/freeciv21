/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__API_SERVER_GAME_METHODS_H
#define FC__API_SERVER_GAME_METHODS_H

/* common/scriptcore */
#include "luascript_types.h"

/* server/scripting */
#include "api_server_edit.h"


/* Server-only methods added to the modules defined in
 * the common tolua_game.pkg. */

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


#endif /* FC__API_SERVER_GAME_METHODS_H */
