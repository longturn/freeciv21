/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#define MG_UNUSED mapgen_terrain_property_invalid()

void generator_free();

void regenerate_lakes();
void smooth_water_depth();
void assign_continent_numbers();
int get_lake_surrounders(Continent_id cont);
int get_continent_size(Continent_id id);
int get_ocean_size(Continent_id id);

struct terrain *most_shallow_ocean(bool frozen);
struct terrain *pick_ocean(int depth, bool frozen);

struct terrain *pick_terrain_by_flag(enum terrain_flag_id flag);
struct terrain *pick_terrain(enum mapgen_terrain_property target,
                             enum mapgen_terrain_property prefer,
                             enum mapgen_terrain_property avoid);

/* Provide a block to convert from native to map coordinates.  For instance
 *   do_in_map_pos(mx, my, xn, yn) {
 *     tile_set_terrain(mx, my, T_OCEAN);
 *   } do_in_map_pos_end;
 * Note: that the map position is declared as const and can't be changed
 * inside the block.
 */
#define do_in_map_pos(nmap, ptile, nat_x, nat_y)                            \
  {                                                                         \
    struct tile *ptile = native_pos_to_tile(nmap, (nat_x), (nat_y));        \
    {

#define do_in_map_pos_end                                                   \
  }                                                                         \
  }

/***************************************************************************
 iterate on selected axe (x if is_X_axis is TRUE) over a interval of -dist
 to dist around the center_tile
 _index : the position in the interval of iteration (from -dist to dist)
 _tile : the tile pointer
 */
#define axis_iterate(nmap, center_tile, _tile, _index, dist, is_X_axis)     \
  {                                                                         \
    int _tile##_x, _tile##_y;                                               \
    struct tile *_tile;                                                     \
    const struct tile *_tile##_center = (center_tile);                      \
    const bool _index##_axis = (is_X_axis);                                 \
    const int _index##_d = (dist);                                          \
    int _index = -(_index##_d);                                             \
                                                                            \
    for (; _index <= _index##_d; _index++) {                                \
      int _nat##_x, _nat##_y;                                               \
      index_to_native_pos(&_nat##_x, &_nat##_y,                             \
                          tile_index(_tile##_center));                      \
      _tile##_x = _nat##_x + (_index##_axis ? _index : 0);                  \
      _tile##_y = _nat##_y + (_index##_axis ? 0 : _index);                  \
      _tile = native_pos_to_tile(nmap, _tile##_x, _tile##_y);               \
      if (nullptr != _tile) {

#define axis_iterate_end                                                    \
  }                                                                         \
  }                                                                         \
  }

/***************************************************************************
  pdata or pfilter can be nullptr!
***************************************************************************/
#define whole_map_iterate_filtered(_tile, pdata, pfilter)                   \
  {                                                                         \
    bool (*_tile##_filter)(const struct tile *vtile, const void *vdata) =   \
        (pfilter);                                                          \
    const void *_tile##_data = (pdata);                                     \
                                                                            \
    whole_map_iterate(&(wld.map), _tile)                                    \
    {                                                                       \
      if (nullptr == _tile##_filter                                         \
          || (_tile##_filter)(_tile, _tile##_data)) {

#define whole_map_iterate_filtered_end                                      \
  }                                                                         \
  }                                                                         \
  whole_map_iterate_end;                                                    \
  }

// int maps tools
void adjust_int_map_filtered(int *int_map, int int_map_max, void *data,
                             bool (*filter)(const struct tile *ptile,
                                            const void *data));
#define adjust_int_map(int_map, int_map_max)                                \
  adjust_int_map_filtered(                                                  \
      int_map, int_map_max, (void *) nullptr,                               \
      (bool (*)(const struct tile *ptile, const void *data)) nullptr)
void smooth_int_map(int *int_map, bool zeroes_at_edges);

// placed_map tool
void create_placed_map();
void destroy_placed_map();
void map_set_placed(struct tile *ptile);
void map_unset_placed(struct tile *ptile);
bool not_placed(const struct tile *ptile);
bool placed_map_is_initialized();
void set_all_ocean_tiles_placed();
void set_placed_near_pos(struct tile *ptile, int dist);
