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
#ifndef FC__FRACTURE_MAP_H
#define FC__FRACTURE_MAP_H



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
extern int *height_map;
extern int hmap_shore_level, hmap_mountain_level;

void make_fracture_map(void);
void make_fracture_relief(void);
#define MG_UNUSED mapgen_terrain_property_invalid()



#endif /* FC__FRACTURE__MAP_H */
