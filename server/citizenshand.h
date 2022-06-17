/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

#include "fc_types.h"

struct city;

#define MAX_CITY_NATIONALITIES MIN(MAX_NUM_PLAYER_SLOTS, MAX_CITY_SIZE)

struct citizens_reduction {
  struct player_slot *pslot;
  citizens change;
};

void citizens_update(struct city *pcity, struct player *plr);
void citizens_convert(struct city *pcity);
void citizens_convert_conquest(struct city *pcity);
struct player *citizens_unit_nationality(const struct city *pcity,
                                         int pop_cost,
                                         struct citizens_reduction *pchange);
void citizens_reduction_apply(struct city *pcity,
                              const struct citizens_reduction *pchange);

void citizens_print(const struct city *pcity);
