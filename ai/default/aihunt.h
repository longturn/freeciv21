/***********************************************************************
Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

// utility
#include "support.h" // bool type

// common
#include "fc_types.h"

void dai_hunter_choice(struct ai_type *ait, struct player *pplayer,
                       struct city *pcity, struct adv_choice *choice,
                       bool allow_gold_upkeep);
bool dai_hunter_qualify(struct player *pplayer, struct unit *punit);
int dai_hunter_manage(struct ai_type *ait, struct player *pplayer,
                      struct unit *punit);
