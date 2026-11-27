// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
#pragma once

// common
#include "city.h"

// server
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
#define POLLUTION_WEIGHTING 20 // tentative
#define INFRA_WEIGHTING (TRADE_WEIGHTING * 0.75)
#define WARMING_FACTOR 50
#define COOLING_FACTOR WARMING_FACTOR

struct player;

void building_advisor(struct player *pplayer);

void advisor_choose_build(struct player *pplayer, struct city *pcity);
void building_advisor_choose(struct city *pcity, struct adv_choice *choice);
