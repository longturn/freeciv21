// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "aisupport.h"

// utility
#include "shared.h"

// common
#include "actions.h"
#include "city.h"
#include "fc_types.h"
#include "game.h"
#include "improvement.h"
#include "map.h"
#include "player.h"
#include "spaceship.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"
#include "victory.h"

// Qt
#include <QtMinMax>

/**
   Find who is leading the space race. Returns nullptr if nobody is.
 */
struct player *player_leading_spacerace()
{
  struct player *best = nullptr;
  int best_arrival = FC_INFINITY;
  enum spaceship_state best_state = SSHIP_NONE;

  if (!victory_enabled(VC_SPACERACE)) {
    return nullptr;
  }

  players_iterate_alive(pplayer)
  {
    struct player_spaceship *ship = &pplayer->spaceship;
    int arrival = static_cast<int>(ship->travel_time) + ship->launch_year;

    if (is_barbarian(pplayer) || ship->state == SSHIP_NONE) {
      continue;
    }

    if (ship->state != SSHIP_LAUNCHED && ship->state > best_state) {
      best_state = ship->state;
      best = pplayer;
    } else if (ship->state == SSHIP_LAUNCHED && arrival < best_arrival) {
      best_state = ship->state;
      best_arrival = arrival;
      best = pplayer;
    }
  }
  players_iterate_alive_end;

  return best;
}

/**
   Calculate average distances to other players. We calculate the
   average distance from all of our cities to the closest enemy city.
 */
int player_distance_to_player(struct player *pplayer, struct player *target)
{
  int cities = 0;
  int dists = 0;

  if (pplayer == target || !target->is_alive || !pplayer->is_alive
      || city_list_size(pplayer->cities) == 0
      || city_list_size(target->cities) == 0) {
    return 1;
  }

  // For all our cities, find the closest distance to an enemy city.
  city_list_iterate(pplayer->cities, pcity)
  {
    int min_dist = FC_INFINITY;

    city_list_iterate(target->cities, c2)
    {
      int dist = real_map_distance(c2->tile, pcity->tile);

      if (min_dist > dist) {
        min_dist = dist;
      }
    }
    city_list_iterate_end;
    dists += min_dist;
    cities++;
  }
  city_list_iterate_end;

  return MAX(dists / qMax(cities, 1), 1);
}

/**
   Rough calculation of the worth of pcity in gold.
 */
int city_gold_worth(struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  int worth = 0, i;
  struct unit_type *u = nullptr;

  if (!game.scenario.prevent_new_cities) {
    u = best_role_unit_for_player(city_owner(pcity),
                                  action_id_get_role(ACTION_FOUND_CITY));
  }

  if (u != nullptr) {
    worth += utype_buy_gold_cost(pcity, u, 0); // cost of settler
  }
  for (i = 1; i < city_size_get(pcity); i++) {
    worth += city_granary_size(i); // cost of growing city
  }
  output_type_iterate(o) { worth += pcity->prod[o] * 10; }
  output_type_iterate_end;
  unit_list_iterate(pcity->units_supported, punit)
  {
    if (same_pos(unit_tile(punit), pcity->tile)) {
      const struct unit_type *punittype = unit_type_get(punit)->obsoleted_by;

      if (punittype && can_city_build_unit_direct(pcity, punittype)) {
        // obsolete, candidate for disbanding
        worth += unit_shield_value(punit, unit_type_get(punit),
                                   action_by_number(ACTION_RECYCLE_UNIT));
      } else {
        worth += unit_build_shield_cost(pcity, punit); // good stuff
      }
    }
  }
  unit_list_iterate_end;
  city_built_iterate(pcity, pimprove)
  {
    if (improvement_obsolete(pplayer, pimprove, pcity)) {
      worth += impr_sell_gold(pimprove); // obsolete, candidate for selling
    } else if (!is_wonder(pimprove)) {
      // Buy cost, with nonzero shield amount
      worth += impr_build_shield_cost(pcity, pimprove) * 2;
    } else {
      worth += impr_build_shield_cost(pcity, pimprove) * 4;
    }
  }
  city_built_iterate_end;
  if (city_unhappy(pcity)) {
    worth *= 0.75;
  }
  return worth;
}
