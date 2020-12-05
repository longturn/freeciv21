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
#ifndef FC__CITYMAP_H
#define FC__CITYMAP_H



/* utility */
#include "support.h" /* bool type */

/* common */
#include "fc_types.h"

void citymap_turn_init(struct player *pplayer);
void citymap_reserve_city_spot(struct tile *ptile, int id);
void citymap_free_city_spot(struct tile *ptile, int id);
void citymap_reserve_tile(struct tile *ptile, int id);
int citymap_read(struct tile *ptile);
bool citymap_is_reserved(struct tile *ptile);

void citymap_free(void);



#endif /* FC__CITYMAP_H */
