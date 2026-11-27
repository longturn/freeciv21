// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

/* Wrappers for easy access.  They are a macros so they can be a lvalues.*/
#define hmap(_tile) (height_map[tile_index(_tile)])

// shore_level safe unit of height
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

void normalize_hmap_poles();
void renormalize_hmap_poles();
void make_random_hmap();
void make_pseudofractal_hmap();

bool area_is_too_flat(struct tile *ptile, int thill, int my_height);
