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
#ifndef FC__BUILDINGADV_H
#define FC__BUILDINGADV_H

/* server/advisors */
#include "advchoice.h"

#define FOOD_WEIGHTING 30
#define SHIELD_WEIGHTING 17
#define TRADE_WEIGHTING 18
/* The Trade Weighting has to about as large as the Shield Weighting,
   otherwise the AI will build Barracks to create veterans in cities
   with only 1 shields production.
    8 is too low
   18 is too high
 */
#define POLLUTION_WEIGHTING 20 /* tentative */
#define INFRA_WEIGHTING (TRADE_WEIGHTING * 0.75)
#define WARMING_FACTOR 50
#define COOLING_FACTOR WARMING_FACTOR

struct player;

void building_advisor(struct player *pplayer);

void advisor_choose_build(struct player *pplayer, struct city *pcity);
void building_advisor_choose(struct city *pcity, struct adv_choice *choice);

#endif /* FC__BUILDINGADV_H */
