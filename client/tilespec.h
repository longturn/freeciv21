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

#define NUM_TILES_PROGRESS 8
#define NUM_CORNER_DIRS 4

#define MAX_NUM_CITIZEN_SPRITES 6

enum arrow_type { ARROW_RIGHT, ARROW_PLUS, ARROW_MINUS, ARROW_LAST };

// This could be moved to common/map.h if there's more use for it.
enum direction4 { DIR4_NORTH = 0, DIR4_SOUTH, DIR4_EAST, DIR4_WEST };

constexpr direction8 DIR4_TO_DIR8[4] = {DIR8_NORTH, DIR8_SOUTH, DIR8_EAST,
                                        DIR8_WEST};

struct tileset_log_entry {
  QtMsgType level;
  QString message;
};

struct tileset;

extern struct tileset *tileset;
extern struct tileset *unscaled_tileset;

const QVector<QString> *get_tileset_list(const struct option *poption);

void tileset_error(struct tileset *t, QtMsgType level, const char *format,
                   ...);

void tileset_init(struct tileset *t);
void tileset_free(struct tileset *tileset);
void tileset_load_tiles(struct tileset *t);
void tileset_free_tiles(struct tileset *t);
void tileset_ruleset_reset(struct tileset *t);
bool tileset_is_fully_loaded();

std::vector<tileset_log_entry> tileset_log(const struct tileset *t);
bool tileset_has_error(const struct tileset *t);

void finish_loading_sprites(struct tileset *t);

bool tilespec_try_read(const char *tileset_name, bool verbose, int topo_id,
                       bool global_default);
bool tilespec_reread(const char *tileset_name, bool game_fully_initialized,
                     float scale, bool is_zoom = false);
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

// Layer order
const std::vector<std::unique_ptr<freeciv::layer>> &
tileset_get_layers(const struct tileset *t);
bool tileset_layer_in_category(enum mapview_layer layer,
                               enum layer_category cat);

// Gfx support
QPixmap *load_sprite(struct tileset *t, const QString &tag_name,
                     bool scale = true, bool smooth = true);

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
struct unit *get_drawable_unit(const struct tileset *t, const ::tile *ptile);
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

const QPixmap *get_spaceship_sprite(const struct tileset *t,
                                    enum spaceship_part part);
const QPixmap *get_citizen_sprite(const struct tileset *t,
                                  enum citizen_category type,
                                  int citizen_index,
                                  const struct city *pcity);
const QPixmap *get_city_flag_sprite(const struct tileset *t,
                                    const struct city *pcity);
void build_tile_data(const struct tile *ptile, struct terrain *pterrain,
                     struct terrain **tterrain_near,
                     bv_extras *textras_near);
const QPixmap *get_nation_flag_sprite(const struct tileset *t,
                                      const struct nation_type *nation);
const QPixmap *get_nation_shield_sprite(const struct tileset *t,
                                        const struct nation_type *nation);
const QPixmap *get_tech_sprite(const struct tileset *t, Tech_type_id tech);
const QPixmap *get_building_sprite(const struct tileset *t,
                                   const struct impr_type *pimprove);
const QPixmap *get_government_sprite(const struct tileset *t,
                                     const struct government *gov);
const QPixmap *get_unittype_sprite(const struct tileset *t,
                                   const struct unit_type *punittype,
                                   enum direction8 facing);
const QPixmap *get_sample_city_sprite(const struct tileset *t,
                                      int style_idx);
const QPixmap *get_tax_sprite(const struct tileset *t, Output_type_id otype);
const QPixmap *get_treaty_thumb_sprite(const struct tileset *t, bool on_off);
const struct sprite_vector *
get_unit_explode_animation(const struct tileset *t);
const QPixmap *get_nuke_explode_sprite(const struct tileset *t);
const QPixmap *get_cursor_sprite(const struct tileset *t,
                                 enum cursor_type cursor, int *hot_x,
                                 int *hot_y, int frame);
const struct citybar_sprites *get_citybar_sprites(const struct tileset *t);
const struct editor_sprites *get_editor_sprites(const struct tileset *t);
const QPixmap *get_icon_sprite(const struct tileset *t, enum icon_type icon);
const QPixmap *get_attention_crosshair_sprite(const struct tileset *t);
const QPixmap *get_indicator_sprite(const struct tileset *t,
                                    enum indicator_type indicator,
                                    int index);
const QPixmap *get_unit_unhappy_sprite(const struct tileset *t,
                                       const struct unit *punit,
                                       int happy_cost);
const QPixmap *get_unit_upkeep_sprite(const struct tileset *t,
                                      Output_type_id otype,
                                      const struct unit *punit,
                                      const int *upkeep_cost);
const QPixmap *get_basic_fog_sprite(const struct tileset *t);
std::vector<drawn_sprite>
fill_basic_extra_sprite_array(const struct tileset *t,
                              const struct extra_type *pextra);
bool is_extra_drawing_enabled(struct extra_type *pextra);
const QPixmap *get_event_sprite(const struct tileset *t,
                                enum event_type event);
const QPixmap *get_dither_sprite(const struct tileset *t);
const QPixmap *get_mask_sprite(const struct tileset *t);

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

int tileset_num_cardinal_dirs(const struct tileset *t);
int tileset_num_index_cardinals(const struct tileset *t);
std::array<direction8, 8> tileset_cardinal_dirs(const struct tileset *t);
QString cardinal_index_str(const struct tileset *t, int idx);

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
