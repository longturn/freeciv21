// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

struct player *player_leading_spacerace();
int player_distance_to_player(struct player *pplayer, struct player *target);
int city_gold_worth(struct city *pcity);
