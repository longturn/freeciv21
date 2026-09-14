/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// dependencies/sol2
#include "sol/sol.hpp"

// dependencies/lua
extern "C" {
#include "lua.h"
}

// utilities
#include "fcintl.h" // _
#include "shared.h" // is_safe_filepath, fileinfoname

// common
#include "game.h"
#include "packets_gen.h" // lsend_packet_*, packet_*
#include "support.h"     // sz_strlcpy

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h" // Player

// server
#include "score.h"
#include "settings.h"
#include "srv_main.h"

/* server/sqavegame */
#include "savemain.h"

// Qt
#include <QLatin1Char>
#include <QLatin1String>
#include <QString>
#include <Qt> // CaseInsensitive

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

/**
 * Like 'require' but is restricted to '.lua' files and to the current
 * rulesetdir, and the 'lua' directory in the 'data' path.
 */
sol::object api_server_require(sol::this_state s, const char *file_path)
{
  LUASCRIPT_CHECK_STATE(s, sol::nil);
  LUASCRIPT_CHECK_ARG_NIL(s, file_path, 2, string, sol::nil);
  const QLatin1String lua_ext(".lua");
  QString relative_path = QString::fromUtf8(file_path);
  if (relative_path.endsWith(lua_ext, Qt::CaseInsensitive)) {
    relative_path.chop(4);
  }
  relative_path.replace(QLatin1Char('.'), QLatin1Char('/'));
  relative_path += lua_ext;
  if (!is_safe_filepath(relative_path)) {
    luascript_error(
        s, _("Freeciv21 script '%s' disallowed for security reasons."),
        relative_path.toLocal8Bit().constData());
    return sol::nil;
  }
  QString absolute_path = fileinfoname(
      get_data_dirs(),
      game.server.rulesetdir + QLatin1String("/") + relative_path);
  if (absolute_path.isEmpty()) {
    absolute_path =
        fileinfoname(get_data_dirs(), QLatin1String("lua/") + relative_path);
  }
  if (absolute_path.isEmpty()) {
    luascript_error(s, _("No Freeciv21 script found by the name '%s'."),
                    relative_path.toLocal8Bit().constData());
    return sol::nil;
  }
  return sol::state_view(s).require_file(relative_path.toStdString(),
                                         absolute_path.toStdString(), false);
}
