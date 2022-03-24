/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common/scriptcore */
#include "luascript.h"

// server
#include "score.h"
#include "settings.h"
#include "srv_main.h"

/* server/sqavegame */
#include "savemain.h"

/* server/scripting */
#include "script_server.h"

#include "api_server_base.h"

/**
   Return the civilization score (total) for player
 */
int api_server_player_civilization_score(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  return get_civ_score(pplayer);
}

/**
   Returns TRUE if the game was started.
 */
bool api_server_was_started(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, false);

  return game_was_started();
}

/**
   Save the game (a manual save is triggered).
 */
bool api_server_save(lua_State *L, const char *filename)
{
  LUASCRIPT_CHECK_STATE(L, false);

  // Limit the allowed characters in the filename.
  if (filename != nullptr && !is_safe_filename(filename)) {
    return false;
  }

  save_game(filename, "User request (Lua)", false);

  return true;
}

/**
   Play music track for player
 */
bool api_play_music(lua_State *L, Player *pplayer, const char *tag)
{
  struct packet_play_music p;

  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pplayer, false);
  LUASCRIPT_CHECK_ARG_NIL(L, tag, 3, API_TYPE_STRING, false);

  sz_strlcpy(p.tag, tag);

  lsend_packet_play_music(pplayer->connections, &p);

  return true;
}

/**
   Return the formated value of the setting or nullptr if no such setting
 exists,
 */
const char *api_server_setting_get(lua_State *L, const char *sett_name)
{
  struct setting *pset;
  static char buf[512];

  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_ARG_NIL(L, sett_name, 2, API_TYPE_STRING, nullptr);

  pset = setting_by_name(sett_name);

  if (!pset) {
    return nullptr;
  }

  return setting_value_name(pset, false, buf, sizeof(buf));
}
