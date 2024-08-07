/*__            ___                 ***************************************
/   \          /   \         Copyright (c) 2021-2023 Freeciv21 contributors.
\_   \        /  __/                         This file is part of Freeciv21.
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

#include "tileset/drawn_sprite.h"

// Forward declarations
class QPixmap;

struct city;
struct citystyle;
struct extra_type;
struct player;
struct terrain;
struct tileset;
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

#define SPECENUM_NAME extrastyle_id
#define SPECENUM_VALUE0 ESTYLE_ROAD_ALL_SEPARATE
#define SPECENUM_VALUE0NAME "RoadAllSeparate"
#define SPECENUM_VALUE1 ESTYLE_ROAD_PARITY_COMBINED
#define SPECENUM_VALUE1NAME "RoadParityCombined"
#define SPECENUM_VALUE2 ESTYLE_ROAD_ALL_COMBINED
#define SPECENUM_VALUE2NAME "RoadAllCombined"
#define SPECENUM_VALUE3 ESTYLE_RIVER
#define SPECENUM_VALUE3NAME "River"
#define SPECENUM_VALUE4 ESTYLE_SINGLE1
#define SPECENUM_VALUE4NAME "Single1"
#define SPECENUM_VALUE5 ESTYLE_SINGLE2
#define SPECENUM_VALUE5NAME "Single2"
#define SPECENUM_VALUE6 ESTYLE_3LAYER
#define SPECENUM_VALUE6NAME "3Layer"
#define SPECENUM_VALUE7 ESTYLE_CARDINALS
#define SPECENUM_VALUE7NAME "Cardinals"
#define SPECENUM_COUNT ESTYLE_COUNT
#include "specenum_gen.h"

// This the way directional indices are now encoded:
#define MAX_INDEX_CARDINAL 64
#define MAX_INDEX_HALF 16
#define MAX_INDEX_VALID 256

// Numbers on the map are shown in base 10
#define NUM_TILES_DIGITS 10

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
#define SPECENUM_VALUE14 LAYER_BASE_FLAGS
#define SPECENUM_VALUE14NAME "BaseFlags"
#define SPECENUM_VALUE15 LAYER_CITY2
#define SPECENUM_VALUE15NAME "City2"
#define SPECENUM_VALUE16 LAYER_GRID2
#define SPECENUM_VALUE16NAME "Grid2"
#define SPECENUM_VALUE17 LAYER_OVERLAYS
#define SPECENUM_VALUE17NAME "Overlays"
#define SPECENUM_VALUE18 LAYER_TILELABEL
#define SPECENUM_VALUE18NAME "TileLabel"
#define SPECENUM_VALUE19 LAYER_CITYBAR
#define SPECENUM_VALUE19NAME "CityBar"
#define SPECENUM_VALUE20 LAYER_FOCUS_UNIT
#define SPECENUM_VALUE20NAME "FocusUnit"
#define SPECENUM_VALUE21 LAYER_GOTO
#define SPECENUM_VALUE21NAME "Goto"
#define SPECENUM_VALUE22 LAYER_WORKERTASK
#define SPECENUM_VALUE22NAME "WorkerTask"
#define SPECENUM_VALUE23 LAYER_EDITOR
#define SPECENUM_VALUE23NAME "Editor"
#define SPECENUM_VALUE24 LAYER_INFRAWORK
#define SPECENUM_VALUE24NAME "InfraWork"
#define SPECENUM_COUNT LAYER_COUNT
#include "specenum_gen.h"

#define TERRAIN_LAYER_COUNT 3

// Layer categories can be used to only render part of a tile.
enum layer_category {
  LAYER_CATEGORY_CITY, // Render cities
  LAYER_CATEGORY_TILE, // Render terrain only
  LAYER_CATEGORY_UNIT  // Render units only
};

namespace freeciv {

/**
 * A layer when drawing the map.
 */
class layer {
public:
  /**
   * Constructor.
   */
  layer(struct tileset *ts, mapview_layer layer) : m_ts(ts), m_layer(layer)
  {
  }

  /**
   * Destructor.
   */
  virtual ~layer() = default;

  virtual std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner, const unit *punit) const;

  /**
   * Loads all sprites that do not depend on the ruleset.
   */
  virtual void load_sprites() {}

  /**
   * Initializes data specific to one player. This allows to cache tiles
   * depending on the actual players in a game.
   *
   * \see free_player
   */
  virtual void initialize_player(const player *player) { Q_UNUSED(player); }

  /**
   * Frees data initialized by initialize_player. Note that this takes the
   * player ID and not a pointer to the player; the player may have never
   * existed.
   *
   * \see initialize_player
   */
  virtual void free_player(int player_id) { Q_UNUSED(player_id); }

  /**
   * Initializes data for a city style.
   */
  virtual void initialize_city_style(const citystyle &style, int index)
  {
    Q_UNUSED(style);
    Q_UNUSED(index);
  }

  /**
   * Initializes extra-specific data. This is called after terrains have been
   * loaded, and only for extras with a graphic (graphic_str != none).
   */
  virtual void initialize_extra(const extra_type *extra, const QString &tag,
                                extrastyle_id style)
  {
    Q_UNUSED(extra);
    Q_UNUSED(tag);
    Q_UNUSED(style);
  }

  /**
   * Initializes terrain-specific data.
   */
  virtual void initialize_terrain(const terrain *terrain)
  {
    Q_UNUSED(terrain);
  }

  /**
   * Resets cached data that depends on the ruleset.
   */
  virtual void reset_ruleset() {}

  mapview_layer type() const { return m_layer; }

protected:
  struct tileset *tileset() const { return m_ts; }

  bool do_draw_unit(const tile *ptile, const unit *punit) const;
  bool solid_background(const tile *ptile, const unit *punit,
                        const city *pcity) const;

private:
  struct tileset *m_ts;
  mapview_layer m_layer;
};

} // namespace freeciv
