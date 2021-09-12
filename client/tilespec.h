/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

/***********************************************************************
  Reading and using the tilespec files, which describe
  the files and contents of tilesets.
***********************************************************************/
#pragma once

#include "layer.h"
#include "options.h"

class QPixmap; // opaque; gui-dep

struct base_type;
struct resource_type;

// Create the sprite_vector type.
#define SPECVEC_TAG sprite
#define SPECVEC_TYPE QPixmap *
#include "specvec.h"
#define sprite_vector_iterate(sprite_vec, psprite)                          \
  TYPED_VECTOR_ITERATE(QPixmap *, sprite_vec, psprite)
#define sprite_vector_iterate_end VECTOR_ITERATE_END

#define SPECENUM_NAME ts_type
#define SPECENUM_VALUE0 TS_OVERHEAD
#define SPECENUM_VALUE0NAME N_("Overhead")
#define SPECENUM_VALUE1 TS_ISOMETRIC
#define SPECENUM_VALUE1NAME N_("Isometric")
#define SPECENUM_VALUE2 TS_3D
#define SPECENUM_VALUE2NAME N_("3D")
#include "specenum_gen.h"

#define SPECENUM_NAME fog_style
// Fog is automatically appended by the code.
#define SPECENUM_VALUE0 FOG_AUTO
#define SPECENUM_VALUE0NAME "Auto"
// A single fog sprite is provided by the tileset (tx.fog).
#define SPECENUM_VALUE1 FOG_SPRITE
#define SPECENUM_VALUE1NAME "Sprite"
// No fog, or fog derived from darkness style.
#define SPECENUM_VALUE2 FOG_DARKNESS
#define SPECENUM_VALUE2NAME "Darkness"
#include "specenum_gen.h"

#define SPECENUM_NAME darkness_style
// No darkness sprites are drawn.
#define SPECENUM_VALUE0 DARKNESS_NONE
#define SPECENUM_VALUE0NAME "None"
/* 1 sprite that is split into 4 parts and treated as a darkness4.  Only
 * works in iso-view. */
#define SPECENUM_VALUE1 DARKNESS_ISORECT
#define SPECENUM_VALUE1NAME "IsoRect"
/* 4 sprites, one per direction.  More than one sprite per tile may be
 * drawn. */
#define SPECENUM_VALUE2 DARKNESS_CARD_SINGLE
#define SPECENUM_VALUE2NAME "CardinalSingle"
/* 15=2^4-1 sprites.  A single sprite is drawn, chosen based on whether
 * there's darkness in _each_ of the cardinal directions. */
#define SPECENUM_VALUE3 DARKNESS_CARD_FULL
#define SPECENUM_VALUE3NAME "CardinalFull"
// Corner darkness & fog.  3^4 = 81 sprites.
#define SPECENUM_VALUE4 DARKNESS_CORNER
#define SPECENUM_VALUE4NAME "Corner"
#include "specenum_gen.h"

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
  enum edge_type type;
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

#define NUM_TILES_PROGRESS 8

#define MAX_NUM_CITIZEN_SPRITES 6

enum arrow_type { ARROW_RIGHT, ARROW_PLUS, ARROW_MINUS, ARROW_LAST };

struct tileset;

extern struct tileset *tileset;
extern struct tileset *unscaled_tileset;

const QVector<QString> *get_tileset_list(const struct option *poption);

void tileset_error(QtMsgType level, const char *format, ...);

void tileset_init(struct tileset *t);
void tileset_free(struct tileset *tileset);
void tileset_load_tiles(struct tileset *t);
void tileset_free_tiles(struct tileset *t);
void tileset_ruleset_reset(struct tileset *t);
bool tileset_is_fully_loaded();

void finish_loading_sprites(struct tileset *t);

bool tilespec_try_read(const char *tileset_name, bool verbose, int topo_id,
                       bool global_default);
bool tilespec_reread(const char *tileset_name, bool game_fully_initialized,
                     float scale);
void tilespec_reread_callback(struct option *poption);
void tilespec_reread_frozen_refresh(const char *tname);

void tileset_setup_specialist_type(struct tileset *t, Specialist_type_id id);
void tileset_setup_unit_type(struct tileset *t, struct unit_type *punittype);
void tileset_setup_impr_type(struct tileset *t, struct impr_type *pimprove);
void tileset_setup_tech_type(struct tileset *t, struct advance *padvance);
void tileset_setup_tile_type(struct tileset *t,
                             const struct terrain *pterrain);
void tileset_setup_resource(struct tileset *t,
                            const struct resource_type *presource);
void tileset_setup_extra(struct tileset *t, struct extra_type *pextra);
void tileset_setup_government(struct tileset *t, struct government *gov);
void tileset_setup_nation_flag(struct tileset *t,
                               struct nation_type *nation);
void tileset_setup_city_tiles(struct tileset *t, int style);

void tileset_player_init(struct tileset *t, struct player *pplayer);
void tileset_background_init(struct tileset *t);
void tileset_background_free(struct tileset *t);

// Layer order

enum mapview_layer tileset_get_layer(const struct tileset *t, int n);
bool tileset_layer_in_category(enum mapview_layer layer,
                               enum layer_category cat);

// Gfx support
std::vector<drawn_sprite>
fill_sprite_array(struct tileset *t, enum mapview_layer layer,
                  const struct tile *ptile, const struct tile_edge *pedge,
                  const struct tile_corner *pcorner,
                  const struct unit *punit, const struct city *pcity,
                  const struct unit_type *putype);
std::vector<drawn_sprite>
fill_basic_terrain_layer_sprite_array(struct tileset *t, int layer,
                                      struct terrain *pterrain);

int get_focus_unit_toggle_timeout(const struct tileset *t);
void reset_focus_unit_state(struct tileset *t);
void focus_unit_in_combat(struct tileset *t);
void toggle_focus_unit_state(struct tileset *t);
struct unit *get_drawable_unit(const struct tileset *t, struct tile *ptile);
bool unit_drawn_with_city_outline(const struct unit *punit,
                                  bool check_focus);

enum cursor_type {
  CURSOR_GOTO,
  CURSOR_PATROL,
  CURSOR_PARADROP,
  CURSOR_NUKE,
  CURSOR_SELECT,
  CURSOR_INVALID,
  CURSOR_ATTACK,
  CURSOR_EDIT_PAINT,
  CURSOR_EDIT_ADD,
  CURSOR_WAIT,
  CURSOR_LAST,
  CURSOR_DEFAULT,
};

#define NUM_CURSOR_FRAMES 6

enum indicator_type {
  INDICATOR_BULB,
  INDICATOR_WARMING,
  INDICATOR_COOLING,
  INDICATOR_COUNT
};

enum icon_type { ICON_FREECIV, ICON_CITYDLG, ICON_COUNT };

enum spaceship_part {
  SPACESHIP_SOLAR_PANEL,
  SPACESHIP_LIFE_SUPPORT,
  SPACESHIP_HABITATION,
  SPACESHIP_STRUCTURAL,
  SPACESHIP_FUEL,
  SPACESHIP_PROPULSION,
  SPACESHIP_EXHAUST,
  SPACESHIP_COUNT
};

struct citybar_sprites {
  QPixmap *shields, *food, *trade, *occupied, *background;
  struct sprite_vector occupancy;
};

struct editor_sprites {
  QPixmap *erase, *brush, *copy, *paste, *copypaste, *startpos, *terrain,
      *terrain_resource, *terrain_special, *unit, *city, *vision, *territory,
      *properties, *road, *military_base;
};

#define NUM_WALL_TYPES 7

QPixmap *get_spaceship_sprite(const struct tileset *t,
                              enum spaceship_part part);
QPixmap *get_citizen_sprite(const struct tileset *t,
                            enum citizen_category type, int citizen_index,
                            const struct city *pcity);
QPixmap *get_city_flag_sprite(const struct tileset *t,
                              const struct city *pcity);
QPixmap *get_nation_flag_sprite(const struct tileset *t,
                                const struct nation_type *nation);
QPixmap *get_nation_shield_sprite(const struct tileset *t,
                                  const struct nation_type *nation);
QPixmap *get_tech_sprite(const struct tileset *t, Tech_type_id tech);
QPixmap *get_building_sprite(const struct tileset *t,
                             const struct impr_type *pimprove);
QPixmap *get_government_sprite(const struct tileset *t,
                               const struct government *gov);
QPixmap *get_unittype_sprite(const struct tileset *t,
                             const struct unit_type *punittype,
                             enum direction8 facing);
QPixmap *get_sample_city_sprite(const struct tileset *t, int style_idx);
QPixmap *get_tax_sprite(const struct tileset *t, Output_type_id otype);
QPixmap *get_treaty_thumb_sprite(const struct tileset *t, bool on_off);
const struct sprite_vector *
get_unit_explode_animation(const struct tileset *t);
QPixmap *get_nuke_explode_sprite(const struct tileset *t);
QPixmap *get_cursor_sprite(const struct tileset *t, enum cursor_type cursor,
                           int *hot_x, int *hot_y, int frame);
const struct citybar_sprites *get_citybar_sprites(const struct tileset *t);
const struct editor_sprites *get_editor_sprites(const struct tileset *t);
QPixmap *get_icon_sprite(const struct tileset *t, enum icon_type icon);
QPixmap *get_attention_crosshair_sprite(const struct tileset *t);
QPixmap *get_indicator_sprite(const struct tileset *t,
                              enum indicator_type indicator, int index);
QPixmap *get_unit_unhappy_sprite(const struct tileset *t,
                                 const struct unit *punit, int happy_cost);
QPixmap *get_unit_upkeep_sprite(const struct tileset *t,
                                Output_type_id otype,
                                const struct unit *punit,
                                const int *upkeep_cost);
QPixmap *get_basic_fog_sprite(const struct tileset *t);
std::vector<drawn_sprite>
fill_basic_extra_sprite_array(const struct tileset *t,
                              const struct extra_type *pextra);
QPixmap *get_event_sprite(const struct tileset *t, enum event_type event);

QPixmap *tiles_lookup_sprite_tag_alt(struct tileset *t, QtMsgType level,
                                     const char *tag, const char *alt,
                                     const char *what, const char *name,
                                     bool scale);

struct color_system;
struct color_system *get_color_system(const struct tileset *t);

// Tileset accessor functions.
struct tileset *get_tileset();
const char *tileset_basename(const struct tileset *t);
bool tileset_is_isometric(const struct tileset *t);
int tileset_hex_width(const struct tileset *t);
int tileset_hex_height(const struct tileset *t);
int tileset_tile_width(const struct tileset *t);
int tileset_tile_height(const struct tileset *t);
int tileset_full_tile_width(const struct tileset *t);
int tileset_full_tile_height(const struct tileset *t);
int tileset_unit_width(const struct tileset *t);
int tileset_unit_height(const struct tileset *t);
int tileset_unit_with_upkeep_height(const struct tileset *t);
int tileset_unit_with_small_upkeep_height(const struct tileset *t);
int tileset_unit_layout_offset_y(const struct tileset *t);
int tileset_unit_layout_small_offset_y(const struct tileset *t);
int tileset_small_sprite_width(const struct tileset *t);
int tileset_small_sprite_height(const struct tileset *t);
int tileset_citybar_offset_y(const struct tileset *t);
int tileset_tilelabel_offset_y(const struct tileset *t);
float tileset_scale(const struct tileset *t);
const char *tileset_main_intro_filename(const struct tileset *t);
int tileset_num_city_colors(const struct tileset *t);
bool tileset_use_hard_coded_fog(const struct tileset *t);

/* These are used as array index -> can't be changed freely to values
   bigger than size of those arrays. */
#define TS_TOPO_SQUARE 0
#define TS_TOPO_HEX 1
#define TS_TOPO_ISOHEX 2

const char *tileset_name_get(struct tileset *t);
const char *tileset_version(struct tileset *t);
const char *tileset_summary(struct tileset *t);
const char *tileset_description(struct tileset *t);
char *tileset_what_ruleset(struct tileset *t);
int tileset_topo_index(struct tileset *t);
