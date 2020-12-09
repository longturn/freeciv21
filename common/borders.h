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
#ifndef FC__BORDERS_H
#define FC__BORDERS_H


#include "fc_types.h"

bool is_border_source(struct tile *ptile);
int tile_border_source_radius_sq(struct tile *ptile);
int tile_border_source_strength(struct tile *ptile);
int tile_border_strength(struct tile *ptile, struct tile *source);


#endif /* FC__BORDERS_H */
