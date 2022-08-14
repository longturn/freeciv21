/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// common
#include "map.h"
#include "shared.h"

// client
#include "client_main.h"
#include "climap.h"
#include "tilespec.h" // tileset_is_isometric(tileset)

/**
   A tile's "known" field is used by the server to store whether _each_
   player knows the tile.  Client-side, it's used as an enum known_type
   to track whether the tile is known/fogged/unknown.

   Judicious use of this function also makes things very convenient for
   civworld, since it uses both client and server-style storage; since it
   uses the stock tilespec.c file, this function serves as a wrapper.
 */
enum known_type client_tile_get_known(const struct tile *ptile)
{
  if (nullptr == client.conn.playing) {
    if (client_is_observer()) {
      return TILE_KNOWN_SEEN;
    } else {
      return TILE_UNKNOWN;
    }
  }
  return tile_get_known(ptile, client.conn.playing);
}

/**
   Convert the given GUI direction into a map direction.

   GUI directions correspond to the current viewing interface, so that
   DIR8_NORTH is up on the mapview.  map directions correspond to the
   underlying map tiles, so that DIR8_NORTH means moving with a vector of
   (0,-1).  Neither necessarily corresponds to "north" on the underlying
   world (once iso-maps are possible).
 */
enum direction8 gui_to_map_dir(enum direction8 gui_dir)
{
  if (tileset_is_isometric(tileset)) {
    return dir_ccw(gui_dir);
  } else {
    return gui_dir;
  }
}

/**
   Client variant of city_tile().  This include the case of this could a
   ghost city (see client/packhand.c).  In a such case, the returned tile
   is an approximative position of the city on the map.
 */
struct tile *client_city_tile(const struct city *pcity)
{
  int dx, dy;
  double x = 0, y = 0;
  size_t num = 0;

  if (nullptr == pcity) {
    return nullptr;
  }

  if (nullptr != city_tile(pcity)) {
    // Normal city case.
    return city_tile(pcity);
  }

  whole_map_iterate(&(wld.map), ptile)
  {
    int tile_x, tile_y;

    index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
    if (pcity == tile_worked(ptile)) {
      if (0 == num) {
        x = tile_x;
        y = tile_y;
        num = 1;
      } else {
        num++;
        base_map_distance_vector(&dx, &dy, static_cast<int>(x),
                                 static_cast<int>(y), tile_x, tile_y);
        x += static_cast<double>(dx) / num;
        y += static_cast<double>(dy) / num;
      }
    }
  }
  whole_map_iterate_end;

  if (0 < num) {
    return map_pos_to_tile(&(wld.map), static_cast<int>(x),
                           static_cast<int>(y));
  } else {
    return nullptr;
  }
}

/**
   Returns TRUE when a tile is available to be worked, or the city itself
   is currently working the tile (and can continue).

   See also city_can_work_tile() (common/city.[ch]).
 */
bool client_city_can_work_tile(const struct city *pcity,
                               const struct tile *ptile)
{
  return base_city_can_work_tile(client_player(), pcity, ptile);
}
