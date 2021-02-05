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

// utility
#include "support.h" // bool type

// common
#include "ai.h"
#include "city.h"
#include "fc_types.h"

struct ai_data;
struct ai_plr;
struct tile_data_cache;

void dai_auto_settler_init(struct ai_plr *ai);
void dai_auto_settler_free(struct ai_plr *ai);

void dai_auto_settler_reset(struct ai_type *ait, struct player *pplayer);
void dai_auto_settler_run(struct ai_type *ait, struct player *pplayer,
                          struct unit *punit, struct settlermap *state);
void dai_auto_settler_cont(struct ai_type *ait, struct player *pplayer,
                           struct unit *punit, struct settlermap *state);

void contemplate_new_city(struct ai_type *ait, struct city *pcity);
