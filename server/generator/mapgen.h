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

#include "fc_types.h"

#define HAS_POLES                                                           \
  (wld.map.server.temperature < 70 && !wld.map.server.alltemperate)

/**
   Conditions used mainly in rand_map_pos_characteristic()
 */
// WETNESS

// necessary condition of deserts placement
#define map_pos_is_dry(ptile)                                               \
  (map_colatitude((ptile)) <= DRY_MAX_LEVEL                                 \
   && map_colatitude((ptile)) > DRY_MIN_LEVEL                               \
   && count_terrain_class_near_tile((ptile), false, true, TC_OCEAN) <= 35)
typedef enum { WC_ALL = 200, WC_DRY, WC_NDRY } wetness_c;

extern int forest_pct;
extern int desert_pct;
extern int swamp_pct;
extern int mountain_pct;
extern int jungle_pct;
extern int river_pct;

extern struct extra_type *river_types[MAX_ROAD_TYPES];
extern int river_type_count;

void make_plains();
void make_polar();

bool test_wetness(const struct tile *ptile, wetness_c c);

bool map_fractal_generate(bool autosize, struct unit_type *initial_unit);
