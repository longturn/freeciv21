/*
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
 */

// common
#include "city.h"
#include "effects.h"
#include "game.h"
#include "player.h"

#include "culture.h"

/**
   Return current culture score of the city.
 */
int city_culture(const struct city *pcity)
{
  return pcity->history + get_city_bonus(pcity, EFT_PERFORMANCE);
}

/**
   How much history city gains this turn.
 */
int city_history_gain(const struct city *pcity)
{
  return get_city_bonus(pcity, EFT_HISTORY)
         + pcity->history * game.info.history_interest_pml / 1000;
}

/**
   Return current culture score of the player.
 */
int player_culture(const struct player *plr)
{
  int culture = plr->history + get_player_bonus(plr, EFT_NATION_PERFORMANCE);

  city_list_iterate(plr->cities, pcity) { culture += city_culture(pcity); }
  city_list_iterate_end;

  return culture;
}

/**
   How much nation-wide history player gains this turn. Does NOT include
   history gains of individual cities.
 */
int nation_history_gain(const struct player *pplayer)
{
  return get_player_bonus(pplayer, EFT_NATION_HISTORY)
         + pplayer->history * game.info.history_interest_pml / 1000;
}
