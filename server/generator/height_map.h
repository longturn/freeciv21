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
#ifndef FC__HEIGHT_MAP_H
#define FC__HEIGHT_MAP_H



/* Wrappers for easy access.  They are a macros so they can be a lvalues.*/
#define hmap(_tile) (height_map[tile_index(_tile)])

/* shore_level safe unit of height */
#define H_UNIT MIN(1, (hmap_max_level - hmap_shore_level) / 100)

/*
 * Height map information
 *
 *   height_map[] stores the height of each tile
 *   hmap_max_level is the maximum height (heights will range from
 *     [0,hmap_max_level).
 *   hmap_shore_level is the level of ocean.  Any tile at this height or
 *     above is land; anything below is ocean.
 *   hmap_mount_level is the level of mountains and hills.  Any tile above
 *     this height will usually be a mountain or hill.
 */
#define hmap_max_level 1000
extern int *height_map;
extern int hmap_shore_level, hmap_mountain_level;

void normalize_hmap_poles(void);
void renormalize_hmap_poles(void);
void make_random_hmap(int smooth);
void make_pseudofractal1_hmap(int extra_div);

bool area_is_too_flat(struct tile *ptile, int thill, int my_height);



#endif /* FC__HEIGHT__MAP_H */
