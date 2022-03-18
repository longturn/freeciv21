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

// Sol
#include "sol/sol.hpp"

// utility
#include "fcintl.h"

// common
#include "effects.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_game_effects.h"

namespace {

/**
   Returns the effect bonus in the world
 */
int effects_world_bonus(const char *effect_type)
{
  enum effect_type etype = EFT_COUNT;

  etype = effect_type_by_name(effect_type, fc_strcasecmp);
  if (!effect_type_is_valid(etype)) {
    return 0;
  }
  return get_world_bonus(etype);
}

/**
   Returns the effect bonus for a player
 */
int effects_player_bonus(player *pplayer, const char *effect_type)
{
  enum effect_type etype = EFT_COUNT;

  etype = effect_type_by_name(effect_type, fc_strcasecmp);
  if (!effect_type_is_valid(etype)) {
    return 0;
  }
  return get_player_bonus(pplayer, etype);
}

/**
   Returns the effect bonus at a city.
 */
int effects_city_bonus(city *pcity, const char *effect_type)
{
  enum effect_type etype = EFT_COUNT;

  etype = effect_type_by_name(effect_type, fc_strcasecmp);
  if (!effect_type_is_valid(etype)) {
    return 0;
  }
  return get_city_bonus(pcity, etype);
}

} // namespace

/**
   Register game effects.
 */
void setup_game_effects(sol::state_view lua)
{
  auto effects = lua["effects"].get_or_create<sol::table>();

  effects.set("world_bonus", effects_world_bonus);
  effects.set("city_bonus", effects_city_bonus);
  effects.set("player_bonus", effects_player_bonus);
}
