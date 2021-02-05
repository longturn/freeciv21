/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// common
#include "fc_types.h"

struct player_slot;
struct city;

void citizens_init(struct city *pcity);
void citizens_free(struct city *pcity);

citizens citizens_nation_get(const struct city *pcity,
                             const struct player_slot *pslot);
citizens citizens_nation_foreign(const struct city *pcity);
void citizens_nation_set(struct city *pcity, const struct player_slot *pslot,
                         citizens count);
void citizens_nation_add(struct city *pcity, const struct player_slot *pslot,
                         int add);
void citizens_nation_move(struct city *pcity,
                          const struct player_slot *pslot_from,
                          const struct player_slot *pslot_to, int move);

citizens citizens_count(const struct city *pcity);
struct player_slot *citizens_random(const struct city *pcity);

// Iterates over all player slots (nationalities) with citizens.
#define citizens_iterate(_pcity, _pslot, _nationality)                      \
  player_slots_iterate(_pslot)                                              \
  {                                                                         \
    citizens _nationality = citizens_nation_get(_pcity, _pslot);            \
    if (_nationality == 0) {                                                \
      continue;                                                             \
    }
#define citizens_iterate_end                                                \
  }                                                                         \
  player_slots_iterate_end;

// Like above but only foreign citizens.
#define citizens_foreign_iterate(_pcity, _pslot, _nationality)              \
  citizens_iterate(_pcity, _pslot, _nationality)                            \
  {                                                                         \
    if (_pslot == city_owner(_pcity)->slot) {                               \
      continue;                                                             \
    }
#define citizens_foreign_iterate_end                                        \
  }                                                                         \
  citizens_iterate_end;
