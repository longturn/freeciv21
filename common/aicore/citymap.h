// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

void citymap_turn_init(struct player *pplayer);
void citymap_reserve_city_spot(struct tile *ptile, int id);
void citymap_free_city_spot(struct tile *ptile, int id);
void citymap_reserve_tile(struct tile *ptile, int id);
int citymap_read(struct tile *ptile);
bool citymap_is_reserved(struct tile *ptile);
void citymap_free();
