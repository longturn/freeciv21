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
#pragma once

/* common */
#include "fc_types.h"
#include "packets.h"

/* server */
#include "hand_gen.h"


struct player_spaceship;
struct conn_list;

void spaceship_calc_derived(struct player_spaceship *ship);
void send_spaceship_info(struct player *src, struct conn_list *dest);
void spaceship_arrived(struct player *pplayer);
void spaceship_lost(struct player *pplayer);
double spaceship_arrival(const struct player *pplayer);
int rank_spaceship_arrival(struct player **result);

bool do_spaceship_place(struct player *pplayer, enum action_requester from,
                        enum spaceship_place_type type, int num);


