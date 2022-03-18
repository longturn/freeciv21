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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "fcintl.h"

/* common/scriptcore */
#include "luascript.h"

// ai
#include "aitraits.h" // ai_trait_get_value()

/* server/scripting */
#include "api_server_base.h"
#include "script_server.h"

#include "api_server_game_methods.h"

namespace {

/**
   Return the current value of an AI trait in force (base+mod)
 */
int player_trait(sol::this_state s, player *pplayer, const char *tname)
{
  enum trait tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  return ai_trait_get_value(tr, pplayer);
}

/**
   Return the current base value of an AI trait (not including Lua mod)
 */
int player_trait_base(sol::this_state s, player *pplayer, const char *tname)
{
  enum trait tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  return pplayer->ai_common.traits[tr].val;
}

/**
   Return the current Lua increment to an AI trait
   (can be changed with api_edit_trait_mod_set())
 */
int player_trait_current_mod(sol::this_state s, player *pplayer,
                             const char *tname)
{
  enum trait tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  return pplayer->ai_common.traits[tr].mod;
}

/**
   Return the minimum random trait value that will be allocated for a nation
 */
int nation_trait_min(sol::this_state s, nation_type *pnation,
                     const char *tname)
{
  enum trait tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].min;
}

/**
   Return the maximum random trait value that will be allocated for a nation
 */
int nation_trait_max(sol::this_state s, nation_type *pnation,
                     const char *tname)
{
  enum trait tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].max;
}

/**
   Return the default trait value that will be allocated for a nation
 */
int nation_trait_default(sol::this_state s, nation_type *pnation,
                         const char *tname)
{
  enum trait tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  return pnation->server.traits[tr].fixed;
}

} // namespace

void setup_server_methods(sol::state_view lua)
{
  auto nation_type = lua["Nation_Type"].get_or_create<sol::table>();

  nation_type["trait_min"] = nation_trait_min;
  nation_type["trait_max"] = nation_trait_max;
  nation_type["trait_default"] = nation_trait_default;

  auto player = lua["Player"].get_or_create<sol::table>();
  player["trait"] = player_trait;
  player["trait_base"] = player_trait_base;
  player["trait_current_mod"] = player_trait_current_mod;

  setup_lua_server_base(lua);
}
