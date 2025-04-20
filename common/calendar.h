// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

struct packet_game_info;
void game_next_year(struct packet_game_info *info);
void game_advance_year();

const char *textcalfrag(int frag);
const char *textyear(int year);
const char *calendar_text();
