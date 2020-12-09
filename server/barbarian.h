/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#pragma once
/* utility */
#include "support.h" /* bool type */

/* common */
#include "fc_types.h"


#define MIN_UNREST_DIST 5
#define MAX_UNREST_DIST 8

#define UPRISE_CIV_SIZE 10

#define MAP_FACTOR 2000 /* adjust this to get a good uprising frequency */

#define BARBARIAN_MIN_LIFESPAN 5

bool unleash_barbarians(struct tile *ptile);
void summon_barbarians(void);
bool is_land_barbarian(struct player *pplayer);
bool is_sea_barbarian(struct player *pplayer);

struct player *create_barbarian_player(enum barbarian_type type);


