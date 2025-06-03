// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "culture.h"

// common
#include "city.h"
#include "effects.h"
#include "fc_types.h"
#include "game.h"

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
