// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

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
