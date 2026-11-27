// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

struct player_spaceship;

bool adv_spaceship_autoplace(struct player *pplayer,
                             struct player_spaceship *ship);
