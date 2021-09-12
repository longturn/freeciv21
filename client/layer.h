/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 2021 Freeciv21 contributors.
\_   \        /  __/                        This file is part of Freeciv21.
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

// Forward declarations
class QPixmap;

struct city;
struct unit;
struct unit_type;

/* An edge is the border between two tiles.  This structure represents one
 * edge.  The tiles are given in the same order as the enumeration name. */
enum edge_type {
  EDGE_NS, // North and south
  EDGE_WE, // West and east
  EDGE_UD, /* Up and down (nw/se), for hex_width tilesets */
  EDGE_LR, /* Left and right (ne/sw), for hex_height tilesets */
  EDGE_COUNT
};

struct tile_edge {
  edge_type type;
#define NUM_EDGE_TILES 2
  const struct tile *tile[NUM_EDGE_TILES];
};

/* A corner is the endpoint of several edges.  At each corner 4 tiles will
 * meet (3 in hex view).  Tiles are in clockwise order NESW. */
struct tile_corner {
#define NUM_CORNER_TILES 4
  const struct tile *tile[NUM_CORNER_TILES];
};

struct drawn_sprite {
  bool foggable; // Set to FALSE for sprites that are never fogged.
  QPixmap *sprite;
  int offset_x, offset_y; // offset from tile origin
};

/* Items on the mapview are drawn in layers.  Each entry below represents
 * one layer.  The names are basically arbitrary and just correspond to
 * groups of elements in fill_sprite_array().  Callers of fill_sprite_array
 * must call it once for each layer. */
#define SPECENUM_NAME mapview_layer
#define SPECENUM_VALUE0 LAYER_BACKGROUND
#define SPECENUM_VALUE0NAME "Background"
// Adjust also TERRAIN_LAYER_COUNT if changing these
#define SPECENUM_VALUE1 LAYER_TERRAIN1
#define SPECENUM_VALUE1NAME "Terrain1"
#define SPECENUM_VALUE2 LAYER_DARKNESS
#define SPECENUM_VALUE2NAME "Darkness"
#define SPECENUM_VALUE3 LAYER_TERRAIN2
#define SPECENUM_VALUE3NAME "Terrain2"
#define SPECENUM_VALUE4 LAYER_TERRAIN3
#define SPECENUM_VALUE4NAME "Terrain3"
#define SPECENUM_VALUE5 LAYER_WATER
#define SPECENUM_VALUE5NAME "Water"
#define SPECENUM_VALUE6 LAYER_ROADS
#define SPECENUM_VALUE6NAME "Roads"
#define SPECENUM_VALUE7 LAYER_SPECIAL1
#define SPECENUM_VALUE7NAME "Special1"
#define SPECENUM_VALUE8 LAYER_GRID1
#define SPECENUM_VALUE8NAME "Grid1"
#define SPECENUM_VALUE9 LAYER_CITY1
#define SPECENUM_VALUE9NAME "City1"
#define SPECENUM_VALUE10 LAYER_SPECIAL2
#define SPECENUM_VALUE10NAME "Special2"
#define SPECENUM_VALUE11 LAYER_FOG
#define SPECENUM_VALUE11NAME "Fog"
#define SPECENUM_VALUE12 LAYER_UNIT
#define SPECENUM_VALUE12NAME "Unit"
#define SPECENUM_VALUE13 LAYER_SPECIAL3
#define SPECENUM_VALUE13NAME "Special3"
#define SPECENUM_VALUE14 LAYER_CITY2
#define SPECENUM_VALUE14NAME "City2"
#define SPECENUM_VALUE15 LAYER_GRID2
#define SPECENUM_VALUE15NAME "Grid2"
#define SPECENUM_VALUE16 LAYER_OVERLAYS
#define SPECENUM_VALUE16NAME "Overlays"
#define SPECENUM_VALUE17 LAYER_TILELABEL
#define SPECENUM_VALUE17NAME "TileLabel"
#define SPECENUM_VALUE18 LAYER_CITYBAR
#define SPECENUM_VALUE18NAME "CityBar"
#define SPECENUM_VALUE19 LAYER_FOCUS_UNIT
#define SPECENUM_VALUE19NAME "FocusUnit"
#define SPECENUM_VALUE20 LAYER_GOTO
#define SPECENUM_VALUE20NAME "Goto"
#define SPECENUM_VALUE21 LAYER_WORKERTASK
#define SPECENUM_VALUE21NAME "WorkerTask"
#define SPECENUM_VALUE22 LAYER_EDITOR
#define SPECENUM_VALUE22NAME "Editor"
#define SPECENUM_VALUE23 LAYER_INFRAWORK
#define SPECENUM_VALUE23NAME "InfraWork"
#define SPECENUM_COUNT LAYER_COUNT
#include "specenum_gen.h"

#define TERRAIN_LAYER_COUNT 3

#define mapview_layer_iterate(layer)                                        \
  {                                                                         \
    enum mapview_layer layer;                                               \
    int layer_index;                                                        \
                                                                            \
    for (layer_index = 0; layer_index < LAYER_COUNT; layer_index++) {       \
      layer = tileset_get_layer(tileset, layer_index);

#define mapview_layer_iterate_end                                           \
  }                                                                         \
  }

// Layer categories can be used to only render part of a tile.
enum layer_category {
  LAYER_CATEGORY_CITY, // Render cities
  LAYER_CATEGORY_TILE, // Render terrain only
  LAYER_CATEGORY_UNIT  // Render units only
};

struct tileset;

namespace freeciv {

/**
 * A layer when drawing the map.
 */
class layer {
public:
  layer(tileset *ts, mapview_layer layer) : m_ts(ts), m_layer(layer) {}

  virtual std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner, const unit *punit,
                    const city *pcity, const unit_type *putype) const;

private:
  tileset *m_ts;
  mapview_layer m_layer;
};

} // namespace freeciv
