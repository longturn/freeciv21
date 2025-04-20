// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

struct city;
struct player;

int city_culture(const struct city *pcity);
int city_history_gain(const struct city *pcity);

int player_culture(const struct player *plr);
int nation_history_gain(const struct player *pplayer);
