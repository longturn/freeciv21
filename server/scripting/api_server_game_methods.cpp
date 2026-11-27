// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "api_server_game_methods.h"

// utility
#include "support.h"
#include "timing.h"

// common
#include "fc_types.h"
#include "game.h"
#include "luascript.h"
#include "luascript_types.h"

// dependencies/lua
extern "C" {
#include "lua.h"
}

// ai
#include "aitraits.h" // ai_trait_get_value()

// server
#include "srv_main.h"

/**
 * Return the number of seconds since the last turn change.
 */
double api_methods_game_seconds_since_turn_change(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK(L, current_turn_timeout() > 0,
                  "The timeout is currently disabled.", 0);

  LUASCRIPT_CHECK(L, server_state() == S_S_RUNNING,
                  "The game is currently not running.", 0);
  return timer_read_seconds(game.server.phase_timer)
         + game.server.additional_phase_seconds;
}

/**
 * Return the number of seconds until the next turn change.
 */
double api_methods_game_seconds_to_turn_change(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK(L, current_turn_timeout() > 0,
                  "The timeout is currently disabled.", 0);
  LUASCRIPT_CHECK(L, server_state() == S_S_RUNNING,
                  "The game is currently not running.", 0);
  return game.tinfo.seconds_to_phasedone
         - timer_read_seconds(game.server.phase_timer)
         - game.server.additional_phase_seconds;
}

/**
   Return the current value of an AI trait in force (base+mod)
 */
int api_methods_player_trait(lua_State *L, Player *pplayer,
                             const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return ai_trait_get_value(tr, pplayer);
}

/**
   Return the current base value of an AI trait (not including Lua mod)
 */
int api_methods_player_trait_base(lua_State *L, Player *pplayer,
                                  const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pplayer->ai_common.traits[tr].val;
}

/**
   Return the current Lua increment to an AI trait
   (can be changed with api_edit_trait_mod_set())
 */
int api_methods_player_trait_current_mod(lua_State *L, Player *pplayer,
                                         const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pplayer->ai_common.traits[tr].mod;
}

/**
   Return the minimum random trait value that will be allocated for a nation
 */
int api_methods_nation_trait_min(lua_State *L, Nation_Type *pnation,
                                 const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pnation, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].min;
}

/**
   Return the maximum random trait value that will be allocated for a nation
 */
int api_methods_nation_trait_max(lua_State *L, Nation_Type *pnation,
                                 const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pnation, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].max;
}

/**
   Return the default trait value that will be allocated for a nation
 */
int api_methods_nation_trait_default(lua_State *L, Nation_Type *pnation,
                                     const char *tname)
{
  enum trait tr;

  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pnation, -1);
  LUASCRIPT_CHECK_ARG_NIL(L, tname, 3, string, 0);

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(L, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].fixed;
}

/**
 * Return the turn number that the given Unit was created.
 */
int api_methods_unit_birth_turn_get(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, punit, -1);
  return punit->server.birth_turn;
}
