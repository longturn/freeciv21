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
