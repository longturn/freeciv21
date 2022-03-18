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

namespace {

/**
   Save the game (a manual save is triggered).
 */
bool server_save(const char *filename)
{
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
bool play_music(player *pplayer, const char *tag)
{
  struct packet_play_music p;

  sz_strlcpy(p.tag, tag);

  lsend_packet_play_music(pplayer->connections, &p);

  return true;
}

/**
   Return the formated value of the setting or nullptr if no such setting
 exists,
 */
const char *server_setting_get(const char *sett_name)
{
  struct setting *pset;
  static char buf[512];

  pset = setting_by_name(sett_name);

  if (!pset) {
    return nullptr;
  }

  return setting_value_name(pset, false, buf, sizeof(buf));
}

} // namespace

void setup_lua_server_base(sol::state_view lua)
{
  auto server = lua["server"].get_or_create<sol::table>();
  server.set("save", server_save);
  server.set("started", game_was_started);
  server.set("civilization_score", get_civ_score);
  server.set("play_music", play_music);

  auto settings = server["settings"].get_or_create<sol::table>();
  settings.set("get", server_setting_get);
}
