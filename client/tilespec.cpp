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
  Functions for handling the tilespec files which describe
  the files and contents of tilesets.
  original author: David Pfitzner <dwp@mso.anu.edu.au>
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <QHash>
#include <QImageReader>
#include <QPixmap>
#include <QSet>
#include <QString>
#include <QVector>
#include <cstdarg>
#include <cstdio>
#include <cstdlib> // exit
#include <cstring>

// utility
#include "bitvector.h"
#include "capability.h"
#include "deprecations.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "style.h"
#include "support.h"
#include "workertask.h"

// common
#include "base.h"
#include "effects.h"
#include "game.h" // game.control.styles_count
#include "government.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "player.h"
#include "road.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"

/* client/include */
#include "citydlg_g.h"
#include "dialogs_g.h"
#include "graphics_g.h"
#include "gui_main_g.h"
#include "mapview_g.h" // for update_map_canvas_visible
#include "menu_g.h"
#include "themes_g.h"

// client
#include "citybar.h"
#include "citydlg_common.h" // for generate_citydlg_dimensions()
#include "client_main.h"
#include "climap.h" // for client_tile_get_known()
#include "colors_common.h"
#include "control.h" // for fill_xxx
#include "editor.h"
#include "goto.h"
#include "helpdata.h"
#include "layer_background.h"
#include "layer_base_flags.h"
#include "layer_darkness.h"
#include "layer_special.h"
#include "layer_terrain.h"
#include "options.h" // for fill_xxx
#include "themes_common.h"

#include "tilespec.h"

#define TILESPEC_CAPSTR                                                     \
  "+Freeciv-tilespec-Devel-2019-Jul-03 duplicates_ok precise-hp-bars "      \
  "unlimited-unit-select-frames unlimited-upkeep-sprites"
/*
 * Tilespec capabilities acceptable to this program:
 *
 * +Freeciv-3.1-tilespec
 *    - basic format for Freeciv versions 3.1.x; required
 *
 * +Freeciv-tilespec-Devel-YYYY.MMM.DD
 *    - tilespec of the development version at the given data
 *
 * duplicates_ok
 *    - we can handle existence of duplicate tags (lattermost tag which
 *      appears is used; tilesets which have duplicates should specify
 *      "duplicates_ok")
 * precise-hp-bars
 *    - HP bars can use up to 100 sprites (unit.hp_*)
 * unlimited-unit-select-frames
 *    - The "selected unit" animation can have as many frames as is desired
 *      (unit.select*)
 * unlimited-upkeep-sprites
 *    - There is no limitation on the number of upkeep sprites
 *      (upkeep.unhappy*, upkeep.output*)
 */

#define SPEC_CAPSTR "+Freeciv-spec-Devel-2019-Jul-03"
/*
 * Individual spec file capabilities acceptable to this program:
 *
 * +Freeciv-3.1-spec
 *    - basic format for Freeciv versions 3.1.x; required
 */

#define TILESPEC_SUFFIX ".tilespec"
#define TILE_SECTION_PREFIX "tile_"

#define NUM_TILES_DIGITS 10

#define FULL_TILE_X_OFFSET ((t->normal_tile_width - t->full_tile_width) / 2)
#define FULL_TILE_Y_OFFSET (t->normal_tile_height - t->full_tile_height)

// This must correspond to enum edge_type.
static const char edge_name[EDGE_COUNT][3] = {"ns", "we", "ud", "lr"};

#define MAX_NUM_LAYERS 3

struct city_style_threshold {
  QPixmap *sprite;
};

struct styles {
  int land_num_thresholds;
  struct city_style_threshold *land_thresholds;
};

struct city_sprite {
  struct styles *styles;
  int num_styles;
};

struct river_sprites {
  QPixmap *spec[MAX_INDEX_CARDINAL], *outlet[MAX_INDEX_CARDINAL];
};

struct citizen_graphic {
  /* Each citizen type has up to MAX_NUM_CITIZEN_SPRITES different
   * sprites, as defined by the tileset. */
  int count;
  QPixmap *sprite[MAX_NUM_CITIZEN_SPRITES];
};

struct named_sprites {
  QPixmap *indicator[INDICATOR_COUNT][NUM_TILES_PROGRESS],
      *treaty_thumb[2],   // 0=disagree, 1=agree
      *arrow[ARROW_LAST], // 0=right arrow, 1=plus, 2=minus

      *icon[ICON_COUNT],

      *events[E_COUNT],

      // The panel sprites for showing tax % allocations.
      *tax_luxury, *tax_science, *tax_gold,
      *dither_tile; // only used for isometric view

  struct {
    QPixmap *tile, *worked_tile, *unworked_tile;
  } mask;

  const QPixmap *tech[A_LAST];
  const QPixmap *building[B_LAST];
  const QPixmap *government[G_LAST];

  struct {
    QPixmap *icon[U_LAST];
    QPixmap *facing[U_LAST][DIR8_MAGIC_MAX];
  } units;

  struct sprite_vector nation_flag;
  struct sprite_vector nation_shield;

  struct citizen_graphic citizen[CITIZEN_LAST], specialist[SP_MAX];
  QPixmap *spaceship[SPACESHIP_COUNT];
  struct {
    int hot_x, hot_y;
    QPixmap *frame[NUM_CURSOR_FRAMES];
  } cursor[CURSOR_LAST];
  struct {
    struct sprite_vector unit;
    QPixmap *nuke;
  } explode;
  struct {
    QPixmap *vet_lev[MAX_VET_LEVELS], *auto_attack, *auto_settler,
        *auto_explore, *fortified, *fortifying,
        *go_to, // goto is a C keyword :-)
        *irrigate, *plant, *pillage, *sentry, *stack, *loaded, *transform,
        *connect, *patrol, *convert, *battlegroup[MAX_NUM_BATTLEGROUPS],
        *action_decision_want, *lowfuel, *tired;
    std::vector<QPixmap *> hp_bar, select;
  } unit;
  struct {
    std::vector<QPixmap *> unhappy, output[O_LAST];
  } upkeep;
  struct {
    QPixmap *disorder, *size[NUM_TILES_DIGITS], *size_tens[NUM_TILES_DIGITS],
        *size_hundreds[NUM_TILES_DIGITS];
    std::vector<QPixmap> tile_foodnum, tile_shieldnum, tile_tradenum;
    struct city_sprite *tile, *single_wall, *wall[NUM_WALL_TYPES], *occupied;
    struct sprite_vector worked_tile_overlay;
    struct sprite_vector unworked_tile_overlay;
  } city;
  struct citybar_sprites citybar;
  struct editor_sprites editor;
  struct {
    struct {
      QPixmap *specific;
      QPixmap *turns[NUM_TILES_DIGITS];
      QPixmap *turns_tens[NUM_TILES_DIGITS];
      QPixmap *turns_hundreds[NUM_TILES_DIGITS];
    } s[GTS_COUNT];
    QPixmap *waypoint;
  } path;
  struct {
    QPixmap *attention;
  } user;
  struct {
    QPixmap *fog, **fullfog;
  } tx; // terrain extra
  struct {
    QPixmap *activity, *rmact;
    int extrastyle;
    union {
      QPixmap *cardinals[MAX_INDEX_CARDINAL];
      struct {
        QPixmap
            /* for extrastyles ESTYLE_ROAD_ALL_SEPARATE and
               ESTYLE_ROAD_PARITY_COMBINED */
            *isolated,
            *corner[8]; /* Indexed by direction; only non-cardinal dirs used.
                         */
        union {
          // for ESTYLE_ROAD_ALL_SEPARATE
          QPixmap *dir[8]; // all entries used
          // ESTYLE_ROAD_PARITY_COMBINED
          struct {
            QPixmap *even[MAX_INDEX_HALF], // first unused
                *odd[MAX_INDEX_HALF];      // first unused
          } combo;
          // ESTYLE_ALL_SEPARATE
          QPixmap *total[MAX_INDEX_VALID];
          struct river_sprites rivers;
        } ru;
      } road;
    } u;
  } extras[MAX_EXTRA_TYPES];
  struct {
    QPixmap *main[EDGE_COUNT], *city[EDGE_COUNT], *worked[EDGE_COUNT],
        *unavailable, *nonnative, *selected[EDGE_COUNT],
        *coastline[EDGE_COUNT], *borders[EDGE_COUNT][2];
  } grid;
  struct {
    struct sprite_vector overlays;
  } colors;
  struct {
    QPixmap *grid_borders[EDGE_COUNT][2];
    QPixmap *color;
  } player[MAX_NUM_PLAYER_SLOTS];
};

struct specfile {
  QPixmap *big_sprite;
  char *file_name;
};

/*
 * Information about an individual sprite. All fields except 'sprite' are
 * filled at the time of the scan of the specfile. 'Sprite' is
 * set/cleared on demand in load_sprite/unload_sprite.
 */
struct small_sprite {
  int ref_count;

  // The sprite is in this file.
  char *file;

  // Or, the sprite is in this file at the location.
  struct specfile *sf;
  int x, y, width, height;

  // A little more (optional) data.
  int hot_x, hot_y;

  QPixmap *sprite;
};

struct tileset {
  char name[512];
  char given_name[MAX_LEN_NAME];
  char version[MAX_LEN_NAME];
  int priority;

  char *summary;
  char *description;
  float scale;

  std::vector<tileset_log_entry> log;

  char *for_ruleset;

  std::vector<std::unique_ptr<freeciv::layer>> layers;
  struct {
    freeciv::layer_special *background, *middleground, *foreground;
  } special_layers;
  std::array<freeciv::layer_terrain *, MAX_NUM_LAYERS> terrain_layers;
  freeciv::layer_darkness *darkness_layer;

  enum ts_type type;
  int hex_width, hex_height;
  int ts_topo_idx;

  int normal_tile_width, normal_tile_height;
  int full_tile_width, full_tile_height;
  int unit_tile_width, unit_tile_height;
  int small_sprite_width, small_sprite_height;

  int max_upkeep_height;

  char *main_intro_filename;

  enum direction8 unit_default_orientation;

  enum fog_style fogstyle;

  int unit_flag_offset_x, unit_flag_offset_y;
  int city_flag_offset_x, city_flag_offset_y;
  int unit_offset_x, unit_offset_y;
  int city_offset_x, city_offset_y;
  int city_size_offset_x, city_size_offset_y;

  int citybar_offset_y;
  int tilelabel_offset_y;
  int activity_offset_x;
  int activity_offset_y;
  int select_offset_x;
  int select_offset_y;
  int select_step_ms = 100;
  int occupied_offset_x;
  int occupied_offset_y;
  int unit_upkeep_offset_y;
  int unit_upkeep_small_offset_y;

  int num_valid_tileset_dirs, num_cardinal_tileset_dirs;
  int num_index_valid, num_index_cardinal;
  std::array<direction8, 8> valid_tileset_dirs, cardinal_tileset_dirs;
  QSet<specfile *> *specfiles;
  QSet<struct small_sprite *> *small_sprites;
  // This hash table maps tilespec tags to struct small_sprites.
  QHash<QString, struct small_sprite *> *sprite_hash;
  QHash<QString, int> *estyle_hash;
  struct named_sprites sprites;
  struct color_system *color_system;
  struct extra_type_list *style_lists[ESTYLE_COUNT];

  int num_preferred_themes;
  char **preferred_themes;
};

struct tileset *tileset;
struct tileset *unscaled_tileset;

int focus_unit_state = 0;

static bool tileset_update = false;

static struct tileset *tileset_read_toplevel(const char *tileset_name,
                                             bool verbose, int topology_id,
                                             float scale);

static void fill_unit_type_sprite_array(const struct tileset *t,
                                        std::vector<drawn_sprite> &sprs,
                                        const struct unit_type *putype,
                                        enum direction8 facing);
static void fill_unit_sprite_array(const struct tileset *t,
                                   std::vector<drawn_sprite> &sprs,
                                   const struct unit *punit, bool stack,
                                   bool backdrop);
static bool load_river_sprites(struct tileset *t,
                               struct river_sprites *store,
                               const char *tag_pfx);

static void tileset_setup_base(struct tileset *t,
                               const struct extra_type *pextra,
                               const char *tag);
static void tileset_setup_road(struct tileset *t, struct extra_type *pextra,
                               const char *tag);

bool is_extra_drawing_enabled(struct extra_type *pextra);

static void tileset_player_free(struct tileset *t, int plrid);

/**
   Called when ever there's problem in ruleset/tileset compatibility
 */
void tileset_error(struct tileset *t, QtMsgType level, const char *format,
                   ...)
{
  va_list args;

  va_start(args, format);
  auto buf = QString::vasprintf(format, args);
  va_end(args);

  if (t != nullptr) {
    t->log.push_back(tileset_log_entry{level, buf});
  }

  log_base(level, "%s", qUtf8Printable(buf));

  if (level == QtFatalMsg) {
    exit(EXIT_FAILURE);
  }
}

/**
   Return unscaled tileset if it exists, or default otherwise
 */
struct tileset *get_tileset()
{
  if (unscaled_tileset != NULL) {
    return unscaled_tileset;
  } else {
    return tileset;
  }
}

/**
   Return the name of the given tileset.
 */
const char *tileset_basename(const struct tileset *t) { return t->name; }

/**
   Return whether the current tileset is isometric.
 */
bool tileset_is_isometric(const struct tileset *t)
{
  return t->type == TS_ISOMETRIC;
}

/**
   Return the hex_width of the current tileset. For iso-hex tilesets this
   value will be > 0 and is_isometric will be set.
 */
int tileset_hex_width(const struct tileset *t) { return t->hex_width; }

/**
   Return the hex_height of the current tileset. For hex tilesets this
   value will be > 0 and is_isometric will be set.
 */
int tileset_hex_height(const struct tileset *t) { return t->hex_height; }

/**
   Return the tile width of the current tileset.  This is the tesselation
   width of the tiled plane.  This means it's the width of the bounding box
   of the basic map tile.

   For best results:
     - The value should be even (or a multiple of 4 in iso-view).
     - In iso-view, the width should be twice the height (to give a
       perspective of 30 degrees above the horizon).
     - In non-iso-view, width and height should be equal (overhead
       perspective).
     - In hex or iso-hex view, remember this is the tesselation vector.
       hex_width and hex_height then give the size of the side of the
       hexagon.  Calculating the dimensions of a "regular" hexagon or
       iso-hexagon may be tricky.
   However these requirements are not absolute and callers should not
   depend on them (although some do).
 */
int tileset_tile_width(const struct tileset *t)
{
  return t->normal_tile_width;
}

/**
   Return the tile height of the current tileset.  This is the tesselation
   height of the tiled plane.  This means it's the height of the bounding box
   of the basic map tile.

   See also tileset_tile_width.
 */
int tileset_tile_height(const struct tileset *t)
{
  return t->normal_tile_height;
}

/**
   Return the full tile width of the current tileset.  This is the maximum
   width that any mapview sprite will have.

   Note: currently this is always equal to the tile width.
 */
int tileset_full_tile_width(const struct tileset *t)
{
  return t->full_tile_width;
}

/**
   Return the full tile height of the current tileset.  This is the maximum
   height that any mapview sprite will have.  This may be greater than the
   tile width in which case the extra area is above the "normal" tile.

   Some callers assume the full height is 50% larger than the height in
   iso-view, and equal in non-iso view.
 */
int tileset_full_tile_height(const struct tileset *t)
{
  return t->full_tile_height;
}

/**
   Return the unit tile width of the current tileset.
 */
int tileset_unit_width(const struct tileset *t)
{
  return t->unit_tile_width;
}

/**
   Return the unit tile height of the current tileset.
 */
int tileset_unit_height(const struct tileset *t)
{
  return t->unit_tile_height;
}

/**
   Calculate the height of a unit upkeep icons.
 */
static int calculate_max_upkeep_height(const struct tileset *t)
{
  int max = 0;

  for (const auto sprite : t->sprites.upkeep.unhappy) {
    max = std::max(max, sprite->height());
  }

  output_type_iterate(o)
  {
    for (const auto sprite : t->sprites.upkeep.output[o]) {
      max = std::max(max, sprite->height());
    }
  }
  output_type_iterate_end;

  return max;
}

/**
   Get the height of a unit upkeep icons.
 */
static int tileset_upkeep_height(const struct tileset *t)
{
  // Return cached value
  return t->max_upkeep_height;
}

/**
   Suitable canvas height for a unit icon that includes upkeep sprites.
 */
int tileset_unit_with_upkeep_height(const struct tileset *t)
{
  int uk_bottom =
      tileset_unit_layout_offset_y(tileset) + tileset_upkeep_height(tileset);
  int u_bottom = tileset_unit_height(tileset);

  return MAX(uk_bottom, u_bottom);
}

/**
   Suitable canvas height for a unit icon that includes upkeep sprites,
   using small space layout.
 */
int tileset_unit_with_small_upkeep_height(const struct tileset *t)
{
  Q_UNUSED(t)
  int uk_bottom = tileset_unit_layout_small_offset_y(tileset)
                  + tileset_upkeep_height(tileset);
  int u_bottom = tileset_unit_height(tileset);

  return MAX(uk_bottom, u_bottom);
}

/**
   Offset to layout extra unit sprites, such as upkeep.
 */
int tileset_unit_layout_offset_y(const struct tileset *t)
{
  return t->unit_upkeep_offset_y;
}

/**
   Offset to layout extra unit sprites, such as upkeep, requesting small
   space layout.
 */
int tileset_unit_layout_small_offset_y(const struct tileset *t)
{
  return t->unit_upkeep_small_offset_y;
}

/**
   Return the small sprite width of the current tileset.  The small sprites
   are used for various theme graphics (e.g., citymap citizens/specialists
   as well as panel indicator icons).
 */
int tileset_small_sprite_width(const struct tileset *t)
{
  return t->small_sprite_width;
}

/**
   Return the offset from the origin of the city tile at which to place the
   city bar text.
 */
int tileset_citybar_offset_y(const struct tileset *t)
{
  return t->citybar_offset_y;
}

/**
   Return the offset from the origin of the tile at which to place the
   label text.
 */
int tileset_tilelabel_offset_y(const struct tileset *t)
{
  return t->tilelabel_offset_y;
}

/**
   Returns tileset scale
 */
float tileset_scale(const struct tileset *t) { return tileset->scale; }

/**
   Return the small sprite height of the current tileset.  The small sprites
   are used for various theme graphics (e.g., citymap citizens/specialists
   as well as panel indicator icons).
 */
int tileset_small_sprite_height(const struct tileset *t)
{
  return t->small_sprite_height;
}

/**
   Return the path within the data directories where the main intro graphics
   file can be found.  (It is left up to the GUI code to load and unload this
   file.)
 */
const char *tileset_main_intro_filename(const struct tileset *t)
{
  return t->main_intro_filename;
}

/**
   Return the number of possible colors for city overlays.
 */
int tileset_num_city_colors(const struct tileset *t)
{
  return t->sprites.city.worked_tile_overlay.size;
}

/**
   Return TRUE if the client will use the code to generate the fog.
 */
bool tileset_use_hard_coded_fog(const struct tileset *t)
{
  return FOG_AUTO == t->fogstyle;
}

/**
 * @brief Returns the number of cardinal directions used by the tileset.
 * @see tileset_cardinal_dirs
 */
int tileset_num_cardinal_dirs(const struct tileset *t)
{
  return t->num_cardinal_tileset_dirs;
}

/**
 * @brief Returns the number of cardinal indices used by the tileset.
 *
 * This is `2^tileset_num_cardinal_dirs(t)`.
 */
int tileset_num_index_cardinals(const struct tileset *t)
{
  return t->num_index_cardinal;
}

/**
 * @brief Returns the cardinal directions used by the tileset.
 *
 * Only the first @ref tileset_num_cardinal_dirs items should be used.
 */
std::array<direction8, 8> tileset_cardinal_dirs(const struct tileset *t)
{
  return t->cardinal_tileset_dirs;
}

/**
   Initialize.
 */
static struct tileset *tileset_new()
{
  struct tileset *t = new struct tileset[1]();

  t->specfiles = new QSet<specfile *>;
  t->small_sprites = new QSet<struct small_sprite *>;
  return t;
}

/**
   Return the tileset name of the direction.  This is similar to
   dir_get_name but you shouldn't change this or all tilesets will break.
 */
static QString dir_get_tileset_name(enum direction8 dir)
{
  switch (dir) {
  case DIR8_NORTH:
    return QStringLiteral("n");
  case DIR8_NORTHEAST:
    return QStringLiteral("ne");
  case DIR8_EAST:
    return QStringLiteral("e");
  case DIR8_SOUTHEAST:
    return QStringLiteral("se");
  case DIR8_SOUTH:
    return QStringLiteral("s");
  case DIR8_SOUTHWEST:
    return QStringLiteral("sw");
  case DIR8_WEST:
    return QStringLiteral("w");
  case DIR8_NORTHWEST:
    return QStringLiteral("nw");
  }
  qCritical("Wrong direction8 variant: %d.", dir);
  return QLatin1String("");
}

/**
   Parse a direction name as a direction8.
 */
static enum direction8 dir_by_tileset_name(const QString &str)
{
  enum direction8 dir;

  for (dir = direction8_begin(); dir != direction8_end();
       dir = direction8_next(dir)) {
    if (dir_get_tileset_name(dir) == str) {
      return dir;
    }
  }

  return direction8_invalid();
}

/**
   Return TRUE iff the dir is valid in this tileset.
 */
static bool is_valid_tileset_dir(const struct tileset *t,
                                 enum direction8 dir)
{
  if (t->hex_width > 0) {
    return dir != DIR8_NORTHEAST && dir != DIR8_SOUTHWEST;
  } else if (t->hex_height > 0) {
    return dir != DIR8_NORTHWEST && dir != DIR8_SOUTHEAST;
  } else {
    return true;
  }
}

/**
   Return TRUE iff the dir is cardinal in this tileset.

   "Cardinal", in this sense, means that a tile will share a border with
   another tile in the direction rather than sharing just a single vertex.
 */
static bool is_cardinal_tileset_dir(const struct tileset *t,
                                    enum direction8 dir)
{
  if (t->hex_width > 0 || t->hex_height > 0) {
    return is_valid_tileset_dir(t, dir);
  } else {
    return (dir == DIR8_NORTH || dir == DIR8_EAST || dir == DIR8_SOUTH
            || dir == DIR8_WEST);
  }
}

/**
   Convert properties of the actual topology to an index of different
   tileset topology types.
 */
static int ts_topology_index(int actual_topology)
{
  int idx;

  if ((actual_topology & TF_HEX) && (actual_topology & TF_ISO)) {
    idx = TS_TOPO_ISOHEX;
  } else if (actual_topology & TF_ISO) {
    idx = TS_TOPO_SQUARE;
  } else if (actual_topology & TF_HEX) {
    idx = TS_TOPO_HEX;
  } else {
    idx = TS_TOPO_SQUARE;
  }

  return idx;
}

/**
   Returns a static list of tilesets available on the system by
   searching all data directories for files matching TILESPEC_SUFFIX.
 */
const QVector<QString> *get_tileset_list(const struct option *poption)
{
  static QVector<QString> *tilesets[3] = {NULL, NULL, NULL};
  int topo = option_get_cb_data(poption);
  int idx;

  idx = ts_topology_index(topo);

  fc_assert_ret_val(idx < ARRAY_SIZE(tilesets), NULL);

  if (tilesets[idx] == NULL) {
    /* Note: this means you must restart the client after installing a new
       tileset. */
    QVector<QString> *list = fileinfolist(get_data_dirs(), TILESPEC_SUFFIX);

    tilesets[idx] = new QVector<QString>;
    for (const auto &file : qAsConst(*list)) {
      struct tileset *t =
          tileset_read_toplevel(qUtf8Printable(file), false, topo, 1.0f);

      if (t) {
        tilesets[idx]->append(file);
        tileset_free(t);
      }
    }
    delete list;
  }

  return tilesets[idx];
}

/**
   Gets full filename for tilespec file, based on input name.
   Returned data is allocated, and freed by user as required.
   Input name may be null, in which case uses default.
   Falls back to default if can't find specified name;
   dies if can't find default.
 */
static char *tilespec_fullname(QString tileset_name)
{
  if (!tileset_name.isEmpty()) {
    QString dname;
    QString fname =
        QStringLiteral("%1%2").arg(tileset_name, TILESPEC_SUFFIX);

    dname = fileinfoname(get_data_dirs(), qUtf8Printable(fname));

    if (!dname.isEmpty()) {
      return fc_strdup(qUtf8Printable(dname));
    }
  }

  return NULL;
}

/**
   Checks options in filename match what we require and support.
   Die if not.
   'which' should be "tilespec" or "spec".
 */
static bool check_tilespec_capabilities(struct section_file *file,
                                        const char *which,
                                        const char *us_capstr,
                                        const char *filename, bool verbose)
{
  QtMsgType level = verbose ? LOG_ERROR : LOG_DEBUG;

  const char *file_capstr = secfile_lookup_str(file, "%s.options", which);

  if (NULL == file_capstr) {
    log_base(level, "\"%s\": %s file doesn't have a capability string",
             filename, which);
    return false;
  }
  if (!has_capabilities(us_capstr, file_capstr)) {
    log_base(level, "\"%s\": %s file appears incompatible:", filename,
             which);
    log_base(level, "  datafile options: %s", file_capstr);
    log_base(level, "  supported options: %s", us_capstr);
    return false;
  }
  if (!has_capabilities(file_capstr, us_capstr)) {
    log_base(level,
             "\"%s\": %s file requires option(s) "
             "that client doesn't support:",
             filename, which);
    log_base(level, "  datafile options: %s", file_capstr);
    log_base(level, "  supported options: %s", us_capstr);
    return false;
  }

  return true;
}

/**
   Frees the tilespec toplevel data, in preparation for re-reading it.

   See tilespec_read_toplevel().
 */
static void tileset_free_toplevel(struct tileset *t)
{
  int i;

  if (t->main_intro_filename) {
    FCPP_FREE(t->main_intro_filename);
  }

  if (t->preferred_themes) {
    for (i = 0; i < t->num_preferred_themes; i++) {
      delete[] t->preferred_themes[i];
    }
    delete[] t->preferred_themes;
    t->preferred_themes = NULL;
  }
  t->num_preferred_themes = 0;

  if (t->estyle_hash) {
    delete t->estyle_hash;
    t->estyle_hash = NULL;
  }
  for (i = 0; i < ESTYLE_COUNT; i++) {
    if (t->style_lists[i] != NULL) {
      extra_type_list_destroy(t->style_lists[i]);
      t->style_lists[i] = NULL;
    }
  }

  if (t->color_system) {
    color_system_free(t->color_system);
    t->color_system = NULL;
  }

  NFCNPP_FREE(t->summary);
  NFCNPP_FREE(t->description);
  NFCNPP_FREE(t->for_ruleset);
}

/**
   Clean up.
 */
void tileset_free(struct tileset *t)
{
  int i;

  tileset_free_tiles(t);
  tileset_free_toplevel(t);
  for (i = 0; i < ARRAY_SIZE(t->sprites.player); i++) {
    tileset_player_free(t, i);
  }
  delete t->specfiles;
  delete t->small_sprites;
  delete[] t;
}

/**
   Read a new tilespec in when first starting the game.

   Call this function with the (guessed) name of the tileset, when
   starting the client.

   Returns TRUE iff tileset with suggested tileset_name was loaded.
 */
bool tilespec_try_read(const char *tileset_name, bool verbose, int topo_id,
                       bool global_default)
{
  bool original;

  if (tileset_name == NULL
      || !(tileset = tileset_read_toplevel(tileset_name, verbose, topo_id,
                                           1.0f))) {
    QVector<QString> *list = fileinfolist(get_data_dirs(), TILESPEC_SUFFIX);

    original = false;
    for (const auto &file : qAsConst(*list)) {
      struct tileset *t =
          tileset_read_toplevel(qUtf8Printable(file), false, topo_id, 1.0f);

      if (t) {
        if (!tileset) {
          tileset = t;
        } else if (t->priority > tileset->priority
                   || (topo_id >= 0
                       && tileset_topo_index(tileset)
                              != tileset_topo_index(t))) {
          tileset_free(tileset);
          tileset = t;
        } else {
          tileset_free(t);
        }
      }
    }
    delete list;

    if (!tileset) {
      tileset_error(nullptr, LOG_FATAL,
                    _("No usable default tileset found, aborting!"));
    }

    qDebug("Trying tileset \"%s\".", tileset->name);
  } else {
    original = true;
  }
  option_set_default_ts(tileset);

  if (global_default) {
    sz_strlcpy(gui_options.default_tileset_name, tileset_basename(tileset));
  }

  return original;
}

/**
   Read a new tilespec in from scratch.

   Unlike the initial reading code, which reads pieces one at a time,
   this gets rid of the old data and reads in the new all at once.  If the
   new tileset fails to load the old tileset may be reloaded; otherwise the
   client will exit.  If a NULL name is given the current tileset will be
   reread.

   It will also call the necessary functions to redraw the graphics.

   Returns TRUE iff new tileset has been succesfully loaded.
 */
bool tilespec_reread(const char *new_tileset_name,
                     bool game_fully_initialized, float scale, bool is_zoom)
{
  int id;
  struct tile *center_tile;
  enum client_states state = client_state();
  const char *name = new_tileset_name ? new_tileset_name : tileset->name;
  bool new_tileset_in_use;

  // Make local copies since these values may be freed down below
  QString tileset_name = name;
  QString old_name = tileset->name;

  qInfo(_("Loading tileset \"%s\"."), qUtf8Printable(tileset_name));

  /* Step 0:  Record old data.
   *
   * We record the current mapcanvas center, etc.
   */
  center_tile = get_center_tile_mapcanvas();

  /* Step 1:  Cleanup.
   *
   * Free old tileset or keep it in memeory if we are loading the same
   * tileset with scaling and old one was not scaled.
   */

  if (tileset_name == old_name && tileset->scale == 1.0f && scale != 1.0f) {
    if (unscaled_tileset) {
      tileset_free(unscaled_tileset);
    }
    unscaled_tileset = tileset;
  } else {
    tileset_free(tileset);
  }

  /* Step 2:  Read.
   *
   * We read in the new tileset.  This should be pretty straightforward.
   */
  tileset =
      tileset_read_toplevel(qUtf8Printable(tileset_name), false, -1, scale);
  if (tileset != NULL) {
    new_tileset_in_use = true;
  } else {
    new_tileset_in_use = false;

    if (!(tileset = tileset_read_toplevel(qUtf8Printable(old_name), false,
                                          -1, scale))) {
      // Always fails.
      fc_assert_exit_msg(NULL != tileset,
                         "Failed to re-read the currently loaded tileset.");
    }
  }
  tileset_load_tiles(tileset);

  if (game_fully_initialized) {
    players_iterate(pplayer) { tileset_player_init(tileset, pplayer); }
    players_iterate_end;
    boot_help_texts(); // "About Current Tileset"
  }

  /* Step 3: Setup
   *
   * This is a seriously sticky problem.  On startup, we build a hash
   * from all the sprite data. Then, when we connect to a server, the
   * server sends us ruleset data a piece at a time and we use this data
   * to assemble the sprite structures.  But if we change while connected
   *  we have to reassemble all of these.  This should just involve
   * calling tilespec_setup_*** on everything.  But how do we tell what
   * "everything" is?
   *
   * The below code just does things straightforwardly, by setting up
   * each possible sprite again.  Hopefully it catches everything, and
   * doesn't mess up too badly if we change tilesets while not connected
   * to a server.
   */
  if (!game.client.ruleset_ready) {
    // The ruleset data is not sent until this point.
    return new_tileset_in_use;
  }

  if (tileset_map_topo_compatible(wld.map.topology_id, tileset)
      == TOPO_INCOMP_HARD) {
    tileset_error(tileset, LOG_NORMAL,
                  _("Map topology and tileset incompatible."));
  }

  terrain_type_iterate(pterrain)
  {
    tileset_setup_tile_type(tileset, pterrain);
  }
  terrain_type_iterate_end;
  unit_type_iterate(punittype)
  {
    tileset_setup_unit_type(tileset, punittype);
  }
  unit_type_iterate_end;
  governments_iterate(gov) { tileset_setup_government(tileset, gov); }
  governments_iterate_end;
  extra_type_iterate(pextra) { tileset_setup_extra(tileset, pextra); }
  extra_type_iterate_end;
  nations_iterate(pnation) { tileset_setup_nation_flag(tileset, pnation); }
  nations_iterate_end;
  improvement_iterate(pimprove)
  {
    tileset_setup_impr_type(tileset, pimprove);
  }
  improvement_iterate_end;
  advance_iterate(A_FIRST, padvance)
  {
    tileset_setup_tech_type(tileset, padvance);
  }
  advance_iterate_end;
  specialist_type_iterate(sp) { tileset_setup_specialist_type(tileset, sp); }
  specialist_type_iterate_end;

  for (id = 0; id < game.control.styles_count; id++) {
    tileset_setup_city_tiles(tileset, id);
  }

  if (is_zoom) {
    // Zooming in or out should not trigger the tileset error warning dialog.
    // This hides the fact that we reload the tileset completely when zomming
    // in or out. It's a hacky solution (the tileset might have changed), but
    // so is our zoom handling. To refresh the tileset, use the dedicated
    // shortcut.
    tileset->log.clear();
  }

  if (state < C_S_RUNNING) {
    // Below redraws do not apply before this.
    return new_tileset_in_use;
  }

  /* Step 4:  Draw.
   *
   * Do any necessary redraws.
   */
  generate_citydlg_dimensions();
  tileset_changed();
  can_slide = false;
  center_tile_mapcanvas(center_tile);
  /* update_map_canvas_visible forces a full redraw.  Otherwise with fast
   * drawing we might not get one.  Of course this is slower. */
  update_map_canvas_visible();
  can_slide = true;

  return new_tileset_in_use;
}

/**
   This is merely a wrapper for tilespec_reread (above) for use in
   options.c and the client local options dialog.
 */
void tilespec_reread_callback(struct option *poption)
{
  const char *tileset_name;
  enum client_states state = client_state();

  if ((state == C_S_RUNNING || state == C_S_OVER)
      && wld.map.topology_id & TF_HEX
      && option_get_cb_data(poption)
             != (wld.map.topology_id & (TF_ISO | TF_HEX))) {
    // Changed option was not for current topology
    return;
  }

  tileset_name = option_str_get(poption);

  /* As it's going to be 'current' tileset, make it global default if
   * options saved. */
  sz_strlcpy(gui_options.default_tileset_name, tileset_name);

  fc_assert_ret(NULL != tileset_name && tileset_name[0] != '\0');
  tileset_update = true;
  tilespec_reread(tileset_name, client.conn.established, 1.0f);
  tileset_update = false;
  menus_init();
}

/**
   Read a new tilespec in from scratch. Keep UI frozen while tileset is
   partially loaded; in inconsistent state.

   See tilespec_reread() for details.
 */
void tilespec_reread_frozen_refresh(const char *tname)
{
  tileset_update = true;
  tilespec_reread(tname, true, 1.0f);
  tileset_update = false;
  menus_init();
}

/**
   Loads the given graphics file (found in the data path) into a newly
   allocated sprite.
 */
static QPixmap *load_gfx_file(const char *gfx_filename)
{
  QPixmap *s;

  // Try out all supported file extensions to find one that works.
  auto supported = QImageReader::supportedImageFormats();

  // Make sure we try png first (it's the most common and Qt always supports
  // it). This dramatically improves tileset loading performance on Windows.
  supported.prepend("png");

  for (auto gfx_fileext : qAsConst(supported)) {
    QString real_full_name;
    QString full_name =
        QStringLiteral("%1.%2").arg(gfx_filename, gfx_fileext.data());

    real_full_name =
        fileinfoname(get_data_dirs(), qUtf8Printable(full_name));
    if (!real_full_name.isEmpty()) {
      log_debug("trying to load gfx file \"%s\".",
                qUtf8Printable(real_full_name));
      s = load_gfxfile(qUtf8Printable(real_full_name));
      if (s) {
        return s;
      }
    }
  }

  qCritical("Could not load gfx file \"%s\".", gfx_filename);
  QColor *c = color_alloc(255, 0, 0);
  return create_sprite(20, 20, c);
}

/**
   Ensure that the big sprite of the given spec file is loaded.
 */
static void ensure_big_sprite(struct tileset *t, struct specfile *sf)
{
  struct section_file *file;
  const char *gfx_filename;

  if (sf->big_sprite) {
    // Looks like it's already loaded.
    return;
  }

  /* Otherwise load it.  The big sprite will sometimes be freed and will have
   * to be reloaded, but most of the time it's just loaded once, the small
   * sprites are extracted, and then it's freed. */
  if (!(file = secfile_load(sf->file_name, true))) {
    tileset_error(t, LOG_FATAL, _("Could not open '%s':\n%s"), sf->file_name,
                  secfile_error());
  }

  if (!check_tilespec_capabilities(file, "spec", SPEC_CAPSTR, sf->file_name,
                                   true)) {
    tileset_error(t, LOG_FATAL, _("Incompatible tileset capabilities"));
  }

  gfx_filename = secfile_lookup_str(file, "file.gfx");

  sf->big_sprite = load_gfx_file(gfx_filename);

  if (!sf->big_sprite) {
    tileset_error(t, LOG_FATAL,
                  _("Could not load gfx file for the spec file \"%s\"."),
                  sf->file_name);
  }
  secfile_destroy(file);
}

/**
   Scan all sprites declared in the given specfile.  This means that the
   positions of the sprites in the big_sprite are saved in the
   small_sprite structs.
 */
static void scan_specfile(struct tileset *t, struct specfile *sf,
                          bool duplicates_ok)
{
  struct section_file *file;
  struct section_list *sections;
  int i;

  if (!(file = secfile_load(sf->file_name, true))) {
    tileset_error(t, LOG_FATAL, _("Could not open '%s':\n%s"), sf->file_name,
                  secfile_error());
  }
  if (!check_tilespec_capabilities(file, "spec", SPEC_CAPSTR, sf->file_name,
                                   true)) {
    tileset_error(t, LOG_FATAL,
                  _("Specfile %s has incompatible capabilities"),
                  sf->file_name);
  }

  // Currently unused
  (void) secfile_entry_lookup(file, "info.artists");

  // Not used here
  (void) secfile_entry_lookup(file, "file.gfx");

  if ((sections = secfile_sections_by_name_prefix(file, "grid_"))) {
    section_list_iterate(sections, psection)
    {
      int j, k;
      int x_top_left, y_top_left, dx, dy;
      int pixel_border_x;
      int pixel_border_y;
      const char *sec_name = section_name(psection);

      pixel_border_x =
          secfile_lookup_int_default(file, 0, "%s.pixel_border", sec_name);
      pixel_border_y = secfile_lookup_int_default(
          file, pixel_border_x, "%s.pixel_border_y", sec_name);
      pixel_border_x = secfile_lookup_int_default(
          file, pixel_border_x, "%s.pixel_border_x", sec_name);
      if (!secfile_lookup_int(file, &x_top_left, "%s.x_top_left", sec_name)
          || !secfile_lookup_int(file, &y_top_left, "%s.y_top_left",
                                 sec_name)
          || !secfile_lookup_int(file, &dx, "%s.dx", sec_name)
          || !secfile_lookup_int(file, &dy, "%s.dy", sec_name)) {
        qCritical("Grid \"%s\" invalid: %s", sec_name, secfile_error());
        continue;
      }

      j = -1;
      while (
          NULL
          != secfile_entry_lookup(file, "%s.tiles%d.tag", sec_name, ++j)) {
        struct small_sprite *ss;
        int row, column;
        int xr, yb;
        const char **tags;
        size_t num_tags;
        int hot_x, hot_y;

        if (!secfile_lookup_int(file, &row, "%s.tiles%d.row", sec_name, j)
            || !secfile_lookup_int(file, &column, "%s.tiles%d.column",
                                   sec_name, j)
            || !(tags = secfile_lookup_str_vec(
                     file, &num_tags, "%s.tiles%d.tag", sec_name, j))) {
          qCritical("Small sprite \"%s.tiles%d\" invalid: %s", sec_name, j,
                    secfile_error());
          continue;
        }
        hot_x = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_x",
                                           sec_name, j);
        hot_y = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_y",
                                           sec_name, j);

        // there must be at least 1 because of the while():
        fc_assert_action(num_tags > 0, continue);

        xr = x_top_left + (dx + pixel_border_x) * column;
        yb = y_top_left + (dy + pixel_border_y) * row;

        ss = new small_sprite;
        ss->ref_count = 0;
        ss->file = NULL;
        ss->x = xr;
        ss->y = yb;
        ss->width = dx;
        ss->height = dy;
        ss->sf = sf;
        ss->sprite = NULL;
        ss->hot_x = hot_x;
        ss->hot_y = hot_y;

        t->small_sprites->insert(ss);

        if (!duplicates_ok) {
          for (k = 0; k < num_tags; k++) {
            if (t->sprite_hash->contains(tags[k])) {
              qCritical("warning: already have a sprite for \"%s\".",
                        tags[k]);
            }
            t->sprite_hash->insert(tags[k], ss);
          }
        } else {
          for (k = 0; k < num_tags; k++) {
            t->sprite_hash->insert(tags[k], ss);
          }
        }

        delete[] tags;
        tags = NULL;
      }
    }
    section_list_iterate_end;
    section_list_destroy(sections);
  }

  // Load "extra" sprites.  Each sprite is one file.
  i = -1;
  while (NULL != secfile_entry_lookup(file, "extra.sprites%d.tag", ++i)) {
    struct small_sprite *ss;
    const char **tags;
    const char *filename;
    size_t num_tags, k;
    int hot_x, hot_y;

    if (!(tags = secfile_lookup_str_vec(file, &num_tags,
                                        "extra.sprites%d.tag", i))
        || !(filename =
                 secfile_lookup_str(file, "extra.sprites%d.file", i))) {
      qCritical("Extra sprite \"extra.sprites%d\" invalid: %s", i,
                secfile_error());
      continue;
    }
    hot_x = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_x", i);
    hot_y = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_y", i);

    ss = new small_sprite;
    ss->ref_count = 0;
    ss->file = fc_strdup(filename);
    ss->sf = NULL;
    ss->sprite = NULL;
    ss->hot_x = hot_x;
    ss->hot_y = hot_y;

    t->small_sprites->insert(ss);

    if (!duplicates_ok) {
      for (k = 0; k < num_tags; k++) {
        if (t->sprite_hash->contains(tags[k])) {
          qCritical("warning: already have a sprite for \"%s\".", tags[k]);
        }
        t->sprite_hash->insert(tags[k], ss);
      }
    } else {
      for (k = 0; k < num_tags; k++) {
        t->sprite_hash->insert(tags[k], ss);
      }
    }
    delete[] tags;
  }

  secfile_check_unused(file);
  secfile_destroy(file);
}

/**
   Returns the correct name of the gfx file (with path and extension)
   Must be free'd when no longer used
 */
static char *tilespec_gfx_filename(struct tileset *t,
                                   const char *gfx_filename)
{
  auto supported = QImageReader::supportedImageFormats();
  supported.prepend("png");
  for (auto gfx_current_fileext : supported) {
    QString real_full_name;
    QString full_name = QStringLiteral("%1.%2").arg(
        gfx_filename, gfx_current_fileext.data());

    real_full_name =
        fileinfoname(get_data_dirs(), qUtf8Printable(full_name));

    if (!real_full_name.isEmpty()) {
      return fc_strdup(qUtf8Printable(real_full_name));
    }
  }

  tileset_error(
      t, LOG_FATAL,
      _("Couldn't find a supported gfx file extension for \"%s\"."),
      gfx_filename);

  return NULL;
}

/**
   Determine the sprite_type string.
 */
static freeciv::layer_terrain::sprite_type
check_sprite_type(const char *sprite_type, const char *tile_section)
{
  if (fc_strcasecmp(sprite_type, "corner") == 0) {
    return freeciv::layer_terrain::CELL_CORNER;
  }
  if (fc_strcasecmp(sprite_type, "single") == 0) {
    return freeciv::layer_terrain::CELL_WHOLE;
  }
  if (fc_strcasecmp(sprite_type, "whole") == 0) {
    return freeciv::layer_terrain::CELL_WHOLE;
  }
  qCritical("[%s] unknown sprite_type \"%s\".", tile_section, sprite_type);
  return freeciv::layer_terrain::CELL_WHOLE;
}

static bool tileset_invalid_offsets(struct tileset *t,
                                    struct section_file *file)
{
  return !secfile_lookup_int(file, &t->unit_flag_offset_x,
                             "tilespec.unit_flag_offset_x")
         || !secfile_lookup_int(file, &t->unit_flag_offset_y,
                                "tilespec.unit_flag_offset_y")
         || !secfile_lookup_int(file, &t->city_flag_offset_x,
                                "tilespec.city_flag_offset_x")
         || !secfile_lookup_int(file, &t->city_flag_offset_y,
                                "tilespec.city_flag_offset_y")
         || !secfile_lookup_int(file, &t->unit_offset_x,
                                "tilespec.unit_offset_x")
         || !secfile_lookup_int(file, &t->unit_offset_y,
                                "tilespec.unit_offset_y")
         || !secfile_lookup_int(file, &t->activity_offset_x,
                                "tilespec.activity_offset_x")
         || !secfile_lookup_int(file, &t->activity_offset_y,
                                "tilespec.activity_offset_y")
         || !secfile_lookup_int(file, &t->select_offset_x,
                                "tilespec.select_offset_x")
         || !secfile_lookup_int(file, &t->select_offset_y,
                                "tilespec.select_offset_y")
         || !secfile_lookup_int(file, &t->city_offset_x,
                                "tilespec.city_offset_x")
         || !secfile_lookup_int(file, &t->city_offset_y,
                                "tilespec.city_offset_y")
         || !secfile_lookup_int(file, &t->city_size_offset_x,
                                "tilespec.city_size_offset_x")
         || !secfile_lookup_int(file, &t->city_size_offset_y,
                                "tilespec.city_size_offset_y")
         || !secfile_lookup_int(file, &t->citybar_offset_y,
                                "tilespec.citybar_offset_y")
         || !secfile_lookup_int(file, &t->tilelabel_offset_y,
                                "tilespec.tilelabel_offset_y")
         || !secfile_lookup_int(file, &t->occupied_offset_x,
                                "tilespec.occupied_offset_x")
         || !secfile_lookup_int(file, &t->occupied_offset_y,
                                "tilespec.occupied_offset_y");
}

static void tileset_set_offsets(struct tileset *t, struct section_file *file)
{
  t->unit_upkeep_offset_y = secfile_lookup_int_default(
      file, tileset_tile_height(t), "tilespec.unit_upkeep_offset_y");
  t->unit_upkeep_small_offset_y = secfile_lookup_int_default(
      file, t->unit_upkeep_offset_y, "tilespec.unit_upkeep_small_offset_y");
  t->city_size_offset_x = t->scale * t->city_size_offset_x;
  t->city_size_offset_y = t->scale * t->city_size_offset_y;
  t->select_offset_x = t->scale * t->select_offset_x;
  t->select_offset_y = t->scale * t->select_offset_y;
  t->unit_flag_offset_x = t->scale * t->unit_flag_offset_x;
  t->unit_flag_offset_y = t->scale * t->unit_flag_offset_y;
  t->city_flag_offset_x = t->scale * t->city_flag_offset_x;
  t->city_flag_offset_y = t->scale * t->city_flag_offset_y;
  t->unit_offset_x = t->scale * t->unit_offset_x;
  t->unit_offset_y = t->scale * t->unit_offset_y;
  t->activity_offset_x = t->scale * t->activity_offset_x;
  t->activity_offset_y = t->scale * t->activity_offset_y;
  t->city_offset_x = t->scale * t->city_offset_x;
  t->city_offset_y = t->scale * t->city_offset_y;
  t->citybar_offset_y = t->scale * t->citybar_offset_y;
  t->tilelabel_offset_y = t->scale * t->tilelabel_offset_y;
  t->occupied_offset_x = t->scale * t->occupied_offset_x;
  t->occupied_offset_y = t->scale * t->occupied_offset_y;
  if (t->scale != 1.0f
      && t->unit_upkeep_offset_y != tileset_tile_height(t)) {
    t->unit_upkeep_offset_y = t->scale * t->unit_upkeep_offset_y;
  }
  if (t->scale != 1.0f
      && t->unit_upkeep_small_offset_y != t->unit_upkeep_offset_y) {
    t->unit_upkeep_small_offset_y = t->scale * t->unit_upkeep_small_offset_y;
  }
}

static void tileset_stop_read(struct tileset *t, struct section_file *file,
                              char *fname, struct section_list *sections,
                              const char **layer_order)
{
  secfile_destroy(file);
  delete[] fname;
  NFCPP_FREE(layer_order);
  delete[] t;
  if (NULL != sections) {
    section_list_destroy(sections);
  }
}

/**
 * Creates a layer object for the given enumerated type and appends it to the
 * layers of `t`. Also fills layer pointers in `*t` if needed.
 */
static void tileset_add_layer(struct tileset *t, mapview_layer layer)
{
  switch (layer) {
  case LAYER_BACKGROUND:
    t->layers.push_back(std::make_unique<freeciv::layer_background>(t));
    break;
  case LAYER_DARKNESS: {
    auto l = std::make_unique<freeciv::layer_darkness>(t);
    t->darkness_layer = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_TERRAIN1: {
    auto l = std::make_unique<freeciv::layer_terrain>(t, 0);
    t->terrain_layers[0] = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_TERRAIN2: {
    auto l = std::make_unique<freeciv::layer_terrain>(t, 1);
    t->terrain_layers[1] = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_TERRAIN3: {
    auto l = std::make_unique<freeciv::layer_terrain>(t, 2);
    t->terrain_layers[2] = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_SPECIAL1: {
    auto l = std::make_unique<freeciv::layer_special>(t, layer);
    t->special_layers.background = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_SPECIAL2: {
    auto l = std::make_unique<freeciv::layer_special>(t, layer);
    t->special_layers.middleground = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_SPECIAL3: {
    auto l = std::make_unique<freeciv::layer_special>(t, layer);
    t->special_layers.foreground = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_BASE_FLAGS: {
    auto l = std::make_unique<freeciv::layer_base_flags>(
        t, FULL_TILE_X_OFFSET + t->city_flag_offset_x,
        FULL_TILE_Y_OFFSET + t->city_flag_offset_y);
    t->layers.emplace_back(std::move(l));
  } break;
  default:
    t->layers.push_back(std::make_unique<freeciv::layer>(t, layer));
    break;
  }
}

/**
   Finds and reads the toplevel tilespec file based on given name.
   Sets global variables, including tile sizes and full names for
   intro files.
   topology_id of -1 means any topology is acceptable.
 */
static struct tileset *tileset_read_toplevel(const char *tileset_name,
                                             bool verbose, int topology_id,
                                             float scale)
{
  struct section_file *file;
  char *fname;
  const char *c;
  int i;
  size_t num_spec_files;
  const char **spec_filenames;
  size_t num_layers;
  const char **layer_order = nullptr;
  size_t num_preferred_themes;
  struct section_list *sections = NULL;
  const char *file_capstr;
  bool duplicates_ok, is_hex;
  enum direction8 dir;
  struct tileset *t = NULL;
  const char *extraname;
  const char *tstr;
  int topo;

  fname = tilespec_fullname(tileset_name);
  if (!fname) {
    if (verbose) {
      qCritical("Can't find tileset \"%s\".", tileset_name);
    }
    return NULL;
  }
  qDebug("tilespec file is \"%s\".", fname);

  if (!(file = secfile_load(fname, true))) {
    qCritical("Could not open '%s':\n%s", fname, secfile_error());
    delete[] fname;
    return NULL;
  }

  if (!check_tilespec_capabilities(file, "tilespec", TILESPEC_CAPSTR, fname,
                                   verbose)) {
    secfile_destroy(file);
    delete[] fname;
    return NULL;
  }

  t = tileset_new();
  t->scale = scale;

  file_capstr = secfile_lookup_str(file, "%s.options", "tilespec");
  duplicates_ok = (NULL != file_capstr
                   && has_capabilities("+duplicates_ok", file_capstr));

  tstr = secfile_lookup_str(file, "tilespec.name");
  // Tileset name found
  sz_strlcpy(t->given_name, tstr);
  tstr = secfile_lookup_str_default(file, "", "tilespec.version");
  if (tstr[0] != '\0') {
    // Tileset version found
    sz_strlcpy(t->version, tstr);
  } else {
    // No version information
    t->version[0] = '\0';
  }

  tstr = secfile_lookup_str_default(file, "", "tilespec.summary");
  if (tstr[0] != '\0') {
    int len;

    // Tileset summary found
    len = qstrlen(tstr);
    t->summary = new char[len + 1];
    fc_strlcpy(t->summary, tstr, len + 1);
  } else {
    // No summary
    NFCNPP_FREE(t->summary);
  }

  tstr = secfile_lookup_str_default(file, "", "tilespec.description");
  if (tstr[0] != '\0') {
    int len;

    // Tileset description found
    len = qstrlen(tstr);
    t->description = new char[len + 1];
    fc_strlcpy(t->description, tstr, len + 1);
  } else {
    // No description
    if (t->description != NULL) {
      FCPP_FREE(t->description);
    }
  }

  tstr = secfile_lookup_str_default(file, NULL, "tilespec.for_ruleset");
  if (tstr != NULL) {
    t->for_ruleset = fc_strdup(tstr);
  } else {
    t->for_ruleset = NULL;
  }

  sz_strlcpy(t->name, tileset_name);
  if (!secfile_lookup_int(file, &t->priority, "tilespec.priority")
      || !secfile_lookup_bool(file, &is_hex, "tilespec.is_hex")) {
    qCritical("Tileset \"%s\" invalid: %s", t->name, secfile_error());
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  tstr = secfile_lookup_str(file, "tilespec.type");
  if (tstr == NULL) {
    qCritical("Tileset \"%s\": no tileset type", t->name);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  t->type = ts_type_by_name(tstr, fc_strcasecmp);
  if (!ts_type_is_valid(t->type)) {
    qCritical("Tileset \"%s\": unknown tileset type \"%s\"", t->name, tstr);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  topo = 0;
  if (t->type == TS_ISOMETRIC) {
    topo = TF_ISO;
  }

  // Read hex-tileset information.
  t->hex_width = t->hex_height = 0;
  if (is_hex) {
    int hex_side;

    if (!secfile_lookup_int(file, &hex_side, "tilespec.hex_side")) {
      qCritical("Tileset \"%s\" invalid: %s", t->name, secfile_error());
      tileset_stop_read(t, file, fname, sections, layer_order);
      return nullptr;
    }
    hex_side = hex_side * t->scale;
    if (t->type == TS_ISOMETRIC) {
      t->hex_width = hex_side;
    } else {
      t->hex_height = hex_side;
    }

    topo |= TF_HEX;

    // Hex tilesets are drawn the same as isometric.
    /* FIXME: There will be other legal values to be used with hex
     * tileset in the future, and this would just overwrite it. */
    t->type = TS_ISOMETRIC;
  }

  if (topology_id >= 0) {
    if (((topology_id & TF_HEX) && topology_id != (topo & (TF_ISO | TF_HEX)))
        || (!(topology_id & TF_HEX) && (topo & TF_HEX))) {
      // Not of requested topology
      tileset_stop_read(t, file, fname, sections, layer_order);
      return nullptr;
    }
  }

  t->ts_topo_idx = ts_topology_index(topo);

  if (!is_view_supported(t->type)) {
    // TRANS: "Overhead" or "Isometric"
    qInfo(_("Client does not support %s tilesets."),
          _(ts_type_name(t->type)));
    qInfo(_("Using default tileset instead."));
    fc_assert(tileset_name != NULL);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  tileset_type_set(t->type);

  /* Create arrays of valid and cardinal tileset dirs.  These depend
   * entirely on the tileset, not the topology.  They are also in clockwise
   * rotational ordering. */
  t->num_valid_tileset_dirs = t->num_cardinal_tileset_dirs = 0;
  dir = DIR8_NORTH;
  do {
    if (is_valid_tileset_dir(t, dir)) {
      t->valid_tileset_dirs[t->num_valid_tileset_dirs] = dir;
      t->num_valid_tileset_dirs++;
    }
    if (is_cardinal_tileset_dir(t, dir)) {
      t->cardinal_tileset_dirs[t->num_cardinal_tileset_dirs] = dir;
      t->num_cardinal_tileset_dirs++;
    }

    dir = dir_cw(dir);
  } while (dir != DIR8_NORTH);
  fc_assert(t->num_valid_tileset_dirs % 2 == 0); // Assumed elsewhere.
  t->num_index_valid = 1 << t->num_valid_tileset_dirs;
  t->num_index_cardinal = 1 << t->num_cardinal_tileset_dirs;

  if (!secfile_lookup_int(file, &t->normal_tile_width,
                          "tilespec.normal_tile_width")
      || !secfile_lookup_int(file, &t->normal_tile_height,
                             "tilespec.normal_tile_height")) {
    qCritical("Tileset \"%s\" invalid: %s", t->name, secfile_error());
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }
  t->normal_tile_width = ceil(t->scale * t->normal_tile_width);
  // Adjust width to be multiple of 8
  if (scale != 1.0f) {
    i = t->normal_tile_width;
    while (i % 8 != 0) {
      i++;
    }
    t->scale = (t->scale * i) / t->normal_tile_width;
    t->normal_tile_width = i;
  }
  t->normal_tile_height = ceil(t->scale * t->normal_tile_height);
  if (t->type == TS_ISOMETRIC) {
    t->full_tile_width = t->normal_tile_width;
    if (tileset_hex_height(t) > 0) {
      t->full_tile_height = t->normal_tile_height;
    } else {
      t->full_tile_height = 3 * t->normal_tile_height / 2;
    }
  } else {
    t->full_tile_width = t->normal_tile_width;
    t->full_tile_height = t->normal_tile_height;
  }
  t->unit_tile_width = secfile_lookup_int_default(file, t->full_tile_width,
                                                  "tilespec.unit_width");
  t->unit_tile_height = secfile_lookup_int_default(file, t->full_tile_height,
                                                   "tilespec.unit_height");
  if (!secfile_lookup_int(file, &t->small_sprite_width,
                          "tilespec.small_tile_width")
      || !secfile_lookup_int(file, &t->small_sprite_height,
                             "tilespec.small_tile_height")) {
    qCritical("Tileset \"%s\" invalid: %s", t->name, secfile_error());
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }
  if (t->unit_tile_width != t->full_tile_width && t->scale != 1.0f) {
    t->unit_tile_width = ceil(t->unit_tile_width * t->scale);
  }
  if (t->unit_tile_height != t->full_tile_height && t->scale != 1.0f) {
    t->unit_tile_height = ceil(t->unit_tile_height * t->scale);
  }
  t->small_sprite_width = t->small_sprite_width * t->scale;
  t->small_sprite_height = t->small_sprite_height * t->scale;
  qDebug("tile sizes %dx%d, %dx%d unit, %dx%d small", t->normal_tile_width,
         t->normal_tile_height, t->full_tile_width, t->full_tile_height,
         t->small_sprite_width, t->small_sprite_height);

  tstr = secfile_lookup_str(file, "tilespec.fog_style");
  if (tstr == NULL) {
    qCritical("Tileset \"%s\": no fog_style", t->name);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  t->fogstyle = fog_style_by_name(tstr, fc_strcasecmp);
  if (!fog_style_is_valid(t->fogstyle)) {
    qCritical("Tileset \"%s\": unknown fog_style \"%s\"", t->name, tstr);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  t->select_step_ms = secfile_lookup_int_def_min_max(
      file, 100, 1, 10000, "tilespec.select_step_ms");

  if (tileset_invalid_offsets(t, file)) {
    qCritical("Tileset \"%s\" invalid: %s", t->name, secfile_error());
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }
  tileset_set_offsets(t, file);

  c = secfile_lookup_str_default(file, NULL,
                                 "tilespec.unit_default_orientation");
  if (!c) {
    // This is valid, but tileset must specify icon for every unit
    t->unit_default_orientation = direction8_invalid();
  } else {
    dir = dir_by_tileset_name(c);

    if (!direction8_is_valid(dir)) {
      tileset_error(t, LOG_ERROR,
                    "Tileset \"%s\": unknown "
                    "unit_default_orientation \"%s\"",
                    t->name, c);
      tileset_stop_read(t, file, fname, sections, layer_order);
      return nullptr;
    } else {
      /* Default orientation is allowed to not be a valid one for the
       * tileset */
      t->unit_default_orientation = dir;
    }
  }

  c = secfile_lookup_str(file, "tilespec.main_intro_file");
  t->main_intro_filename = tilespec_gfx_filename(t, c);
  log_debug("intro file %s", t->main_intro_filename);

  // Layer order
  num_layers = 0;
  layer_order =
      secfile_lookup_str_vec(file, &num_layers, "tilespec.layer_order");
  if (layer_order != NULL) {
    mapview_layer order[LAYER_COUNT];
    for (i = 0; i < num_layers; i++) {
      int j;
      enum mapview_layer layer =
          mapview_layer_by_name(layer_order[i], fc_strcasecmp);

      // Check for wrong layer names.
      if (!mapview_layer_is_valid(layer)) {
        qCritical("layer_order: Invalid layer \"%s\"", layer_order[i]);
        tileset_stop_read(t, file, fname, sections, layer_order);
        return nullptr;
      }
      // Check for duplicates.
      for (j = 0; j < i; j++) {
        if (order[j] == layer) {
          qCritical("layer_order: Duplicate layer \"%s\"", layer_order[i]);
          tileset_stop_read(t, file, fname, sections, layer_order);
          return nullptr;
        }
      }
      order[i] = layer;
    }

    /* Now check that all layers are present. Doing it now allows for a
     * more comprehensive error message. */
    for (i = 0; i < LAYER_COUNT; i++) {
      int j;
      bool found = false;

      for (j = 0; j < num_layers; j++) {
        if (i == order[j]) {
          found = true;
          break;
        }
      }
      if (!found) {
        qCritical("layer_order: Missing layer \"%s\"",
                  mapview_layer_name(static_cast<mapview_layer>(i)));
        tileset_stop_read(t, file, fname, sections, layer_order);
        return nullptr;
      }
    }

    for (auto layer : order) {
      tileset_add_layer(t, layer);
    }
  } else {
    // There is no layer_order tag in the specfile -> use the default
    for (i = 0; i < LAYER_COUNT; ++i) {
      tileset_add_layer(t, static_cast<mapview_layer>(i));
    }
  }

  tstr = secfile_lookup_str(file, "tilespec.darkness_style");
  if (tstr == NULL) {
    qCritical("Tileset \"%s\": no darkness_style", t->name);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  auto darkness_style = freeciv::darkness_style_by_name(tstr, fc_strcasecmp);
  if (!darkness_style_is_valid(darkness_style)) {
    qCritical("Tileset \"%s\": unknown darkness_style \"%s\"", t->name,
              tstr);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  if (darkness_style == freeciv::DARKNESS_ISORECT
      && (t->type == TS_OVERHEAD || t->hex_width > 0 || t->hex_height > 0)) {
    qCritical("Invalid darkness style set in tileset \"%s\".", t->name);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  t->darkness_layer->set_darkness_style(darkness_style);

  // Terrain layer info.
  for (i = 0; i < MAX_NUM_LAYERS; i++) {
    std::size_t count = 0;
    auto match_types =
        secfile_lookup_str_vec(file, &count, "layer%d.match_types", i);
    for (int j = 0; j < count; j++) {
      if (!t->terrain_layers[i]->create_matching_group(match_types[j])) {
        tileset_stop_read(t, file, fname, sections, layer_order);
        return nullptr;
      }
    }
  }

  // Tile drawing info.
  sections = secfile_sections_by_name_prefix(file, TILE_SECTION_PREFIX);
  if (NULL == sections || 0 == section_list_size(sections)) {
    tileset_error(t, LOG_ERROR,
                  _("No [%s] sections supported by tileset \"%s\"."),
                  TILE_SECTION_PREFIX, fname);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  section_list_iterate(sections, psection)
  {
    auto sec_name = section_name(psection);

    QString tag;
    {
      auto c_tag = secfile_lookup_str(file, "%s.tag", sec_name);
      if (c_tag != NULL) {
        tag = c_tag;
      } else {
        tileset_error(t, LOG_ERROR,
                      _("No terrain tag given in section [%s]."), sec_name);
        tileset_stop_read(t, file, fname, sections, layer_order);
        return nullptr;
      }
    }

    {
      auto num_layers =
          secfile_lookup_int_default(file, 0, "%s.num_layers", sec_name);
      num_layers = CLIP(1, num_layers, MAX_NUM_LAYERS);

      for (int l = 0; l < num_layers; l++) {
        if (!t->terrain_layers[l]->add_tag(
                tag, QString(sec_name).mid(strlen(TILE_SECTION_PREFIX)))) {
          tileset_stop_read(t, file, fname, sections, layer_order);
          return nullptr;
        }

        // Offsets
        {
          auto is_tall = secfile_lookup_bool_default(
              file, false, "%s.layer%d_is_tall", sec_name, l);

          auto offset_x = secfile_lookup_int_default(
              file, 0, "%s.layer%d_offset_x", sec_name, l);
          offset_x = ceil(t->scale * offset_x);
          if (is_tall) {
            offset_x += FULL_TILE_X_OFFSET;
          }

          auto offset_y = secfile_lookup_int_default(
              file, 0, "%s.layer%d_offset_y", sec_name, l);
          offset_y = ceil(t->scale * offset_y);
          if (is_tall) {
            offset_y += FULL_TILE_Y_OFFSET;
          }

          if (!t->terrain_layers[l]->set_tag_offsets(tag, offset_x,
                                                     offset_y)) {
            tileset_stop_read(t, file, fname, sections, layer_order);
            return nullptr;
          }
        }

        // Sprite type
        {
          auto type_str = secfile_lookup_str_default(
              file, "whole", "%s.layer%d_sprite_type", sec_name, l);
          if (!t->terrain_layers[l]->set_tag_sprite_type(
                  tag, check_sprite_type(type_str, sec_name))) {
            tileset_stop_read(t, file, fname, sections, layer_order);
            return nullptr;
          }
        }

        // Matching
        {
          auto matching_group = secfile_lookup_str_default(
              file, NULL, "%s.layer%d_match_type", sec_name, l);

          if (matching_group) {
            if (!t->terrain_layers[l]->set_tag_matching_group(
                    tag, matching_group)) {
              tileset_stop_read(t, file, fname, sections, layer_order);
              return nullptr;
            }
          }

          std::size_t count = 0;
          auto match_with = secfile_lookup_str_vec(
              file, &count, "%s.layer%d_match_with", sec_name, l);
          if (match_with) {
            for (std::size_t j = 0; j < count; ++j) {
              if (!t->terrain_layers[l]->set_tag_matches_with(
                      tag, match_with[j])) {
                tileset_stop_read(t, file, fname, sections, layer_order);
                return nullptr;
              }
            }
          }
        }
      }
      {
        auto blending =
            secfile_lookup_int_default(file, 0, "%s.blend_layer", sec_name);
        if (blending > 0) {
          t->terrain_layers[CLIP(0, blending - 1, MAX_NUM_LAYERS - 1)]
              ->enable_blending(tag);
        }
      }
    }
  }
  section_list_iterate_end;
  section_list_destroy(sections);
  sections = NULL;

  t->estyle_hash = new QHash<QString, int>;

  for (i = 0; i < ESTYLE_COUNT; i++) {
    t->style_lists[i] = extra_type_list_new();
  }

  for (i = 0; (extraname = secfile_lookup_str_default(
                   file, NULL, "extras.styles%d.name", i));
       i++) {
    const char *style_name;
    int style;

    style_name = secfile_lookup_str_default(file, "Single1",
                                            "extras.styles%d.style", i);
    style = extrastyle_id_by_name(style_name, fc_strcasecmp);

    if (t->estyle_hash->contains(extraname)) {
      qCritical("warning: duplicate extrastyle entry [%s].", extraname);
      tileset_stop_read(t, file, fname, sections, layer_order);
      return nullptr;
    }
    t->estyle_hash->insert(extraname, style);
  }

  spec_filenames =
      secfile_lookup_str_vec(file, &num_spec_files, "tilespec.files");
  if (NULL == spec_filenames || 0 == num_spec_files) {
    qCritical("No tile graphics files specified in \"%s\"", fname);
    tileset_stop_read(t, file, fname, sections, layer_order);
    return nullptr;
  }

  fc_assert(t->sprite_hash == NULL);
  t->sprite_hash = new QHash<QString, struct small_sprite *>;
  for (i = 0; i < num_spec_files; i++) {
    struct specfile *sf = new specfile();
    QString dname;

    log_debug("spec file %s", spec_filenames[i]);

    sf->big_sprite = NULL;
    dname = fileinfoname(get_data_dirs(), spec_filenames[i]);
    if (dname.isEmpty()) {
      if (verbose) {
        qCritical("Can't find spec file \"%s\".", spec_filenames[i]);
      }
      delete sf;
      tileset_stop_read(t, file, fname, sections, layer_order);
      return nullptr;
    }
    sf->file_name = fc_strdup(qUtf8Printable(dname));
    scan_specfile(t, sf, duplicates_ok);

    t->specfiles->insert(sf);
  }
  delete[] spec_filenames;

  t->color_system = color_system_read(file);

  // FIXME: remove this hack.
  t->preferred_themes = const_cast<char **>(secfile_lookup_str_vec(
      file, &num_preferred_themes, "tilespec.preferred_themes"));
  if (num_preferred_themes <= 0) {
    t->preferred_themes = const_cast<char **>(secfile_lookup_str_vec(
        file, &num_preferred_themes, "tilespec.prefered_themes"));
    if (num_preferred_themes > 0) {
      qCWarning(deprecations_category,
                "Entry tilespec.prefered_themes in tilespec."
                " Use correct spelling tilespec.preferred_themes instead");
    }
  }
  t->num_preferred_themes = num_preferred_themes;
  for (i = 0; i < t->num_preferred_themes; i++) {
    t->preferred_themes[i] = fc_strdup(t->preferred_themes[i]);
  }

  secfile_check_unused(file);
  secfile_destroy(file);
  qDebug("finished reading \"%s\".", fname);
  delete[] fname;
  NFCPP_FREE(layer_order);

  return t;
}

/**
   Returns a text name for the citizen, as used in the tileset.
 */
static const char *citizen_rule_name(enum citizen_category citizen)
{
  /* These strings are used in reading the tileset.  Do not
   * translate. */
  switch (citizen) {
  case CITIZEN_HAPPY:
    return "happy";
  case CITIZEN_CONTENT:
    return "content";
  case CITIZEN_UNHAPPY:
    return "unhappy";
  case CITIZEN_ANGRY:
    return "angry";
  default:
    break;
  }
  qCritical("Unknown citizen type: %d.", static_cast<int>(citizen));
  return NULL;
}

/**
   Return a directional string for the cardinal directions.  Normally the
   binary value 1000 will be converted into "n1e0s0w0".  This is in a
   clockwise ordering.
 */
QString cardinal_index_str(const struct tileset *t, int idx)
{
  auto c = QString();
  for (int i = 0; i < t->num_cardinal_tileset_dirs; i++) {
    int value = (idx >> i) & 1;

    c += QStringLiteral("%1%2").arg(
        dir_get_tileset_name(t->cardinal_tileset_dirs[i]),
        QString::number(value));
  }

  return c;
}

/**
   Do the same thing as cardinal_str, except including all valid
 directions. The returned string is a pointer to static memory.
 */
static QString &valid_index_str(const struct tileset *t, int idx)
{
  static QString c;
  int i;

  c = QString();
  for (i = 0; i < t->num_valid_tileset_dirs; i++) {
    int value = (idx >> i) & 1;

    c += QStringLiteral("%1%2").arg(
        dir_get_tileset_name(t->valid_tileset_dirs[i]),
        QString::number(value));
  }

  return c;
}

/**
   Loads the sprite. If the sprite is already loaded a reference
   counter is increased. Can return NULL if the sprite couldn't be
   loaded.
   Scale means if sprite should be scaled, smooth if scaling might use
   other scaling algorithm than nearest neighbor.
 */
QPixmap *load_sprite(struct tileset *t, const QString &tag_name, bool scale,
                     bool smooth)
{
  struct small_sprite *ss;
  float sprite_scale = 1.0f;

  log_debug("load_sprite(tag='%s')", qUtf8Printable(tag_name));
  // Lookup information about where the sprite is found.
  if (!(ss = t->sprite_hash->value(tag_name, nullptr))) {
    return NULL;
  }

  fc_assert(ss->ref_count >= 0);

  if (!ss->sprite) {
    // If the sprite hasn't been loaded already, then load it.
    fc_assert(ss->ref_count == 0);
    if (ss->file) {
      int w, h;
      QPixmap *s;

      if (scale) {
        s = load_gfx_file(ss->file);
        get_sprite_dimensions(s, &w, &h);
        ss->sprite =
            crop_sprite(s, 0, 0, w, h, NULL, -1, -1, t->scale, smooth);
        free_sprite(s);
      } else {
        ss->sprite = load_gfx_file(ss->file);
      }
      if (!ss->sprite) {
        tileset_error(t, LOG_ERROR,
                      _("Couldn't load gfx file \"%s\" for sprite '%s'."),
                      ss->file, qUtf8Printable(tag_name));
        return nullptr;
      }
    } else {
      int sf_w, sf_h;

      ensure_big_sprite(t, ss->sf);
      get_sprite_dimensions(ss->sf->big_sprite, &sf_w, &sf_h);
      if (ss->x < 0 || ss->x + ss->width > sf_w || ss->y < 0
          || ss->y + ss->height > sf_h) {
        tileset_error(
            t, LOG_ERROR,
            _("Sprite '%s' in file \"%s\" isn't within the image!"),
            qUtf8Printable(tag_name), ss->sf->file_name);
        return NULL;
      }
      if (scale) {
        sprite_scale = t->scale;
      }
      ss->sprite =
          crop_sprite(ss->sf->big_sprite, ss->x, ss->y, ss->width,
                      ss->height, NULL, -1, -1, sprite_scale, smooth);
    }
  }

  // Track the reference count so we know when to free the sprite.
  ss->ref_count++;

  return ss->sprite;
}

/**
   Unloads the sprite. Decrease the reference counter. If the last
   reference is removed the sprite is freed.
 */
static void unload_sprite(struct tileset *t, const QString &tag_name)
{
  struct small_sprite *ss = t->sprite_hash->value(tag_name);
  fc_assert_ret(ss);
  fc_assert_ret(ss->ref_count >= 1);
  fc_assert_ret(ss->sprite);

  ss->ref_count--;

  if (ss->ref_count == 0) {
    /* Nobody's using the sprite anymore, so we should free it.  We know
     * where to find it if we need it again. */
    // log_debug("freeing sprite '%s'.", tag_name);
    free_sprite(ss->sprite);
    ss->sprite = NULL;
  }
}

// Not very safe, but convenient:
#define SET_SPRITE(field, tag)                                              \
  do {                                                                      \
    t->sprites.field = load_sprite(t, tag, true, true);                     \
    if (t->sprites.field == NULL) {                                         \
      tileset_error(t, LOG_FATAL, _("Sprite for tag '%s' missing."),        \
                    qUtf8Printable(tag));                                   \
    }                                                                       \
  } while (false)

#define SET_SPRITE_NOTSMOOTH(field, tag)                                    \
  do {                                                                      \
    t->sprites.field = load_sprite(t, tag, true, false);                    \
    if (t->sprites.field == NULL) {                                         \
      tileset_error(t, LOG_FATAL, _("Sprite for tag '%s' missing."),        \
                    qUtf8Printable(tag));                                   \
    }                                                                       \
  } while (false)

#define SET_SPRITE_UNSCALED(field, tag)                                     \
  do {                                                                      \
    t->sprites.field = load_sprite(t, qUtf8Printable(tag), false, false);   \
    if (t->sprites.field == NULL) {                                         \
      tileset_error(t, LOG_FATAL, _("Sprite for tag '%s' missing."),        \
                    qUtf8Printable(tag));                                   \
    }                                                                       \
  } while (false)

// Sets sprites.field to tag or (if tag isn't available) to alt
#define SET_SPRITE_ALT(field, tag, alt)                                     \
  do {                                                                      \
    t->sprites.field = load_sprite(t, tag, true, true);                     \
    if (!t->sprites.field) {                                                \
      t->sprites.field = load_sprite(t, alt, true, true);                   \
    }                                                                       \
    if (t->sprites.field == NULL) {                                         \
      tileset_error(t, LOG_FATAL,                                           \
                    _("Sprite for tags '%s' and alternate '%s' are "        \
                      "both missing."),                                     \
                    qUtf8Printable(tag), qUtf8Printable(alt));              \
    }                                                                       \
  } while (false)

// Sets sprites.field to tag, or NULL if not available
#define SET_SPRITE_OPT(field, tag)                                          \
  t->sprites.field = load_sprite(t, tag, true, true)

#define SET_SPRITE_ALT_OPT(field, tag, alt)                                 \
  do {                                                                      \
    t->sprites.field = tiles_lookup_sprite_tag_alt(                         \
        t, LOG_VERBOSE, qUtf8Printable(tag), qUtf8Printable(alt), "sprite", \
        #field, true);                                                      \
  } while (false)

/**
   Setup the graphics for specialist types.
 */
void tileset_setup_specialist_type(struct tileset *t, Specialist_type_id id)
{
  // Load the specialist sprite graphics.
  QString buffer;
  int j;
  struct specialist *spe = specialist_by_number(id);
  const char *tag = spe->graphic_str;
  const char *graphic_alt = spe->graphic_alt;

  for (j = 0; j < MAX_NUM_CITIZEN_SPRITES; j++) {
    // Try tag name + index number
    buffer = QStringLiteral("%1_%2").arg(tag, QString::number(j));
    t->sprites.specialist[id].sprite[j] =
        load_sprite(t, buffer, false, false);

    // Break if no more index specific sprites are defined
    if (!t->sprites.specialist[id].sprite[j]) {
      break;
    }
  }

  if (j == 0) {
    // Try non-indexed
    t->sprites.specialist[id].sprite[j] = load_sprite(t, tag, false, false);

    if (t->sprites.specialist[id].sprite[j]) {
      j = 1;
    }
  }

  if (j == 0) {
    // Try the alt tag
    for (j = 0; j < MAX_NUM_CITIZEN_SPRITES; j++) {
      // Try alt tag name + index number
      buffer = QStringLiteral("%1_%2").arg(graphic_alt, QString::number(j));
      t->sprites.specialist[id].sprite[j] =
          load_sprite(t, buffer, false, false);

      // Break if no more index specific sprites are defined
      if (!t->sprites.specialist[id].sprite[j]) {
        break;
      }
    }
  }

  if (j == 0) {
    // Try alt tag non-indexed
    t->sprites.specialist[id].sprite[j] =
        load_sprite(t, graphic_alt, false, false);

    if (t->sprites.specialist[id].sprite[j]) {
      j = 1;
    }
  }

  t->sprites.specialist[id].count = j;

  // Still nothing? Give up.
  if (j == 0) {
    tileset_error(t, LOG_FATAL, _("No graphics for specialist \"%s\"."),
                  tag);
  }
}

/**
   Setup the graphics for (non-specialist) citizen types.
 */
static void tileset_setup_citizen_types(struct tileset *t)
{
  int i, j;
  QString buffer;

  // Load the citizen sprite graphics, no specialist.
  for (i = 0; i < CITIZEN_LAST; i++) {
    const char *name = citizen_rule_name(static_cast<citizen_category>(i));

    for (j = 0; j < MAX_NUM_CITIZEN_SPRITES; j++) {
      buffer = QStringLiteral("citizen.%1_%2").arg(name, QString::number(j));
      t->sprites.citizen[i].sprite[j] = load_sprite(t, buffer, false, false);
      if (!t->sprites.citizen[i].sprite[j]) {
        break;
      }
    }
    t->sprites.citizen[i].count = j;
    if (j == 0) {
      tileset_error(t, LOG_FATAL, _("No graphics for citizen \"%s\"."),
                    name);
    }
  }
}

/**
   Return the sprite in the city_sprite listing that corresponds to this
   city - based on city style and size.

   See also load_city_sprite, free_city_sprite.
 */
static QPixmap *get_city_sprite(const struct city_sprite *city_sprite,
                                const struct city *pcity)
{
  // get style and match the best tile based on city size
  int style = style_of_city(pcity);
  int num_thresholds;
  struct city_style_threshold *thresholds;
  int img_index;

  fc_assert_ret_val(style < city_sprite->num_styles, NULL);

  num_thresholds = city_sprite->styles[style].land_num_thresholds;
  thresholds = city_sprite->styles[style].land_thresholds;

  if (num_thresholds == 0) {
    return NULL;
  }

  // Get the sprite with the index defined by the effects.
  img_index = pcity->client.city_image;
  if (img_index == -100) {
    /* Server doesn't know right value as this is from old savegame.
     * Guess here based on *client* side information as was done in
     * versions where information was not saved to savegame - this should
     * give us right answer of what city looked like by the time it was
     * put under FoW. */
    img_index = get_city_bonus(pcity, EFT_CITY_IMAGE);
  }
  img_index = CLIP(0, img_index, num_thresholds - 1);

  return thresholds[img_index].sprite;
}

/**
   Allocates one threshold set for city sprite
 */
static int
load_city_thresholds_sprites(struct tileset *t, QString tag, char *graphic,
                             char *graphic_alt,
                             struct city_style_threshold **thresholds)
{
  QString buffer;
  char *gfx_in_use = graphic;
  int num_thresholds = 0;
  QPixmap *sprite;
  int size;

  *thresholds = NULL;

  for (size = 0; size < MAX_CITY_SIZE; size++) {
    buffer = QStringLiteral("%1_%2_%3")
                 .arg(gfx_in_use, tag, QString::number(size));
    if ((sprite = load_sprite(t, buffer, true, true))) {
      num_thresholds++;

      *thresholds = static_cast<city_style_threshold *>(
          fc_realloc(*thresholds, num_thresholds * sizeof(**thresholds)));
      (*thresholds)[num_thresholds - 1].sprite = sprite;
    } else if (size == 0) {
      if (gfx_in_use == graphic) {
        // Try again with graphic_alt.
        size--;
        gfx_in_use = graphic_alt;
      } else {
        // Don't load any others if the 0 element isn't there.
        break;
      }
    }
  }

  return num_thresholds;
}

/**
   Allocates and loads a new city sprite from the given sprite tags.

   tag may be NULL.

   See also get_city_sprite, free_city_sprite.
 */
static struct city_sprite *load_city_sprite(struct tileset *t,
                                            const QString &tag)
{
  struct city_sprite *city_sprite = new struct city_sprite;
  int style;

  /* Store number of styles we have allocated memory for.
   * game.control.styles_count might change if client disconnects from
   * server and connects new one. */
  city_sprite->num_styles = game.control.styles_count;
  city_sprite->styles = new styles[city_sprite->num_styles]();

  for (style = 0; style < city_sprite->num_styles; style++) {
    city_sprite->styles[style].land_num_thresholds =
        load_city_thresholds_sprites(
            t, tag, city_styles[style].graphic,
            city_styles[style].graphic_alt,
            &city_sprite->styles[style].land_thresholds);
  }

  return city_sprite;
}

/**
   Frees a city sprite.

   See also get_city_sprite, load_city_sprite.
 */
static void free_city_sprite(struct city_sprite *city_sprite)
{
  int style;

  if (!city_sprite) {
    return;
  }
  for (style = 0; style < city_sprite->num_styles; style++) {
    if (city_sprite->styles[style].land_thresholds) {
      free(city_sprite->styles[style].land_thresholds);
    }
  }
  delete[] city_sprite->styles;
  delete city_sprite;
}

/**
   Initialize 'sprites' structure based on hardwired tags which
   freeciv always requires.
 */
static void tileset_lookup_sprite_tags(struct tileset *t)
{
  QString buffer, buffer2;
  const int W = t->normal_tile_width, H = t->normal_tile_height;
  int i, j, f;

  fc_assert_ret(t->sprite_hash != NULL);

  SET_SPRITE_UNSCALED(treaty_thumb[0], "treaty.disagree_thumb_down");
  SET_SPRITE_UNSCALED(treaty_thumb[1], "treaty.agree_thumb_up");

  for (j = 0; j < INDICATOR_COUNT; j++) {
    const char *names[] = {"science_bulb", "warming_sun", "cooling_flake"};

    for (i = 0; i < NUM_TILES_PROGRESS; i++) {
      buffer = QStringLiteral("s.%1_%2").arg(names[j], QString::number(j));
      SET_SPRITE_UNSCALED(indicator[j][i], buffer);
    }
  }

  SET_SPRITE(arrow[ARROW_RIGHT], "s.right_arrow");
  SET_SPRITE(arrow[ARROW_PLUS], "s.plus");
  SET_SPRITE(arrow[ARROW_MINUS], "s.minus");
  if (t->type == TS_ISOMETRIC) {
    SET_SPRITE(dither_tile, "t.dither_tile");
  }

  if (tileset_is_isometric(tileset)) {
    SET_SPRITE_NOTSMOOTH(mask.tile, "mask.tile");
  } else {
    SET_SPRITE(mask.tile, "mask.tile");
  }
  SET_SPRITE(mask.worked_tile, "mask.worked_tile");
  SET_SPRITE(mask.unworked_tile, "mask.unworked_tile");

  SET_SPRITE_UNSCALED(tax_luxury, "s.tax_luxury");
  SET_SPRITE_UNSCALED(tax_science, "s.tax_science");
  SET_SPRITE_UNSCALED(tax_gold, "s.tax_gold");

  tileset_setup_citizen_types(t);

  for (i = 0; i < SPACESHIP_COUNT; i++) {
    const char *names[SPACESHIP_COUNT] = {
        "solar_panels", "life_support", "habitation", "structural",
        "fuel",         "propulsion",   "exhaust"};

    buffer = QStringLiteral("spaceship.%1").arg(names[i]);
    SET_SPRITE(spaceship[i], buffer);
  }

  for (i = 0; i < CURSOR_LAST; i++) {
    for (f = 0; f < NUM_CURSOR_FRAMES; f++) {
      const char *names[CURSOR_LAST] = {
          "goto",    "patrol", "paradrop",   "nuke",     "select",
          "invalid", "attack", "edit_paint", "edit_add", "wait"};
      struct small_sprite *ss;

      fc_assert(ARRAY_SIZE(names) == CURSOR_LAST);
      buffer =
          QStringLiteral("cursor.%1%2").arg(names[i], QString::number(f));
      SET_SPRITE(cursor[i].frame[f], buffer);
      ss = t->sprite_hash->value(buffer, nullptr);
      if (ss) {
        t->sprites.cursor[i].hot_x = ss->hot_x;
        t->sprites.cursor[i].hot_y = ss->hot_y;
      }
    }
  }

  for (i = 0; i < ICON_COUNT; i++) {
    const char *names[ICON_COUNT] = {"freeciv", "citydlg"};

    buffer = QStringLiteral("icon.%1").arg(names[i]);
    SET_SPRITE(icon[i], buffer);
  }

  for (i = 0; i < E_COUNT; i++) {
    const char *tag = get_event_tag(static_cast<event_type>(i));

    SET_SPRITE(events[i], tag);
  }

  SET_SPRITE(explode.nuke, "explode.nuke");

  sprite_vector_init(&t->sprites.explode.unit);
  for (i = 0;; i++) {
    QPixmap *sprite;

    buffer = QStringLiteral("explode.unit_%1").arg(QString::number(i));
    sprite = load_sprite(t, buffer, true, true);
    if (!sprite) {
      break;
    }
    sprite_vector_append(&t->sprites.explode.unit, sprite);
  }

  SET_SPRITE(unit.auto_attack, "unit.auto_attack");
  SET_SPRITE(unit.auto_settler, "unit.auto_settler");
  SET_SPRITE(unit.auto_explore, "unit.auto_explore");
  SET_SPRITE(unit.fortified, "unit.fortified");
  SET_SPRITE(unit.fortifying, "unit.fortifying");
  SET_SPRITE(unit.go_to, "unit.goto");
  SET_SPRITE(unit.irrigate, "unit.irrigate");
  SET_SPRITE(unit.plant, "unit.plant");
  SET_SPRITE(unit.pillage, "unit.pillage");
  SET_SPRITE(unit.sentry, "unit.sentry");
  SET_SPRITE(unit.convert, "unit.convert");
  SET_SPRITE(unit.stack, "unit.stack");
  SET_SPRITE(unit.loaded, "unit.loaded");
  SET_SPRITE(unit.transform, "unit.transform");
  SET_SPRITE(unit.connect, "unit.connect");
  SET_SPRITE(unit.patrol, "unit.patrol");
  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    buffer = QStringLiteral("unit.battlegroup_%1").arg(QString::number(i));
    buffer2 = QStringLiteral("city.size_%1").arg(QString::number(i + 1));
    fc_assert(MAX_NUM_BATTLEGROUPS < NUM_TILES_DIGITS);
    SET_SPRITE_ALT(unit.battlegroup[i], buffer, buffer2);
  }
  SET_SPRITE(unit.lowfuel, "unit.lowfuel");
  SET_SPRITE(unit.tired, "unit.tired");

  SET_SPRITE_OPT(unit.action_decision_want, "unit.action_decision_want");

  for (i = 0; i <= 100; i++) {
    buffer = QStringLiteral("unit.hp_%1").arg(QString::number(i));
    auto sprite = load_sprite(t, buffer, true, true);
    if (sprite) {
      t->sprites.unit.hp_bar.push_back(sprite);
    }
  }

  for (i = 0; i < MAX_VET_LEVELS; i++) {
    /* Veteran level sprites are optional.  For instance "green" units
     * usually have no special graphic. */
    buffer = QStringLiteral("unit.vet_%1").arg(QString::number(i));
    t->sprites.unit.vet_lev[i] = load_sprite(t, buffer, true, true);
  }

  for (i = 0;; i++) {
    buffer = QStringLiteral("unit.select%1").arg(QString::number(i));
    auto sprite = load_sprite(t, buffer, true, true);
    if (!sprite) {
      break;
    }
    t->sprites.unit.select.push_back(sprite);
  }

  SET_SPRITE(citybar.shields, "citybar.shields");
  SET_SPRITE(citybar.food, "citybar.food");
  SET_SPRITE(citybar.trade, "citybar.trade");
  SET_SPRITE(citybar.occupied, "citybar.occupied");
  SET_SPRITE(citybar.background, "citybar.background");
  sprite_vector_init(&t->sprites.citybar.occupancy);
  for (i = 0;; i++) {
    QPixmap *sprite;

    buffer = QStringLiteral("citybar.occupancy_%1").arg(QString::number(i));
    sprite = load_sprite(t, buffer, true, true);
    if (!sprite) {
      break;
    }
    sprite_vector_append(&t->sprites.citybar.occupancy, sprite);
  }
  if (t->sprites.citybar.occupancy.size < 2) {
    tileset_error(t, LOG_FATAL,
                  _("Missing necessary citybar.occupancy_N sprites."));
  }

#define SET_EDITOR_SPRITE(x) SET_SPRITE(editor.x, "editor." #x)
  SET_EDITOR_SPRITE(erase);
  SET_EDITOR_SPRITE(brush);
  SET_EDITOR_SPRITE(copy);
  SET_EDITOR_SPRITE(paste);
  SET_EDITOR_SPRITE(copypaste);
  SET_EDITOR_SPRITE(startpos);
  SET_EDITOR_SPRITE(terrain);
  SET_EDITOR_SPRITE(terrain_resource);
  SET_EDITOR_SPRITE(terrain_special);
  SET_EDITOR_SPRITE(unit);
  SET_EDITOR_SPRITE(city);
  SET_EDITOR_SPRITE(vision);
  SET_EDITOR_SPRITE(territory);
  SET_EDITOR_SPRITE(properties);
  SET_EDITOR_SPRITE(road);
  SET_EDITOR_SPRITE(military_base);
#undef SET_EDITOR_SPRITE

  SET_SPRITE(city.disorder, "city.disorder");

  /* Fallbacks for goto path turn numbers:
   *   path.step_%d, path.exhausted_mp_%d
   *   --> path.turns_%d
   *       --> city.size_%d */
#define SET_GOTO_TURN_SPRITE(state, state_name, factor, factor_name)        \
  buffer = QStringLiteral("path." state_name "_%1" #factor).arg(i);         \
  SET_SPRITE_OPT(path.s[state].turns##factor_name[i], buffer);              \
  if (t->sprites.path.s[state].turns##factor_name[i] == NULL) {             \
    t->sprites.path.s[state].turns##factor_name[i] =                        \
        t->sprites.path.s[GTS_MP_LEFT].turns##factor_name[i];               \
  }

  for (i = 0; i < NUM_TILES_DIGITS; i++) {
    buffer = QStringLiteral("city.size_%1").arg(QString::number(i));
    SET_SPRITE(city.size[i], buffer);
    buffer2 = QStringLiteral("path.turns_%1").arg(QString::number(i));
    SET_SPRITE_ALT(path.s[GTS_MP_LEFT].turns[i], buffer2, buffer);
    SET_GOTO_TURN_SPRITE(GTS_TURN_STEP, "step", , );
    SET_GOTO_TURN_SPRITE(GTS_EXHAUSTED_MP, "exhausted_mp", , );

    // using old buffer doesnt work, mb something wiped it
    buffer = QStringLiteral("city.size_%1").arg(QString::number(i));
    buffer += "0";
    SET_SPRITE(city.size_tens[i], buffer);
    buffer2 = QStringLiteral("path.turns_%1").arg(QString::number(i));
    buffer2 += "0";
    SET_SPRITE_ALT(path.s[GTS_MP_LEFT].turns_tens[i], buffer2, buffer);
    SET_GOTO_TURN_SPRITE(GTS_TURN_STEP, "step", 0, _tens);
    SET_GOTO_TURN_SPRITE(GTS_EXHAUSTED_MP, "exhausted_mp", 0, _tens);

    buffer = QStringLiteral("city.size_%1").arg(QString::number(i));
    buffer += "00";
    SET_SPRITE_OPT(city.size_hundreds[i], buffer);
    buffer2 = QStringLiteral("path.turns_%1").arg(QString::number(i));
    buffer2 += "00";
    SET_SPRITE_ALT_OPT(path.s[GTS_MP_LEFT].turns_hundreds[i], buffer2,
                       buffer);
    SET_GOTO_TURN_SPRITE(GTS_TURN_STEP, "step", 00, _hundreds);
    SET_GOTO_TURN_SPRITE(GTS_EXHAUSTED_MP, "exhausted_mp", 00, _hundreds);
  }
  for (int i = 0;; ++i) {
    buffer = QStringLiteral("city.t_food_%1").arg(QString::number(i));
    if (auto sprite = load_sprite(t, buffer, true, true)) {
      t->sprites.city.tile_foodnum.push_back(*sprite);
    } else {
      break;
    }
  }
  for (int i = 0;; ++i) {
    buffer = QStringLiteral("city.t_shields_%1").arg(QString::number(i));
    if (auto sprite = load_sprite(t, buffer, true, true)) {
      t->sprites.city.tile_shieldnum.push_back(*sprite);
    } else {
      break;
    }
  }
  for (int i = 0;; ++i) {
    buffer = QStringLiteral("city.t_trade_%1").arg(QString::number(i));
    if (auto sprite = load_sprite(t, buffer, true, true)) {
      t->sprites.city.tile_tradenum.push_back(*sprite);
    } else {
      break;
    }
  }
#undef SET_GOTO_TURN_SPRITE

  // Must have at least one upkeep sprite per output type (and unhappy)
  // The rest are optional.
  buffer = QStringLiteral("upkeep.unhappy");
  t->sprites.upkeep.unhappy.push_back(load_sprite(t, buffer, true, true));
  if (!t->sprites.upkeep.unhappy.back()) {
    tileset_error(t, LOG_FATAL, "Missing sprite upkeep.unhappy");
  }
  for (i = 1;; i++) {
    buffer = QStringLiteral("upkeep.unhappy%1").arg(QString::number(i));
    auto sprite = load_sprite(t, buffer, true, true);
    if (!sprite) {
      break;
    }
    t->sprites.upkeep.unhappy.push_back(sprite);
  }
  output_type_iterate(o)
  {
    buffer = QStringLiteral("upkeep.%1")
                 .arg(get_output_identifier(static_cast<Output_type_id>(o)));
    auto sprite = load_sprite(t, buffer, true, true);
    if (sprite) {
      t->sprites.upkeep.output[o].push_back(sprite);
    }
    for (i = 1;; i++) {
      buffer =
          QStringLiteral("upkeep.%1%2")
              .arg(get_output_identifier(static_cast<Output_type_id>(o)),
                   QString::number(i + 1));
      auto sprite = load_sprite(t, buffer, true, true);
      if (!sprite) {
        break;
      }
      t->sprites.upkeep.output[o].push_back(sprite);
    }
  }
  output_type_iterate_end;

  t->max_upkeep_height = calculate_max_upkeep_height(t);

  SET_SPRITE(user.attention, "user.attention");

  SET_SPRITE_OPT(path.s[GTS_MP_LEFT].specific, "path.normal");
  SET_SPRITE_OPT(path.s[GTS_EXHAUSTED_MP].specific, "path.exhausted_mp");
  SET_SPRITE_OPT(path.s[GTS_TURN_STEP].specific, "path.step");
  SET_SPRITE(path.waypoint, "path.waypoint");

  SET_SPRITE_NOTSMOOTH(tx.fog, "tx.fog");

  sprite_vector_init(&t->sprites.colors.overlays);
  for (i = 0;; i++) {
    QPixmap *sprite;

    buffer = QStringLiteral("colors.overlay_%1").arg(QString::number(i));
    sprite = load_sprite(t, buffer, true, true);
    if (!sprite) {
      break;
    }
    sprite_vector_append(&t->sprites.colors.overlays, sprite);
  }
  if (i == 0) {
    tileset_error(t, LOG_FATAL,
                  _("Missing overlay-color sprite colors.overlay_0."));
  }

  // Chop up and build the overlay graphics.
  sprite_vector_reserve(&t->sprites.city.worked_tile_overlay,
                        sprite_vector_size(&t->sprites.colors.overlays));
  sprite_vector_reserve(&t->sprites.city.unworked_tile_overlay,
                        sprite_vector_size(&t->sprites.colors.overlays));
  for (i = 0; i < sprite_vector_size(&t->sprites.colors.overlays); i++) {
    QPixmap *color, *color_mask;
    QPixmap *worked, *unworked;

    color = *sprite_vector_get(&t->sprites.colors.overlays, i);
    color_mask = crop_sprite(color, 0, 0, W, H, t->sprites.mask.tile, 0, 0,
                             1.0f, false);
    worked = crop_sprite(color_mask, 0, 0, W, H, t->sprites.mask.worked_tile,
                         0, 0, 1.0f, false);
    unworked = crop_sprite(color_mask, 0, 0, W, H,
                           t->sprites.mask.unworked_tile, 0, 0, 1.0f, false);
    free_sprite(color_mask);
    t->sprites.city.worked_tile_overlay.p[i] = worked;
    t->sprites.city.unworked_tile_overlay.p[i] = unworked;
  }

  {
    SET_SPRITE(grid.unavailable, "grid.unavailable");
    SET_SPRITE_OPT(grid.nonnative, "grid.nonnative");

    for (i = 0; i < EDGE_COUNT; i++) {
      int be;

      if (i == EDGE_UD && t->hex_width == 0) {
        continue;
      } else if (i == EDGE_LR && t->hex_height == 0) {
        continue;
      }

      buffer = QStringLiteral("grid.main.%1").arg(edge_name[i]);
      SET_SPRITE(grid.main[i], buffer);

      buffer = QStringLiteral("grid.city.%1").arg(edge_name[i]);
      SET_SPRITE(grid.city[i], buffer);

      buffer = QStringLiteral("grid.worked.%1").arg(edge_name[i]);
      SET_SPRITE(grid.worked[i], buffer);

      buffer = QStringLiteral("grid.selected.%1").arg(edge_name[i]);
      SET_SPRITE(grid.selected[i], buffer);

      buffer = QStringLiteral("grid.coastline.%1").arg(edge_name[i]);
      SET_SPRITE(grid.coastline[i], buffer);

      for (be = 0; be < 2; be++) {
        buffer = QStringLiteral("grid.borders.%1").arg(edge_name[i][be]);
        SET_SPRITE(grid.borders[i][be], buffer);
      }
    }
  }

  switch (t->darkness_layer->style()) {
  case freeciv::DARKNESS_NONE:
    // Nothing.
    break;
  case freeciv::DARKNESS_ISORECT: {
    // Isometric: take a single tx.darkness tile and split it into 4.
    QPixmap *darkness =
        load_sprite(t, QStringLiteral("tx.darkness"), true, false);
    const int ntw = t->normal_tile_width, nth = t->normal_tile_height;
    int offsets[4][2] = {
        {ntw / 2, 0}, {0, nth / 2}, {ntw / 2, nth / 2}, {0, 0}};

    if (!darkness) {
      tileset_error(t, LOG_FATAL, _("Sprite tx.darkness missing."));
    }
    for (i = 0; i < 4; i++) {
      t->darkness_layer->set_sprite(
          i, *crop_sprite(darkness, offsets[i][0], offsets[i][1], ntw / 2,
                          nth / 2, NULL, 0, 0, 1.0f, false));
    }
  } break;
  case freeciv::DARKNESS_CARD_SINGLE:
    for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
      enum direction8 dir = t->cardinal_tileset_dirs[i];

      buffer =
          QStringLiteral("tx.darkness_%1").arg(dir_get_tileset_name(dir));

      const auto sprite = load_sprite(t, buffer, true, false);
      if (sprite) {
        t->darkness_layer->set_sprite(i, *sprite);
      } else {
        tileset_error(t, LOG_FATAL, _("Sprite for tag '%s' missing."),
                      qUtf8Printable(buffer));
      }
    }
    break;
  case freeciv::DARKNESS_CARD_FULL:
    for (i = 1; i < t->num_index_cardinal; i++) {
      buffer =
          QStringLiteral("tx.darkness_%1").arg(cardinal_index_str(t, i));

      const auto sprite = load_sprite(t, buffer, true, false);
      if (sprite) {
        t->darkness_layer->set_sprite(i, *sprite);
      } else {
        tileset_error(t, LOG_FATAL, _("Sprite for tag '%s' missing."),
                      qUtf8Printable(buffer));
      }
    }
    break;
  case freeciv::DARKNESS_CORNER:
    t->sprites.tx.fullfog = static_cast<QPixmap **>(fc_realloc(
        t->sprites.tx.fullfog, 81 * sizeof(*t->sprites.tx.fullfog)));
    for (i = 0; i < 81; i++) {
      // Unknown, fog, known.
      char ids[] = {'u', 'f', 'k'};
      char buf[512] = "t.fog";
      int values[4], vi, k = i;

      for (vi = 0; vi < 4; vi++) {
        values[vi] = k % 3;
        k /= 3;

        cat_snprintf(buf, sizeof(buf), "_%c", ids[values[vi]]);
      }
      fc_assert(k == 0);

      t->sprites.tx.fullfog[i] = load_sprite(t, buf, true, false);
    }
    break;
  };

  // no other place to initialize these variables
  sprite_vector_init(&t->sprites.nation_flag);
  sprite_vector_init(&t->sprites.nation_shield);
}

/**
   Load sprites of one river type.
 */
static bool load_river_sprites(struct tileset *t,
                               struct river_sprites *store,
                               const char *tag_pfx)
{
  int i;
  QString buffer;

  for (i = 0; i < t->num_index_cardinal; i++) {
    buffer =
        QStringLiteral("%1_s_%2").arg(tag_pfx, cardinal_index_str(t, i));
    store->spec[i] = load_sprite(t, buffer, true, true);
    if (store->spec[i] == NULL) {
      return false;
    }
  }

  for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
    buffer =
        QStringLiteral("%1_outlet_%2")
            .arg(tag_pfx, dir_get_tileset_name(t->cardinal_tileset_dirs[i]));
    store->outlet[i] = load_sprite(t, buffer, true, true);
    if (store->outlet[i] == NULL) {
      qCritical("Missing \"%s\" for \"%s\".", qUtf8Printable(buffer),
                tag_pfx);
      return false;
    }
  }

  return true;
}

/**
   Frees any internal buffers which are created by load_sprite. Should
   be called after the last (for a given period of time) load_sprite
   call.  This saves a fair amount of memory, but it will take extra time
   the next time we start loading sprites again.
 */
void finish_loading_sprites(struct tileset *t)
{
  for (auto *sf : qAsConst(*t->specfiles)) {
    if (sf->big_sprite) {
      free_sprite(sf->big_sprite);
      sf->big_sprite = NULL;
    }
  }
}

/**
   Load the tiles; requires tilespec_read_toplevel() called previously.
   Leads to tile_sprites being allocated and filled with pointers
   to sprites.   Also sets up and populates sprite_hash, and calls func
   to initialize 'sprites' structure.
 */
void tileset_load_tiles(struct tileset *t)
{
  tileset_lookup_sprite_tags(t);
  finish_loading_sprites(t);
}

/**
   Lookup sprite to match tag, or else to match alt if don't find,
   or else return NULL, and emit log message.
 */
QPixmap *tiles_lookup_sprite_tag_alt(struct tileset *t, QtMsgType level,
                                     const char *tag, const char *alt,
                                     const char *what, const char *name,
                                     bool scale)
{
  QPixmap *sp;

  // (should get sprite_hash before connection)
  fc_assert_ret_val_msg(NULL != t->sprite_hash, NULL,
                        "attempt to lookup for %s \"%s\" before "
                        "sprite_hash setup",
                        what, name);

  sp = load_sprite(t, tag, scale, true);
  if (sp) {
    return sp;
  }

  sp = load_sprite(t, alt, scale, true);
  if (sp) {
    qDebug("Using alternate graphic \"%s\" "
           "(instead of \"%s\") for %s \"%s\".",
           alt, tag, what, name);
    return sp;
  }

  tileset_error(
      t, level,
      _("Don't have graphics tags \"%s\" or \"%s\" for %s \"%s\"."), tag,
      alt, what, name);

  return NULL;
}

/**
   Helper function to load sprite for one unit orientation.
   Returns FALSE if a needed sprite was not found.
 */
static bool tileset_setup_unit_direction(struct tileset *t, int uidx,
                                         const char *base_str,
                                         enum direction8 dir, bool has_icon)
{
  QString buf;
  enum direction8 loaddir = dir;

  /*
   * There may be more orientations available in this tileset than are
   * needed, if an oriented unit set has been re-used between tilesets.
   *
   * Don't bother loading unused ones, unless they might be used by
   * unit_default_orientation (logic here mirrors get_unittype_sprite()).
   */
  if (!(dir == t->unit_default_orientation && !has_icon)
      && !is_valid_tileset_dir(t, dir)) {
    /* Instead we copy a nearby valid dir's sprite, so we're not caught
     * out in case this tileset is used with an incompatible topology,
     * although it'll be ugly. */
    do {
      loaddir = dir_cw(loaddir);
      // This loop _should_ terminate...
      fc_assert_ret_val(loaddir != dir, false);
    } while (!is_valid_tileset_dir(t, loaddir));
  }

  buf = QStringLiteral("%1_%2").arg(base_str, dir_get_tileset_name(loaddir));

  /* We don't use _alt graphics here, as that could lead to loading
   * real icon gfx, but alternative orientation gfx. Tileset author
   * probably meant icon gfx to be used as fallback for all orientations */
  t->sprites.units.facing[uidx][dir] = load_sprite(t, buf, true, true);

  return t->sprites.units.facing[uidx][dir] != NULL;
}

/**
   Try to setup all unit type sprites from single tag
 */
static bool tileset_setup_unit_type_from_tag(struct tileset *t, int uidx,
                                             const char *tag)
{
  bool has_icon, facing_sprites = true;

  t->sprites.units.icon[uidx] = load_sprite(t, tag, true, true);
  has_icon = t->sprites.units.icon[uidx] != NULL;

#define LOAD_FACING_SPRITE(dir)                                             \
  if (!tileset_setup_unit_direction(t, uidx, tag, dir, has_icon)) {         \
    facing_sprites = false;                                                 \
  }

  LOAD_FACING_SPRITE(DIR8_NORTHWEST);
  LOAD_FACING_SPRITE(DIR8_NORTH);
  LOAD_FACING_SPRITE(DIR8_NORTHEAST);
  LOAD_FACING_SPRITE(DIR8_WEST);
  LOAD_FACING_SPRITE(DIR8_EAST);
  LOAD_FACING_SPRITE(DIR8_SOUTHWEST);
  LOAD_FACING_SPRITE(DIR8_SOUTH);
  LOAD_FACING_SPRITE(DIR8_SOUTHEAST);

  if (!has_icon && !facing_sprites) {
    // Neither icon gfx or orientation sprites
    return false;
  }

  return true;

#undef LOAD_FACING_SPRITE
}

/**
   Set unit_type sprite value; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_unit_type(struct tileset *t, struct unit_type *ut)
{
  int uidx = utype_index(ut);

  if (!tileset_setup_unit_type_from_tag(t, uidx, ut->graphic_str)
      && !tileset_setup_unit_type_from_tag(t, uidx, ut->graphic_alt)) {
    tileset_error(t, LOG_FATAL,
                  _("Missing %s unit sprite for tags \"%s\" and "
                    "alternative \"%s\"."),
                  utype_rule_name(ut), ut->graphic_str, ut->graphic_alt);
  }

  if (!t->sprites.units.icon[uidx]) {
    if (!direction8_is_valid(t->unit_default_orientation)) {
      tileset_error(t, LOG_FATAL,
                    "Unit type %s has no unoriented sprite and "
                    "tileset has no unit_default_orientation.",
                    utype_rule_name(ut));
    } else {
      /* We're guaranteed to have an oriented sprite corresponding to
       * unit_default_orientation, because
       * tileset_setup_unit_type_from_tag() checked for this. */
      fc_assert(t->sprites.units.facing[uidx][t->unit_default_orientation]
                != NULL);
    }
  }
}

/**
   Set improvement_type sprite value; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_impr_type(struct tileset *t, struct impr_type *pimprove)
{
  t->sprites.building[improvement_index(pimprove)] =
      tiles_lookup_sprite_tag_alt(t, LOG_VERBOSE, pimprove->graphic_str,
                                  pimprove->graphic_alt, "improvement",
                                  improvement_rule_name(pimprove), false);

  // should maybe do something if NULL, eg generic default?
}

/**
   Set tech_type sprite value; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_tech_type(struct tileset *t, struct advance *padvance)
{
  if (valid_advance(padvance)) {
    t->sprites.tech[advance_index(padvance)] = tiles_lookup_sprite_tag_alt(
        t, LOG_VERBOSE, padvance->graphic_str, padvance->graphic_alt,
        "technology", advance_rule_name(padvance), false);

    // should maybe do something if NULL, eg generic default?
  } else {
    t->sprites.tech[advance_index(padvance)] = NULL;
  }
}

/**
   Set extra sprite values; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_extra(struct tileset *t, struct extra_type *pextra)
{
  const int id = extra_index(pextra);
  int extrastyle;

  if (!fc_strcasecmp(pextra->graphic_str, "none")) {
    // Extra without graphics
    t->sprites.extras[id].extrastyle = extrastyle_id_invalid();
  } else {
    const char *tag;

    tag = pextra->graphic_str;
    if (!t->estyle_hash->contains(tag)) {
      tag = pextra->graphic_alt;
      if (!t->estyle_hash->contains(tag)) {
        tileset_error(t, LOG_FATAL,
                      _("No extra style for \"%s\" or \"%s\"."),
                      pextra->graphic_str, pextra->graphic_alt);
      } else {
        qDebug("Using alternate graphic \"%s\" "
               "(instead of \"%s\") for extra \"%s\".",
               pextra->graphic_alt, pextra->graphic_str,
               extra_rule_name(pextra));
      }
    }
    extrastyle = t->estyle_hash->value(tag);

    t->sprites.extras[id].extrastyle = extrastyle;

    extra_type_list_append(t->style_lists[extrastyle], pextra);

    switch (extrastyle) {
    case ESTYLE_3LAYER:
      tileset_setup_base(t, pextra, tag);
      break;

    case ESTYLE_ROAD_ALL_SEPARATE:
    case ESTYLE_ROAD_PARITY_COMBINED:
    case ESTYLE_ROAD_ALL_COMBINED:
    case ESTYLE_RIVER:
      tileset_setup_road(t, pextra, tag);
      break;

    case ESTYLE_SINGLE1:
      t->special_layers.background->set_sprite(pextra, tag);
      break;
    case ESTYLE_SINGLE2:
      t->special_layers.middleground->set_sprite(pextra, tag);
      break;

    case ESTYLE_CARDINALS: {
      int i;
      QString buffer;

      /* We use direction-specific irrigation and farmland graphics, if
       * they are available.  If not, we just fall back to the basic
       * irrigation graphics. */
      for (i = 0; i < t->num_index_cardinal; i++) {
        buffer = QStringLiteral("%1_%2").arg(tag, cardinal_index_str(t, i));
        t->sprites.extras[id].u.cardinals[i] =
            load_sprite(t, buffer, true, true);
        if (!t->sprites.extras[id].u.cardinals[i]) {
          t->sprites.extras[id].u.cardinals[i] =
              load_sprite(t, tag, true, true);
        }
        if (!t->sprites.extras[id].u.cardinals[i]) {
          tileset_error(t, LOG_FATAL,
                        _("No cardinal-style graphics \"%s*\" for "
                          "extra \"%s\""),
                        tag, extra_rule_name(pextra));
        }
      }
    } break;
    case ESTYLE_COUNT:
      break;
    }
  }

  if (!fc_strcasecmp(pextra->activity_gfx, "none")) {
    t->sprites.extras[id].activity = NULL;
  } else {
    t->sprites.extras[id].activity =
        load_sprite(t, pextra->activity_gfx, true, true);
    if (t->sprites.extras[id].activity == NULL) {
      t->sprites.extras[id].activity =
          load_sprite(t, pextra->act_gfx_alt, true, true);
    }
    if (t->sprites.extras[id].activity == NULL) {
      t->sprites.extras[id].activity =
          load_sprite(t, pextra->act_gfx_alt2, true, true);
    }
    if (t->sprites.extras[id].activity == NULL) {
      tileset_error(t, LOG_FATAL,
                    _("Missing %s building activity sprite for tags \"%s\" "
                      "and alternatives \"%s\" and \"%s\"."),
                    extra_rule_name(pextra), pextra->activity_gfx,
                    pextra->act_gfx_alt, pextra->act_gfx_alt2);
    }
  }

  if (!fc_strcasecmp(pextra->rmact_gfx, "none")) {
    t->sprites.extras[id].rmact = NULL;
  } else {
    t->sprites.extras[id].rmact =
        load_sprite(t, pextra->rmact_gfx, true, true);
    if (t->sprites.extras[id].rmact == NULL) {
      t->sprites.extras[id].rmact =
          load_sprite(t, pextra->rmact_gfx_alt, true, true);
      if (t->sprites.extras[id].rmact == NULL) {
        tileset_error(t, LOG_FATAL,
                      _("Missing %s removal activity sprite for tags \"%s\" "
                        "and alternative \"%s\"."),
                      extra_rule_name(pextra), pextra->rmact_gfx,
                      pextra->rmact_gfx_alt);
      }
    }
  }
}

/**
   Set road sprite values; should only happen after
   tilespec_load_tiles().
 */
static void tileset_setup_road(struct tileset *t, struct extra_type *pextra,
                               const char *tag)
{
  QString full_tag_name;
  const int id = extra_index(pextra);
  int i;
  int extrastyle = t->sprites.extras[id].extrastyle;

  /* Isolated road graphics are used by ESTYLE_ROAD_ALL_SEPARATE and
     ESTYLE_ROAD_PARITY_COMBINED. */
  if (extrastyle == ESTYLE_ROAD_ALL_SEPARATE
      || extrastyle == ESTYLE_ROAD_PARITY_COMBINED) {
    full_tag_name = QStringLiteral("%1_isolated").arg(tag);
    SET_SPRITE(extras[id].u.road.isolated, qUtf8Printable(full_tag_name));
  }

  if (extrastyle == ESTYLE_ROAD_ALL_SEPARATE) {
    /* ESTYLE_ROAD_ALL_SEPARATE has just 8 additional sprites for each
     * road type: one going off in each direction. */
    for (i = 0; i < t->num_valid_tileset_dirs; i++) {
      enum direction8 dir = t->valid_tileset_dirs[i];
      QString dir_name = dir_get_tileset_name(dir);

      full_tag_name = QStringLiteral("%1_%2").arg(tag, dir_name);

      SET_SPRITE(extras[id].u.road.ru.dir[i], qUtf8Printable(full_tag_name));
    }
  } else if (extrastyle == ESTYLE_ROAD_PARITY_COMBINED) {
    int num_index = 1 << (t->num_valid_tileset_dirs / 2), j;

    /* ESTYLE_ROAD_PARITY_COMBINED has 32 additional sprites for each road
     * type: 16 each for cardinal and diagonal directions.  Each set
     * of 16 provides a NSEW-indexed sprite to provide connectors for
     * all rails in the cardinal/diagonal directions.  The 0 entry is
     * unused (the "isolated" sprite is used instead). */

    for (i = 1; i < num_index; i++) {
      QString c, d;

      for (j = 0; j < t->num_valid_tileset_dirs / 2; j++) {
        int value = (i >> j) & 1;
        c += QStringLiteral("%1%2").arg(
            dir_get_tileset_name(t->valid_tileset_dirs[2 * j]),
            QString::number(value));
        d += QStringLiteral("%1%2").arg(
            dir_get_tileset_name(t->valid_tileset_dirs[2 * j + 1]),
            QString::number(value));
      }
      full_tag_name = QStringLiteral("%1_c_%2").arg(tag, c);

      SET_SPRITE(extras[id].u.road.ru.combo.even[i],
                 qUtf8Printable(full_tag_name));
      full_tag_name = QStringLiteral("%1_d_%2").arg(tag, d);

      SET_SPRITE(extras[id].u.road.ru.combo.odd[i],
                 qUtf8Printable(full_tag_name));
    }
  } else if (extrastyle == ESTYLE_ROAD_ALL_COMBINED) {
    /* ESTYLE_ROAD_ALL_COMBINED includes 256 sprites, one for every
     * possibility. Just go around clockwise, with all combinations. */
    for (i = 0; i < t->num_index_valid; i++) {
      QString idx_str = valid_index_str(t, i);

      full_tag_name = QStringLiteral("%1_%2").arg(tag, idx_str);

      SET_SPRITE(extras[id].u.road.ru.total[i],
                 qUtf8Printable(full_tag_name));
    }
  } else if (extrastyle == ESTYLE_RIVER) {
    if (!load_river_sprites(t, &t->sprites.extras[id].u.road.ru.rivers,
                            tag)) {
      tileset_error(t, LOG_FATAL,
                    _("No river-style graphics \"%s*\" for extra \"%s\""),
                    tag, extra_rule_name(pextra));
    }
  } else {
    fc_assert(false);
  }

  /* Corner road graphics are used by ESTYLE_ROAD_ALL_SEPARATE and
   * ESTYLE_ROAD_PARITY_COMBINED. */
  if (extrastyle == ESTYLE_ROAD_ALL_SEPARATE
      || extrastyle == ESTYLE_ROAD_PARITY_COMBINED) {
    for (i = 0; i < t->num_valid_tileset_dirs; i++) {
      enum direction8 dir = t->valid_tileset_dirs[i];

      if (!is_cardinal_tileset_dir(t, dir)) {
        QString dtn = dir_get_tileset_name(dir);

        full_tag_name =
            QStringLiteral("%1_c_%2").arg(pextra->graphic_str, dtn);

        SET_SPRITE_OPT(extras[id].u.road.corner[dir],
                       qUtf8Printable(full_tag_name));
      }
    }
  }
}

/**
   Set base sprite values; should only happen after
   tilespec_load_tiles().
 */
static void tileset_setup_base(struct tileset *t,
                               const struct extra_type *pextra,
                               const char *tag)
{
  const int id = extra_index(pextra);

  fc_assert_ret(id >= 0 && id < extra_count());

  QString full_tag_name = QString(tag) + QStringLiteral("_bg");
  t->special_layers.background->set_sprite(
      pextra, full_tag_name, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET);

  full_tag_name = QString(tag) + QStringLiteral("_mg");
  t->special_layers.middleground->set_sprite(
      pextra, full_tag_name, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET);

  full_tag_name = QString(tag) + QStringLiteral("_fg");
  t->special_layers.foreground->set_sprite(
      pextra, full_tag_name, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET);
}

/**
   Set tile_type sprite values; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_tile_type(struct tileset *t,
                             const struct terrain *pterrain)
{
  for (auto &layer : t->layers) {
    layer->initialize_terrain(pterrain);
  }
}

/**
   Set government sprite value; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_government(struct tileset *t, struct government *gov)
{
  t->sprites.government[government_index(gov)] = tiles_lookup_sprite_tag_alt(
      t, LOG_FATAL, gov->graphic_str, gov->graphic_alt, "government",
      government_rule_name(gov), false);

  // should probably do something if NULL, eg generic default?
}

/**
   Set nation flag sprite value; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_nation_flag(struct tileset *t, struct nation_type *nation)
{
  const char *tags[] = {nation->flag_graphic_str, nation->flag_graphic_alt,
                        "unknown", NULL};
  int i;
  QPixmap *flag = NULL, *shield = NULL;
  QString buf;

  for (i = 0; tags[i] && !flag; i++) {
    buf = QStringLiteral("f.%1").arg(tags[i]);
    flag = load_sprite(t, buf, true, true);
  }
  for (i = 0; tags[i] && !shield; i++) {
    buf = QStringLiteral("f.shield.%1").arg(tags[i]);
    shield = load_sprite(t, buf, true, true);
  }
  if (!flag || !shield) {
    // Should never get here because of the f.unknown fallback.
    tileset_error(t, LOG_FATAL, _("Nation %s: no national flag."),
                  nation_rule_name(nation));
  }

  sprite_vector_reserve(&t->sprites.nation_flag, nation_count());
  t->sprites.nation_flag.p[nation_index(nation)] = flag;

  sprite_vector_reserve(&t->sprites.nation_shield, nation_count());
  t->sprites.nation_shield.p[nation_index(nation)] = shield;
}

/**
   Return the flag graphic to be used by the city.
 */
const QPixmap *get_city_flag_sprite(const struct tileset *t,
                                    const struct city *pcity)
{
  return get_nation_flag_sprite(t, nation_of_city(pcity));
}

/**
   Return a sprite for the national flag for this unit.
 */
static QPixmap *get_unit_nation_flag_sprite(const struct tileset *t,
                                            const struct unit *punit)
{
  struct nation_type *pnation = nation_of_unit(punit);

  if (gui_options.draw_unit_shields) {
    return t->sprites.nation_shield.p[nation_index(pnation)];
  } else {
    return t->sprites.nation_flag.p[nation_index(pnation)];
  }
}

#define ADD_SPRITE_FULL(s)                                                  \
  sprs.emplace_back(t, s, true, FULL_TILE_X_OFFSET, FULL_TILE_Y_OFFSET)

/**
   Assemble some data that is used in building the tile sprite arrays.
     (map_x, map_y) : the (normalized) map position
   The values we fill in:
     tterrain_near  : terrain types of all adjacent terrain
     tspecial_near  : specials of all adjacent terrain
 */
void build_tile_data(const struct tile *ptile, struct terrain *pterrain,
                     struct terrain **tterrain_near, bv_extras *textras_near)
{
  int dir;

  // Loop over all adjacent tiles.  We should have an iterator for this.
  for (dir = 0; dir < 8; dir++) {
    struct tile *tile1 =
        mapstep(&(wld.map), ptile, static_cast<direction8>(dir));

    if (tile1 && client_tile_get_known(tile1) != TILE_UNKNOWN) {
      struct terrain *terrain1 = tile_terrain(tile1);

      if (NULL != terrain1) {
        tterrain_near[dir] = terrain1;
        textras_near[dir] = *tile_extras(tile1);
        continue;
      }
      qCritical("build_tile_data() tile (%d,%d) has no terrain!",
                TILE_XY(tile1));
    }
    /* At the edges of the (known) map, pretend the same terrain continued
     * past the edge of the map. */
    tterrain_near[dir] = pterrain;
    BV_CLR_ALL(textras_near[dir]);
  }
}

/**
   Fill in the sprite array for the unit type.
 */
static void fill_unit_type_sprite_array(const struct tileset *t,
                                        std::vector<drawn_sprite> &sprs,
                                        const struct unit_type *putype,
                                        enum direction8 facing)
{
  auto uspr = get_unittype_sprite(t, putype, facing);

  sprs.emplace_back(t, uspr, true, FULL_TILE_X_OFFSET + t->unit_offset_x,
                    FULL_TILE_Y_OFFSET + t->unit_offset_y);
}

/**
   Fill in the sprite array for the unit.
 */
static void fill_unit_sprite_array(const struct tileset *t,
                                   std::vector<drawn_sprite> &sprs,
                                   const struct unit *punit, bool stack,
                                   bool backdrop)
{
  int ihp;
  const struct unit_type *ptype = unit_type_get(punit);

  if (backdrop) {
    if (!gui_options.solid_color_behind_units) {
      sprs.emplace_back(t, get_unit_nation_flag_sprite(t, punit), true,
                        FULL_TILE_X_OFFSET + t->unit_flag_offset_x,
                        FULL_TILE_Y_OFFSET + t->unit_flag_offset_y);
    } else {
      // Taken care of in the LAYER_BACKGROUND.
    }
  }

  // Add the sprite for the unit type.
  fill_unit_type_sprite_array(t, sprs, ptype, punit->facing);

  if (t->sprites.unit.loaded && unit_transported(punit)) {
    ADD_SPRITE_FULL(t->sprites.unit.loaded);
  }

  if (punit->activity != ACTIVITY_IDLE) {
    QPixmap *s = NULL;

    switch (punit->activity) {
    case ACTIVITY_MINE:
      if (punit->activity_target == NULL) {
        s = t->sprites.unit.plant;
      } else {
        s = t->sprites.extras[extra_index(punit->activity_target)].activity;
      }
      break;
    case ACTIVITY_PLANT:
      s = t->sprites.unit.plant;
      break;
    case ACTIVITY_IRRIGATE:
      if (punit->activity_target == NULL) {
        s = t->sprites.unit.irrigate;
      } else {
        s = t->sprites.extras[extra_index(punit->activity_target)].activity;
      }
      break;
    case ACTIVITY_CULTIVATE:
      s = t->sprites.unit.irrigate;
      break;
    case ACTIVITY_POLLUTION:
    case ACTIVITY_FALLOUT:
      s = t->sprites.extras[extra_index(punit->activity_target)].rmact;
      break;
    case ACTIVITY_PILLAGE:
      s = t->sprites.unit.pillage;
      break;
    case ACTIVITY_EXPLORE:
      // Drawn below as the server side agent.
      break;
    case ACTIVITY_FORTIFIED:
      s = t->sprites.unit.fortified;
      break;
    case ACTIVITY_FORTIFYING:
      s = t->sprites.unit.fortifying;
      break;
    case ACTIVITY_SENTRY:
      s = t->sprites.unit.sentry;
      break;
    case ACTIVITY_GOTO:
      s = t->sprites.unit.go_to;
      break;
    case ACTIVITY_TRANSFORM:
      s = t->sprites.unit.transform;
      break;
    case ACTIVITY_BASE:
    case ACTIVITY_GEN_ROAD:
      s = t->sprites.extras[extra_index(punit->activity_target)].activity;
      break;
    case ACTIVITY_CONVERT:
      s = t->sprites.unit.convert;
      break;
    default:
      break;
    }

    if (s != NULL) {
      sprs.emplace_back(t, s, true,
                        FULL_TILE_X_OFFSET + t->activity_offset_x,
                        FULL_TILE_Y_OFFSET + t->activity_offset_y);
    }
  }

  {
    QPixmap *s = NULL;
    int offset_x = 0;
    int offset_y = 0;

    switch (punit->ssa_controller) {
    case SSA_NONE:
      break;
    case SSA_AUTOSETTLER:
      s = t->sprites.unit.auto_settler;
      break;
    case SSA_AUTOEXPLORE:
      s = t->sprites.unit.auto_explore;
      // Specified as an activity in the tileset.
      offset_x = t->activity_offset_x;
      offset_y = t->activity_offset_y;
      break;
    default:
      s = t->sprites.unit.auto_attack;
      break;
    }

    if (s != NULL) {
      sprs.emplace_back(t, s, true, FULL_TILE_X_OFFSET + offset_x,
                        FULL_TILE_Y_OFFSET + offset_y);
    }
  }

  if (unit_has_orders(punit)) {
    if (punit->orders.repeat) {
      ADD_SPRITE_FULL(t->sprites.unit.patrol);
    } else if (punit->activity != ACTIVITY_IDLE) {
      sprs.emplace_back(t, t->sprites.unit.connect);
    } else {
      sprs.emplace_back(t, t->sprites.unit.go_to, true,
                        FULL_TILE_X_OFFSET + t->activity_offset_x,
                        FULL_TILE_Y_OFFSET + t->activity_offset_y);
    }
  }

  if (t->sprites.unit.action_decision_want != NULL
      && should_ask_server_for_actions(punit)) {
    sprs.emplace_back(t, t->sprites.unit.action_decision_want, true,
                      FULL_TILE_X_OFFSET + t->activity_offset_x,
                      FULL_TILE_Y_OFFSET + t->activity_offset_y);
  }

  if (punit->battlegroup != BATTLEGROUP_NONE) {
    ADD_SPRITE_FULL(t->sprites.unit.battlegroup[punit->battlegroup]);
  }

  if (t->sprites.unit.lowfuel && utype_fuel(ptype) && punit->fuel == 1
      && punit->moves_left <= 2 * SINGLE_MOVE) {
    // Show a low-fuel graphic if the plane has 2 or fewer moves left.
    ADD_SPRITE_FULL(t->sprites.unit.lowfuel);
  }
  if (t->sprites.unit.tired && punit->moves_left < SINGLE_MOVE
      && ptype->move_rate > 0) {
    /* Show a "tired" graphic if the unit has fewer than one move
     * remaining, except for units for which it's full movement. */
    ADD_SPRITE_FULL(t->sprites.unit.tired);
  }

  if (stack || punit->client.occupied) {
    ADD_SPRITE_FULL(t->sprites.unit.stack);
  }

  if (t->sprites.unit.vet_lev[punit->veteran]) {
    ADD_SPRITE_FULL(t->sprites.unit.vet_lev[punit->veteran]);
  }

  ihp = ((t->sprites.unit.hp_bar.size() - 1) * punit->hp) / ptype->hp;
  ihp = CLIP(0, ihp, t->sprites.unit.hp_bar.size() - 1); // Safety
  ADD_SPRITE_FULL(t->sprites.unit.hp_bar[ihp]);
}

/**
   Add any corner road sprites to the sprite array.
 */
static void fill_road_corner_sprites(const struct tileset *t,
                                     const struct extra_type *pextra,
                                     std::vector<drawn_sprite> &sprs,
                                     bool road, bool *road_near, bool hider,
                                     bool *hider_near)
{
  int i;
  int extra_idx = extra_index(pextra);

  if (is_cardinal_only_road(pextra)) {
    return;
  }

  /* Roads going diagonally adjacent to this tile need to be
   * partly drawn on this tile. */

  /* Draw the corner sprite if:
   *   - There is a diagonal road (not rail!) between two adjacent tiles.
   *   - There is no diagonal road (not rail!) that intersects this road.
   * The logic is simple: roads are drawn underneath railrods, but are
   * not always covered by them (even in the corners!).  But if a railroad
   * connects two tiles, only the railroad (no road) is drawn between
   * those tiles.
   */
  for (i = 0; i < t->num_valid_tileset_dirs; i++) {
    enum direction8 dir = t->valid_tileset_dirs[i];

    if (!is_cardinal_tileset_dir(t, dir)) {
      // Draw corner sprites for this non-cardinal direction.
      int cw = (i + 1) % t->num_valid_tileset_dirs;
      int ccw =
          (i + t->num_valid_tileset_dirs - 1) % t->num_valid_tileset_dirs;
      enum direction8 cwdir = t->valid_tileset_dirs[cw];
      enum direction8 ccwdir = t->valid_tileset_dirs[ccw];

      if (t->sprites.extras[extra_idx].u.road.corner[dir]
          && (road_near[cwdir] && road_near[ccwdir]
              && !(hider_near[cwdir] && hider_near[ccwdir]))
          && !(road && road_near[dir] && !(hider && hider_near[dir]))) {
        sprs.emplace_back(t,
                          t->sprites.extras[extra_idx].u.road.corner[dir]);
      }
    }
  }
}

/**
   Fill all road and rail sprites into the sprite array.
 */
static void fill_road_sprite_array(const struct tileset *t,
                                   const struct extra_type *pextra,
                                   std::vector<drawn_sprite> &sprs,
                                   bv_extras textras,
                                   bv_extras *textras_near,
                                   struct terrain *tterrain_near[8],
                                   const struct city *pcity)
{
  bool road, road_near[8], hider, hider_near[8];
  bool land_near[8], hland_near[8];
  bool draw_road[8], draw_single_road;
  int dir;
  int extra_idx = -1;
  bool cl = false;
  int extrastyle;
  const struct road_type *proad = extra_road_get(pextra);

  extra_idx = extra_index(pextra);

  extrastyle = t->sprites.extras[extra_idx].extrastyle;

  if (extra_has_flag(pextra, EF_CONNECT_LAND)) {
    cl = true;
  } else {
    int i;

    for (i = 0; i < 8; i++) {
      land_near[i] = false;
    }
  }

  /* Fill some data arrays. rail_near and road_near store whether road/rail
   * is present in the given direction.  draw_rail and draw_road store
   * whether road/rail is to be drawn in that direction.  draw_single_road
   * and draw_single_rail store whether we need an isolated road/rail to be
   * drawn. */
  road = BV_ISSET(textras, extra_idx);

  hider = false;
  extra_type_list_iterate(pextra->hiders, phider)
  {
    if (BV_ISSET(textras, extra_index(phider))) {
      hider = true;
      break;
    }
  }
  extra_type_list_iterate_end;

  draw_single_road = road && (!pcity || !gui_options.draw_cities) && !hider;

  for (dir = 0; dir < 8; dir++) {
    bool roads_exist;

    /* Check if there is adjacent road/rail. */
    if (!is_cardinal_only_road(pextra)
        || is_cardinal_tileset_dir(t, static_cast<direction8>(dir))) {
      road_near[dir] = false;
      extra_type_list_iterate(proad->integrators, iextra)
      {
        if (BV_ISSET(textras_near[dir], extra_index(iextra))) {
          road_near[dir] = true;
          break;
        }
      }
      extra_type_list_iterate_end;
      if (cl) {
        land_near[dir] =
            (tterrain_near[dir] != T_UNKNOWN
             && terrain_type_terrain_class(tterrain_near[dir]) != TC_OCEAN);
      }
    } else {
      road_near[dir] = false;
      land_near[dir] = false;
    }

    /* Draw rail/road if there is a connection from this tile to the
     * adjacent tile.  But don't draw road if there is also a rail
     * connection. */
    roads_exist = road && (road_near[dir] || land_near[dir]);
    draw_road[dir] = roads_exist;
    hider_near[dir] = false;
    hland_near[dir] =
        tterrain_near[dir] != T_UNKNOWN
        && terrain_type_terrain_class(tterrain_near[dir]) != TC_OCEAN;
    extra_type_list_iterate(pextra->hiders, phider)
    {
      bool hider_dir = false;
      bool land_dir = false;

      if (!is_cardinal_only_road(phider)
          || is_cardinal_tileset_dir(t, static_cast<direction8>(dir))) {
        if (BV_ISSET(textras_near[dir], extra_index(phider))) {
          hider_near[dir] = true;
          hider_dir = true;
        }
        if (hland_near[dir] && is_extra_caused_by(phider, EC_ROAD)
            && extra_has_flag(phider, EF_CONNECT_LAND)) {
          land_dir = true;
        }
        if (hider_dir || land_dir) {
          if (BV_ISSET(textras, extra_index(phider))) {
            draw_road[dir] = false;
          }
        }
      }
    }
    extra_type_list_iterate_end;

    /* Don't draw an isolated road/rail if there's any connection.
     * draw_single_road would be true in the first place only if start tile
     * has road, so it will have road connection with any adjacent road
     * tile. We check from real existence of road (road_near[dir]) and not
     * from whether road gets drawn (draw_road[dir]) as latter can be FALSE
     * when road is simply hidden by another one, and we don't want to
     * draw single road in that case either. */
    if (draw_single_road && road_near[dir]) {
      draw_single_road = false;
    }
  }

  // Draw road corners
  fill_road_corner_sprites(t, pextra, sprs, road, road_near, hider,
                           hider_near);

  if (extrastyle == ESTYLE_ROAD_ALL_SEPARATE) {
    /* With ESTYLE_ROAD_ALL_SEPARATE, we simply draw one road for every
     * connection. This means we only need a few sprites, but a lot of
     * drawing is necessary and it generally doesn't look very good. */
    int i;

    // First draw roads under rails.
    if (road) {
      for (i = 0; i < t->num_valid_tileset_dirs; i++) {
        if (draw_road[t->valid_tileset_dirs[i]]) {
          sprs.emplace_back(t,
                            t->sprites.extras[extra_idx].u.road.ru.dir[i]);
        }
      }
    }
  } else if (extrastyle == ESTYLE_ROAD_PARITY_COMBINED) {
    /* With ESTYLE_ROAD_PARITY_COMBINED, we draw one sprite for cardinal
     * road connections, one sprite for diagonal road connections.
     * This means we need about 4x more sprites than in style 0, but up to
     * 4x less drawing is needed.  The drawing quality may also be
     * improved. */

    // First draw roads under rails.
    if (road) {
      int road_even_tileno = 0, road_odd_tileno = 0, i;

      for (i = 0; i < t->num_valid_tileset_dirs / 2; i++) {
        enum direction8 even = t->valid_tileset_dirs[2 * i];
        enum direction8 odd = t->valid_tileset_dirs[2 * i + 1];

        if (draw_road[even]) {
          road_even_tileno |= 1 << i;
        }
        if (draw_road[odd]) {
          road_odd_tileno |= 1 << i;
        }
      }

      /* Draw the cardinal/even roads first. */
      if (road_even_tileno != 0) {
        sprs.emplace_back(t, t->sprites.extras[extra_idx]
                                 .u.road.ru.combo.even[road_even_tileno]);
      }
      if (road_odd_tileno != 0) {
        sprs.emplace_back(t, t->sprites.extras[extra_idx]
                                 .u.road.ru.combo.odd[road_odd_tileno]);
      }
    }
  } else if (extrastyle == ESTYLE_ROAD_ALL_COMBINED) {
    /* RSTYLE_ALL_COMBINED is a very simple method that lets us simply
     * retrieve entire finished tiles, with a bitwise index of the presence
     * of roads in each direction. */

    // Draw roads first
    if (road) {
      int road_tileno = 0, i;

      for (i = 0; i < t->num_valid_tileset_dirs; i++) {
        enum direction8 vdir = t->valid_tileset_dirs[i];

        if (draw_road[vdir]) {
          road_tileno |= 1 << i;
        }
      }

      if (road_tileno != 0 || draw_single_road) {
        sprs.emplace_back(
            t, t->sprites.extras[extra_idx].u.road.ru.total[road_tileno]);
      }
    }
  } else {
    fc_assert(false);
  }

  /* Draw isolated rail/road separately (ESTYLE_ROAD_ALL_SEPARATE and
     ESTYLE_ROAD_PARITY_COMBINED only). */
  if (extrastyle == ESTYLE_ROAD_ALL_SEPARATE
      || extrastyle == ESTYLE_ROAD_PARITY_COMBINED) {
    if (draw_single_road) {
      sprs.emplace_back(t, t->sprites.extras[extra_idx].u.road.isolated);
    }
  }
}

/**
   Return the index of the sprite to be used for irrigation or farmland in
   this tile.

   We assume that the current tile has farmland or irrigation.  We then
   choose a sprite (index) based upon which cardinally adjacent tiles have
   either farmland or irrigation (the two are considered interchangable for
   this).
 */
static int get_irrigation_index(const struct tileset *t,
                                struct extra_type *pextra,
                                bv_extras *textras_near)
{
  int tileno = 0, i;

  for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
    enum direction8 dir = t->cardinal_tileset_dirs[i];

    if (BV_ISSET(textras_near[dir], extra_index(pextra))) {
      tileno |= 1 << i;
    }
  }

  return tileno;
}

/**
   Fill in the farmland/irrigation sprite for the tile.
 */
static void fill_irrigation_sprite_array(const struct tileset *t,
                                         std::vector<drawn_sprite> &sprs,
                                         bv_extras textras,
                                         bv_extras *textras_near,
                                         const struct city *pcity)
{
  /* We don't draw the irrigation if there's a city (it just gets overdrawn
   * anyway, and ends up looking bad). */
  if (!(pcity && gui_options.draw_cities)) {
    extra_type_list_iterate(t->style_lists[ESTYLE_CARDINALS], pextra)
    {
      if (is_extra_drawing_enabled(pextra)) {
        int eidx = extra_index(pextra);

        if (BV_ISSET(textras, eidx)) {
          bool hidden = false;

          extra_type_list_iterate(pextra->hiders, phider)
          {
            if (BV_ISSET(textras, extra_index(phider))) {
              hidden = true;
              break;
            }
          }
          extra_type_list_iterate_end;

          if (!hidden) {
            int idx = get_irrigation_index(t, pextra, textras_near);

            sprs.emplace_back(t, t->sprites.extras[eidx].u.cardinals[idx]);
          }
        }
      }
    }
    extra_type_list_iterate_end;
  }
}

/**
   Fill in the city overlays for the tile.  This includes the citymap
   overlays on the mapview as well as the tile output sprites.
 */
static void fill_city_overlays_sprite_array(const struct tileset *t,
                                            std::vector<drawn_sprite> &sprs,
                                            const struct tile *ptile,
                                            const struct city *citymode)
{
  const struct city *pcity;
  const struct city *pwork;
  struct unit *psettler = nullptr;
  int city_x, city_y;
  const int NUM_CITY_COLORS = t->sprites.city.worked_tile_overlay.size;

  if (NULL == ptile || TILE_UNKNOWN == client_tile_get_known(ptile)) {
    return;
  }
  pwork = tile_worked(ptile);

  if (citymode) {
    pcity = citymode;
  } else {
    pcity = find_city_or_settler_near_tile(ptile, &psettler);
  }

  /* Below code does not work if pcity is invisible.
   * Make sure it is not. */
  fc_assert_ret(pcity == NULL || pcity->tile != NULL);
  if (pcity && !pcity->tile) {
    pcity = NULL;
  }

  if (pcity && city_base_to_city_map(&city_x, &city_y, pcity, ptile)) {
    // FIXME: check elsewhere for valid tile (instead of above)
    if (!citymode && pcity->client.colored) {
      // Add citymap overlay for a city.
      int idx = pcity->client.color_index % NUM_CITY_COLORS;

      if (NULL != pwork && pwork == pcity) {
        sprs.emplace_back(t, t->sprites.city.worked_tile_overlay.p[idx]);
      } else if (city_can_work_tile(pcity, ptile)) {
        sprs.emplace_back(t, t->sprites.city.unworked_tile_overlay.p[idx]);
      }
    } else if (NULL != pwork && pwork == pcity
               && (citymode || gui_options.draw_city_output
                   || pcity->client.city_opened)) {
      // Add on the tile output sprites.
      const int ox = t->type == TS_ISOMETRIC ? t->normal_tile_width / 3 : 0;
      const int oy =
          t->type == TS_ISOMETRIC ? -t->normal_tile_height / 3 : 0;

      if (!t->sprites.city.tile_foodnum.empty()) {
        int food = city_tile_output_now(pcity, ptile, O_FOOD);
        food = CLIP(0, food / game.info.granularity,
                    t->sprites.city.tile_foodnum.size() - 1);
        sprs.emplace_back(
            t, const_cast<QPixmap *>(&t->sprites.city.tile_foodnum[food]),
            true, ox, oy);
      }
      if (!t->sprites.city.tile_shieldnum.empty()) {
        int shields = city_tile_output_now(pcity, ptile, O_SHIELD);
        shields = CLIP(0, shields / game.info.granularity,
                       t->sprites.city.tile_shieldnum.size() - 1);
        sprs.emplace_back(
            t,
            const_cast<QPixmap *>(&t->sprites.city.tile_shieldnum[shields]),
            true, ox, oy);
      }
      if (!t->sprites.city.tile_tradenum.empty()) {
        int trade = city_tile_output_now(pcity, ptile, O_TRADE);
        trade = CLIP(0, trade / game.info.granularity,
                     t->sprites.city.tile_tradenum.size() - 1);
        sprs.emplace_back(
            t, const_cast<QPixmap *>(&t->sprites.city.tile_tradenum[trade]),
            true, ox, oy);
      }
    }
  } else if (psettler && psettler->client.colored) {
    // Add citymap overlay for a unit.
    int idx = psettler->client.color_index % NUM_CITY_COLORS;

    sprs.emplace_back(t, t->sprites.city.unworked_tile_overlay.p[idx]);
  }
}

/**
   Add sprites for fog (and some forms of darkness).
 */
static void fill_fog_sprite_array(const struct tileset *t,
                                  std::vector<drawn_sprite> &sprs,
                                  const struct tile *ptile,
                                  const struct tile_edge *pedge,
                                  const struct tile_corner *pcorner)
{
  if (t->fogstyle == FOG_SPRITE && gui_options.draw_fog_of_war
      && NULL != ptile
      && TILE_KNOWN_UNSEEN == client_tile_get_known(ptile)) {
    // With FOG_AUTO, fog is done this way.
    sprs.emplace_back(t, t->sprites.tx.fog);
  }

  if (t->darkness_layer->style() == freeciv::DARKNESS_CORNER && pcorner
      && gui_options.draw_fog_of_war) {
    int i, tileno = 0;

    for (i = 3; i >= 0; i--) {
      const int unknown = 0, fogged = 1, known = 2;
      int value = -1;

      if (!pcorner->tile[i]) {
        value = fogged;
      } else {
        switch (client_tile_get_known(pcorner->tile[i])) {
        case TILE_KNOWN_SEEN:
          value = known;
          break;
        case TILE_KNOWN_UNSEEN:
          value = fogged;
          break;
        case TILE_UNKNOWN:
          value = unknown;
          break;
        }
      }
      fc_assert(value >= 0 && value < 3);

      tileno = tileno * 3 + value;
    }

    if (t->sprites.tx.fullfog[tileno]) {
      sprs.emplace_back(t, t->sprites.tx.fullfog[tileno]);
    }
  }
}

/**
   Indicate whether a unit is to be drawn with a surrounding city outline
   under current conditions.
   (This includes being in focus, but if the caller has already checked
 that, they can bypass this slightly expensive check with check_focus ==
 FALSE.)
 */
bool unit_drawn_with_city_outline(const struct unit *punit, bool check_focus)
{
  /* Display an outline for city-builder type units if they are selected,
   * and on a tile where a city can be built.
   * But suppress the outline if the unit has orders (likely it is in
   * transit to somewhere else and this will just slow down redraws). */
  return gui_options.draw_city_outlines && unit_is_cityfounder(punit)
         && !unit_has_orders(punit)
         && (client_tile_get_known(unit_tile(punit)) != TILE_UNKNOWN
             && city_can_be_built_here(unit_tile(punit), punit))
         && (!check_focus || unit_is_in_focus(punit));
}

/**
   Fill in the grid sprites for the given tile, city, and unit.
 */
static void fill_grid_sprite_array(const struct tileset *t,
                                   std::vector<drawn_sprite> &sprs,
                                   const struct tile *ptile,
                                   const struct tile_edge *pedge,
                                   const struct city *citymode)
{
  if (pedge) {
    bool known[NUM_EDGE_TILES], city[NUM_EDGE_TILES];
    bool unit[NUM_EDGE_TILES], worked[NUM_EDGE_TILES];
    int i;
    struct unit_list *pfocus_units = get_units_in_focus();

    for (i = 0; i < NUM_EDGE_TILES; i++) {
      int dummy_x, dummy_y;
      const struct tile *tile = pedge->tile[i];
      struct player *powner = tile ? tile_owner(tile) : NULL;

      known[i] = tile && client_tile_get_known(tile) != TILE_UNKNOWN;
      unit[i] = false;
      if (tile && !citymode) {
        unit_list_iterate(pfocus_units, pfocus_unit)
        {
          if (unit_drawn_with_city_outline(pfocus_unit, false)) {
            struct tile *utile = unit_tile(pfocus_unit);
            int radius =
                game.info.init_city_radius_sq
                + get_target_bonus_effects(
                    NULL, unit_owner(pfocus_unit), NULL, NULL, NULL, utile,
                    NULL, NULL, NULL, NULL, NULL, EFT_CITY_RADIUS_SQ);

            if (city_tile_to_city_map(&dummy_x, &dummy_y, radius, utile,
                                      tile)) {
              unit[i] = true;
              break;
            }
          }
        }
        unit_list_iterate_end;
      }
      worked[i] = false;

      city[i] = (tile
                 && (NULL == powner || NULL == client.conn.playing
                     || powner == client.conn.playing)
                 && player_in_city_map(client.conn.playing, tile));
      if (city[i]) {
        if (citymode) {
          /* In citymode, we only draw worked tiles for this city - other
           * tiles may be marked as unavailable. */
          worked[i] = (tile_worked(tile) == citymode);
        } else {
          worked[i] = (NULL != tile_worked(tile));
        }
      }
      // Draw city grid for main citymap
      if (tile && citymode
          && city_base_to_city_map(&dummy_x, &dummy_y, citymode, tile)) {
        sprs.emplace_back(t, t->sprites.grid.selected[pedge->type]);
      }
    }
    if (mapdeco_is_highlight_set(pedge->tile[0])
        || mapdeco_is_highlight_set(pedge->tile[1])) {
      sprs.emplace_back(t, t->sprites.grid.selected[pedge->type]);
    } else {
      if (gui_options.draw_map_grid) {
        if (worked[0] || worked[1]) {
          sprs.emplace_back(t, t->sprites.grid.worked[pedge->type]);
        } else if (city[0] || city[1]) {
          sprs.emplace_back(t, t->sprites.grid.city[pedge->type]);
        } else if (known[0] || known[1]) {
          sprs.emplace_back(t, t->sprites.grid.main[pedge->type]);
        }
      }
      if (gui_options.draw_city_outlines) {
        if (XOR(city[0], city[1])) {
          sprs.emplace_back(t, t->sprites.grid.city[pedge->type]);
        }
        if (XOR(unit[0], unit[1])) {
          sprs.emplace_back(t, t->sprites.grid.worked[pedge->type]);
        }
      }
    }

    if (gui_options.draw_borders && BORDERS_DISABLED != game.info.borders
        && known[0] && known[1]) {
      struct player *owner0 = tile_owner(pedge->tile[0]);
      struct player *owner1 = tile_owner(pedge->tile[1]);

      if (owner0 != owner1) {
        if (owner0) {
          int plrid = player_index(owner0);
          sprs.emplace_back(
              t, t->sprites.player[plrid].grid_borders[pedge->type][0]);
        }
        if (owner1) {
          int plrid = player_index(owner1);
          sprs.emplace_back(
              t, t->sprites.player[plrid].grid_borders[pedge->type][1]);
        }
      }
    }
  } else if (NULL != ptile && TILE_UNKNOWN != client_tile_get_known(ptile)) {
    int cx, cy;

    if (citymode
        // test to ensure valid coordinates?
        && city_base_to_city_map(&cx, &cy, citymode, ptile)
        && !client_city_can_work_tile(citymode, ptile)) {
      sprs.emplace_back(t, t->sprites.grid.unavailable);
    }

    if (gui_options.draw_native && citymode == NULL) {
      bool native = true;
      struct unit_list *pfocus_units = get_units_in_focus();

      unit_list_iterate(pfocus_units, pfocus)
      {
        if (!is_native_tile(unit_type_get(pfocus), ptile)) {
          native = false;
          break;
        }
      }
      unit_list_iterate_end;

      if (!native) {
        if (t->sprites.grid.nonnative != NULL) {
          sprs.emplace_back(t, t->sprites.grid.nonnative);
        } else {
          sprs.emplace_back(t, t->sprites.grid.unavailable);
        }
      }
    }
  }
}

/**
   Fill in the given sprite array with any needed goto sprites.
 */
static void fill_goto_sprite_array(const struct tileset *t,
                                   std::vector<drawn_sprite> &sprs,
                                   const struct tile *ptile,
                                   const struct tile_edge *pedge,
                                   const struct tile_corner *pcorner)
{
  QPixmap *sprite;
  bool warn = false;
  enum goto_tile_state state;
  int length;
  bool waypoint;

  if (goto_tile_state(ptile, &state, &length, &waypoint)) {
    if (length >= 0) {
      fc_assert(state >= 0);
      fc_assert(state < ARRAY_SIZE(t->sprites.path.s));

      sprite = t->sprites.path.s[state].specific;
      if (sprite != NULL) {
        sprs.emplace_back(t, sprite, false, 0, 0);
      }

      sprite = t->sprites.path.s[state].turns[length % 10];
      sprs.emplace_back(t, sprite);
      if (length >= 10) {
        sprite = t->sprites.path.s[state].turns_tens[(length / 10) % 10];
        sprs.emplace_back(t, sprite);
        if (length >= 100) {
          sprite =
              t->sprites.path.s[state].turns_hundreds[(length / 100) % 10];

          if (sprite != NULL) {
            sprs.emplace_back(t, sprite);
            if (length >= 1000) {
              warn = true;
            }
          } else {
            warn = true;
          }
        }
      }
    }

    if (waypoint) {
      sprs.emplace_back(t, t->sprites.path.waypoint, false, 0, 0);
    }

    if (warn) {
      // Warn only once by tileset.
      static char last_reported[256] = "";

      if (0 != strcmp(last_reported, t->name)) {
        qInfo(_("Tileset \"%s\" doesn't support long goto paths, "
                "such as %d. Path not displayed as expected."),
              t->name, length);
        sz_strlcpy(last_reported, t->name);
      }
    }
  }
}

/**
   Should the given extra be drawn
   FIXME: Some extras can not be switched
 */
bool is_extra_drawing_enabled(struct extra_type *pextra)
{
  bool no_disable = true; // Draw if matches no cause

  if (is_extra_caused_by(pextra, EC_IRRIGATION)) {
    if (gui_options.draw_irrigation) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_POLLUTION)
      || is_extra_caused_by(pextra, EC_FALLOUT)) {
    if (gui_options.draw_pollution) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_MINE)) {
    if (gui_options.draw_mines) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_RESOURCE)) {
    if (gui_options.draw_specials) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_removed_by(pextra, ERM_ENTER)) {
    if (gui_options.draw_huts) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_BASE)) {
    if (gui_options.draw_fortress_airbase) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_ROAD)) {
    if (gui_options.draw_roads_rails) {
      return true;
    }
    no_disable = false;
  }

  return no_disable;
}

/**
   Fill in the sprite array for the given tile, city, and unit.

   ptile, if specified, gives the tile.  If specified the terrain and
 specials will be drawn for this tile.  In this case (map_x,map_y) should
 give the location of the tile.

   punit, if specified, gives the unit.  For tile drawing this should
   generally be get_drawable_unit(); otherwise it can be any unit.

   pcity, if specified, gives the city.  For tile drawing this should
   generally be tile_city(ptile); otherwise it can be any city.
 */
std::vector<drawn_sprite>
fill_sprite_array(struct tileset *t, enum mapview_layer layer,
                  const struct tile *ptile, const struct tile_edge *pedge,
                  const struct tile_corner *pcorner,
                  const struct unit *punit, const struct city *pcity,
                  const struct unit_type *putype)
{
  int tileno, dir;
  bv_extras textras_near[8]{};
  bv_extras textras;
  struct terrain *tterrain_near[8] = {nullptr};
  struct terrain *pterrain = NULL;
  /* Unit drawing is disabled when the view options are turned off,
   * but only where we're drawing on the mapview. */
  bool do_draw_unit =
      (punit
       && (gui_options.draw_units || !ptile
           || (gui_options.draw_focus_unit && unit_is_in_focus(punit))));
  bool solid_bg = (gui_options.solid_color_behind_units
                   && (do_draw_unit || (pcity && gui_options.draw_cities)));

  const city *citymode = is_any_city_dialog_open();

  if (ptile && client_tile_get_known(ptile) != TILE_UNKNOWN) {
    textras = *tile_extras(ptile);
    pterrain = tile_terrain(ptile);

    if (NULL != pterrain) {
      if (layer == LAYER_TERRAIN1 || layer == LAYER_TERRAIN2
          || layer == LAYER_TERRAIN3 || layer == LAYER_WATER
          || layer == LAYER_ROADS) {
        build_tile_data(ptile, pterrain, tterrain_near, textras_near);
      }
    } else if (!tile_virtual_check(ptile)) {
      qCritical("fill_sprite_array() tile (%d,%d) has no terrain!",
                TILE_XY(ptile));
    }
  } else {
    BV_CLR_ALL(textras);
  }

  auto sprs = std::vector<drawn_sprite>();
  sprs.reserve(80);

  switch (layer) {
  case LAYER_BACKGROUND:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_TERRAIN1:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_DARKNESS:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_TERRAIN2:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_TERRAIN3:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_WATER:
    if (NULL != pterrain) {
      if (!solid_bg && terrain_type_terrain_class(pterrain) == TC_OCEAN) {
        for (dir = 0; dir < t->num_cardinal_tileset_dirs; dir++) {
          int didx = t->cardinal_tileset_dirs[dir];

          extra_type_list_iterate(t->style_lists[ESTYLE_RIVER], priver)
          {
            int idx = extra_index(priver);

            if (BV_ISSET(textras_near[didx], idx)) {
              sprs.emplace_back(
                  t, t->sprites.extras[idx].u.road.ru.rivers.outlet[dir]);
            }
          }
          extra_type_list_iterate_end;
        }
      }
    }

    fill_irrigation_sprite_array(t, sprs, textras, textras_near, pcity);

    if (!solid_bg) {
      extra_type_list_iterate(t->style_lists[ESTYLE_RIVER], priver)
      {
        int idx = extra_index(priver);

        if (BV_ISSET(textras, idx)) {
          int i;

          // Draw rivers on top of irrigation.
          tileno = 0;
          for (i = 0; i < t->num_cardinal_tileset_dirs; i++) {
            enum direction8 cdir = t->cardinal_tileset_dirs[i];

            if (tterrain_near[cdir] == nullptr
                || terrain_type_terrain_class(tterrain_near[cdir])
                       == TC_OCEAN
                || BV_ISSET(textras_near[cdir], idx)) {
              tileno |= 1 << i;
            }
          }

          sprs.emplace_back(
              t, t->sprites.extras[idx].u.road.ru.rivers.spec[tileno]);
        }
      }
      extra_type_list_iterate_end;
    }
    break;

  case LAYER_ROADS:
    extra_type_list_iterate(t->style_lists[ESTYLE_ROAD_ALL_SEPARATE], pextra)
    {
      if (is_extra_drawing_enabled(pextra)) {
        fill_road_sprite_array(t, pextra, sprs, textras, textras_near,
                               tterrain_near, pcity);
      }
    }
    extra_type_list_iterate_end;
    extra_type_list_iterate(t->style_lists[ESTYLE_ROAD_PARITY_COMBINED],
                            pextra)
    {
      if (is_extra_drawing_enabled(pextra)) {
        fill_road_sprite_array(t, pextra, sprs, textras, textras_near,
                               tterrain_near, pcity);
      }
    }
    extra_type_list_iterate_end;
    extra_type_list_iterate(t->style_lists[ESTYLE_ROAD_ALL_COMBINED], pextra)
    {
      if (is_extra_drawing_enabled(pextra)) {
        fill_road_sprite_array(t, pextra, sprs, textras, textras_near,
                               tterrain_near, pcity);
      }
    }
    extra_type_list_iterate_end;
    break;

  case LAYER_SPECIAL1:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_GRID1:
    if (t->type == TS_ISOMETRIC) {
      fill_grid_sprite_array(t, sprs, ptile, pedge, citymode);
    }
    break;

  case LAYER_CITY1:
    // City.  Some city sprites are drawn later.
    if (pcity && gui_options.draw_cities) {
      if (!citybar_painter::current()->has_flag()) {
        sprs.emplace_back(t, get_city_flag_sprite(t, pcity), true,
                          FULL_TILE_X_OFFSET + t->city_flag_offset_x,
                          FULL_TILE_Y_OFFSET + t->city_flag_offset_y);
      }
      /* For iso-view the city.wall graphics include the full city, whereas
       * for non-iso view they are an overlay on top of the base city
       * graphic. */
      if (t->type == TS_OVERHEAD || pcity->client.walls <= 0) {
        sprs.emplace_back(t, get_city_sprite(t->sprites.city.tile, pcity),
                          true, FULL_TILE_X_OFFSET + t->city_offset_x,
                          FULL_TILE_Y_OFFSET + t->city_offset_y);
      }
      if (t->type == TS_ISOMETRIC && pcity->client.walls > 0) {
        struct city_sprite *cspr =
            t->sprites.city.wall[pcity->client.walls - 1];
        QPixmap *spr = NULL;

        if (cspr != NULL) {
          spr = get_city_sprite(cspr, pcity);
        }
        if (spr == NULL) {
          cspr = t->sprites.city.single_wall;
          if (cspr != NULL) {
            spr = get_city_sprite(cspr, pcity);
          }
        }

        if (spr != NULL) {
          sprs.emplace_back(t, spr, true,
                            FULL_TILE_X_OFFSET + t->city_offset_x,
                            FULL_TILE_Y_OFFSET + t->city_offset_y);
        }
      }
      if (!citybar_painter::current()->has_units()
          && pcity->client.occupied) {
        sprs.emplace_back(t,
                          get_city_sprite(t->sprites.city.occupied, pcity),
                          true, FULL_TILE_X_OFFSET + t->occupied_offset_x,
                          FULL_TILE_Y_OFFSET + t->occupied_offset_y);
      }
      if (t->type == TS_OVERHEAD && pcity->client.walls > 0) {
        struct city_sprite *cspr =
            t->sprites.city.wall[pcity->client.walls - 1];
        QPixmap *spr = NULL;

        if (cspr != NULL) {
          spr = get_city_sprite(cspr, pcity);
        }
        if (spr == NULL) {
          cspr = t->sprites.city.single_wall;
          if (cspr != NULL) {
            spr = get_city_sprite(cspr, pcity);
          }
        }

        if (spr != NULL) {
          ADD_SPRITE_FULL(spr);
        }
      }
      if (pcity->client.unhappy) {
        ADD_SPRITE_FULL(t->sprites.city.disorder);
      }
    }
    break;

  case LAYER_SPECIAL2:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_UNIT:
  case LAYER_FOCUS_UNIT:
    if (do_draw_unit && XOR(layer == LAYER_UNIT, unit_is_in_focus(punit))) {
      bool stacked = ptile && (unit_list_size(ptile->units) > 1);
      bool backdrop = !pcity;

      if (ptile && unit_is_in_focus(punit) && t->sprites.unit.select[0]) {
        /* Special case for drawing the selection rectangle.  The blinking
         * unit is handled separately, inside get_drawable_unit(). */
        sprs.emplace_back(t, t->sprites.unit.select[focus_unit_state], true,
                          t->select_offset_x, t->select_offset_y);
      }

      fill_unit_sprite_array(t, sprs, punit, stacked, backdrop);
    } else if (putype != NULL && layer == LAYER_UNIT) {
      // Only the sprite for the unit type.
      fill_unit_type_sprite_array(t, sprs, putype, direction8_invalid());
    }
    break;

  case LAYER_SPECIAL3:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_BASE_FLAGS:
    fc_assert_ret_val(false, {});
    break;

  case LAYER_FOG:
    fill_fog_sprite_array(t, sprs, ptile, pedge, pcorner);
    break;

  case LAYER_CITY2:
    // City size.  Drawing this under fog makes it hard to read.
    if (pcity && gui_options.draw_cities
        && !citybar_painter::current()->has_size()) {
      bool warn = false;
      unsigned int size = city_size_get(pcity);

      sprs.emplace_back(t, t->sprites.city.size[size % 10], false,
                        FULL_TILE_X_OFFSET + t->city_size_offset_x,
                        FULL_TILE_Y_OFFSET + t->city_size_offset_y);
      if (10 <= size) {
        sprs.emplace_back(t, t->sprites.city.size_tens[(size / 10) % 10],
                          false, FULL_TILE_X_OFFSET + t->city_size_offset_x,
                          FULL_TILE_Y_OFFSET + t->city_size_offset_y);
        if (100 <= size) {
          QPixmap *sprite = t->sprites.city.size_hundreds[(size / 100) % 10];

          if (NULL != sprite) {
            sprs.emplace_back(t, sprite, false,
                              FULL_TILE_X_OFFSET + t->city_size_offset_x,
                              FULL_TILE_Y_OFFSET + t->city_size_offset_y);
          } else {
            warn = true;
          }
          if (1000 <= size) {
            warn = true;
          }
        }
      }

      if (warn) {
        // Warn only once by tileset.
        static char last_reported[256] = "";

        if (0 != strcmp(last_reported, t->name)) {
          qInfo(_("Tileset \"%s\" doesn't support big cities size, "
                  "such as %d. Size not displayed as expected."),
                t->name, size);
          sz_strlcpy(last_reported, t->name);
        }
      }
    }
    break;

  case LAYER_GRID2:
    if (t->type == TS_OVERHEAD) {
      fill_grid_sprite_array(t, sprs, ptile, pedge, citymode);
    }
    break;

  case LAYER_OVERLAYS:
    fill_city_overlays_sprite_array(t, sprs, ptile, citymode);
    if (mapdeco_is_crosshair_set(ptile)) {
      sprs.emplace_back(t, t->sprites.user.attention);
    }
    break;

  case LAYER_CITYBAR:
  case LAYER_TILELABEL:
    // Nothing.  This is just a placeholder.
    break;

  case LAYER_GOTO:
    if (ptile && goto_is_active()) {
      fill_goto_sprite_array(t, sprs, ptile, pedge, pcorner);
    }
    break;

  case LAYER_WORKERTASK:
    if (citymode != NULL && ptile != NULL) {
      worker_task_list_iterate(citymode->task_reqs, ptask)
      {
        if (ptask->ptile == ptile) {
          switch (ptask->act) {
          case ACTIVITY_MINE:
            if (ptask->tgt == NULL) {
              sprs.emplace_back(t, t->sprites.unit.plant, true,
                                FULL_TILE_X_OFFSET + t->activity_offset_x,
                                FULL_TILE_Y_OFFSET + t->activity_offset_y);
            } else {
              sprs.emplace_back(
                  t, t->sprites.extras[extra_index(ptask->tgt)].activity,
                  true, FULL_TILE_X_OFFSET + t->activity_offset_x,
                  FULL_TILE_Y_OFFSET + t->activity_offset_y);
            }
            break;
          case ACTIVITY_PLANT:
            sprs.emplace_back(t, t->sprites.unit.plant, true,
                              FULL_TILE_X_OFFSET + t->activity_offset_x,
                              FULL_TILE_Y_OFFSET + t->activity_offset_y);
            break;
          case ACTIVITY_IRRIGATE:
            if (ptask->tgt == NULL) {
              sprs.emplace_back(t, t->sprites.unit.irrigate, true,
                                FULL_TILE_X_OFFSET + t->activity_offset_x,
                                FULL_TILE_Y_OFFSET + t->activity_offset_y);
            } else {
              sprs.emplace_back(
                  t, t->sprites.extras[extra_index(ptask->tgt)].activity,
                  true, FULL_TILE_X_OFFSET + t->activity_offset_x,
                  FULL_TILE_Y_OFFSET + t->activity_offset_y);
            }
            break;
          case ACTIVITY_CULTIVATE:
            sprs.emplace_back(t, t->sprites.unit.irrigate, true,
                              FULL_TILE_X_OFFSET + t->activity_offset_x,
                              FULL_TILE_Y_OFFSET + t->activity_offset_y);
            break;
          case ACTIVITY_GEN_ROAD:
            sprs.emplace_back(
                t, t->sprites.extras[extra_index(ptask->tgt)].activity, true,
                FULL_TILE_X_OFFSET + t->activity_offset_x,
                FULL_TILE_Y_OFFSET + t->activity_offset_y);
            break;
          case ACTIVITY_TRANSFORM:
            sprs.emplace_back(t, t->sprites.unit.transform, true,
                              FULL_TILE_X_OFFSET + t->activity_offset_x,
                              FULL_TILE_Y_OFFSET + t->activity_offset_y);
            break;
          case ACTIVITY_POLLUTION:
          case ACTIVITY_FALLOUT:
            sprs.emplace_back(
                t, t->sprites.extras[extra_index(ptask->tgt)].rmact, true,
                FULL_TILE_X_OFFSET + t->activity_offset_x,
                FULL_TILE_Y_OFFSET + t->activity_offset_y);
            break;
          default:
            break;
          }
        }
      }
      worker_task_list_iterate_end;
    }
    break;

  case LAYER_EDITOR:
    if (ptile && editor_is_active()) {
      if (editor_tile_is_selected(ptile)) {
        int color = 2 % tileset_num_city_colors(tileset);
        sprs.emplace_back(t, t->sprites.city.unworked_tile_overlay.p[color]);
      }

      if (NULL != map_startpos_get(ptile)) {
        // FIXME: Use a more representative sprite.
        sprs.emplace_back(t, t->sprites.user.attention);
      }
    }
    break;

  case LAYER_INFRAWORK:
    if (ptile != NULL && ptile->placing != NULL) {
      const int id = extra_index(ptile->placing);

      if (t->sprites.extras[id].activity != NULL) {
        sprs.emplace_back(t, t->sprites.extras[id].activity, true,
                          FULL_TILE_X_OFFSET + t->activity_offset_x,
                          FULL_TILE_Y_OFFSET + t->activity_offset_y);
      }
    }
    break;

  case LAYER_COUNT:
    fc_assert(false);
    break;
  }

  return sprs;
}

/**
   Set city tiles sprite values; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_city_tiles(struct tileset *t, int style)
{
  if (style == game.control.styles_count - 1) {
    int i;

    // Free old sprites
    free_city_sprite(t->sprites.city.tile);

    for (i = 0; i < NUM_WALL_TYPES; i++) {
      free_city_sprite(t->sprites.city.wall[i]);
      t->sprites.city.wall[i] = NULL;
    }
    free_city_sprite(t->sprites.city.single_wall);
    t->sprites.city.single_wall = NULL;

    free_city_sprite(t->sprites.city.occupied);

    t->sprites.city.tile = load_city_sprite(t, QStringLiteral("city"));

    for (i = 0; i < NUM_WALL_TYPES; i++) {
      QString buffer;

      buffer = QStringLiteral("bldg_%1").arg(QString::number(i));
      t->sprites.city.wall[i] = load_city_sprite(t, buffer);
    }
    t->sprites.city.single_wall =
        load_city_sprite(t, QStringLiteral("wall"));

    t->sprites.city.occupied =
        load_city_sprite(t, QStringLiteral("occupied"));

    for (style = 0; style < game.control.styles_count; style++) {
      if (t->sprites.city.tile->styles[style].land_num_thresholds == 0) {
        tileset_error(t, LOG_FATAL,
                      _("City style \"%s\": no city graphics."),
                      city_style_rule_name(style));
      }
      if (t->sprites.city.occupied->styles[style].land_num_thresholds == 0) {
        tileset_error(t, LOG_FATAL,
                      _("City style \"%s\": no occupied graphics."),
                      city_style_rule_name(style));
      }
    }
  }
}

/**
   Return the amount of time between calls to toggle_focus_unit_state.
   The main loop needs to call toggle_focus_unit_state about this often
   to do the active-unit animation.
 */
int get_focus_unit_toggle_timeout(const struct tileset *t)
{
  if (t->sprites.unit.select.empty()) {
    return 100;
  } else {
    return t->select_step_ms;
  }
}

/**
   Reset the focus unit state.  This should be called when changing
   focus units.
 */
void reset_focus_unit_state(struct tileset *t) { focus_unit_state = 0; }

/**
   Setup tileset for showing combat where focus unit participates.
 */
void focus_unit_in_combat(struct tileset *t)
{
  if (t->sprites.unit.select.empty()) {
    reset_focus_unit_state(t);
  }
}

/**
   Toggle/increment the focus unit state.  This should be called once
   every get_focus_unit_toggle_timeout() seconds.
 */
void toggle_focus_unit_state(struct tileset *t)
{
  focus_unit_state++;
  if (t->sprites.unit.select.empty()) {
    focus_unit_state %= 2;
  } else {
    focus_unit_state %= t->sprites.unit.select.size();
  }
}

/**
   Find unit that we can display from given tile.
 */
struct unit *get_drawable_unit(const struct tileset *t, const ::tile *ptile)
{
  struct unit *punit = find_visible_unit(ptile);

  if (punit == NULL) {
    return NULL;
  }

  if (!unit_is_in_focus(punit) || t->sprites.unit.select[0]
      || focus_unit_state == 0) {
    return punit;
  } else {
    return NULL;
  }
}

/**
   This patch unloads all sprites from the sprite hash (the hash itself
   is left intact).
 */
static void unload_all_sprites(struct tileset *t)
{
  if (t->sprite_hash) {
    QHash<QString, small_sprite *>::const_iterator i =
        t->sprite_hash->constBegin();
    while (i != t->sprite_hash->constEnd()) {
      QByteArray qba = i.key().toLocal8Bit();
      while (i.value()->ref_count > 0) {
        unload_sprite(t, qba.data());
      }
      i++;
    }
  }
}

/**
   Free all sprites from tileset.
 */
void tileset_free_tiles(struct tileset *t)
{
  int i;

  log_debug("tileset_free_tiles()");

  unload_all_sprites(t);

  free_city_sprite(t->sprites.city.tile);
  t->sprites.city.tile = NULL;

  for (i = 0; i < NUM_WALL_TYPES; i++) {
    free_city_sprite(t->sprites.city.wall[i]);
    t->sprites.city.wall[i] = NULL;
  }
  free_city_sprite(t->sprites.city.single_wall);
  t->sprites.city.single_wall = NULL;

  free_city_sprite(t->sprites.city.occupied);
  t->sprites.city.occupied = NULL;

  if (t->sprite_hash) {
    delete t->sprite_hash;
    t->sprite_hash = NULL;
  }

  for (auto *ss : qAsConst(*t->small_sprites)) {
    if (ss->file) {
      delete[] ss->file;
    }
    fc_assert(ss->sprite == NULL);
    delete ss;
  }
  t->small_sprites->clear();

  for (auto *sf : qAsConst(*t->specfiles)) {
    delete[] sf->file_name;
    if (sf->big_sprite) {
      free_sprite(sf->big_sprite);
      sf->big_sprite = NULL;
    }
    delete sf;
  }
  t->specfiles->clear();

  sprite_vector_iterate(&t->sprites.city.worked_tile_overlay, psprite)
  {
    free_sprite(*psprite);
  }
  sprite_vector_iterate_end;
  sprite_vector_free(&t->sprites.city.worked_tile_overlay);

  sprite_vector_iterate(&t->sprites.city.unworked_tile_overlay, psprite)
  {
    free_sprite(*psprite);
  }
  sprite_vector_iterate_end;
  sprite_vector_free(&t->sprites.city.unworked_tile_overlay);

  if (t->sprites.tx.fullfog) {
    free(t->sprites.tx.fullfog);
    t->sprites.tx.fullfog = NULL;
  }

  sprite_vector_free(&t->sprites.colors.overlays);
  sprite_vector_free(&t->sprites.explode.unit);
  sprite_vector_free(&t->sprites.nation_flag);
  sprite_vector_free(&t->sprites.nation_shield);
  sprite_vector_free(&t->sprites.citybar.occupancy);
}

/**
   Return the sprite for drawing the given spaceship part.
 */
const QPixmap *get_spaceship_sprite(const struct tileset *t,
                                    enum spaceship_part part)
{
  return t->sprites.spaceship[part];
}

/**
   Return a sprite for the given citizen.  The citizen's type is given,
   as well as their index (in the range [0..city_size_get(pcity))).  The
   citizen's city can be used to determine which sprite to use (a NULL
   value indicates there is no city; i.e., the sprite is just being
   used as a picture).
 */
const QPixmap *get_citizen_sprite(const struct tileset *t,
                                  enum citizen_category type,
                                  int citizen_index,
                                  const struct city *pcity)
{
  const struct citizen_graphic *graphic;
  int gfx_index = citizen_index;

  if (pcity != NULL) {
    gfx_index += pcity->client.first_citizen_index;
  }

  if (type < CITIZEN_SPECIALIST) {
    fc_assert(type >= 0);
    graphic = &t->sprites.citizen[type];
  } else {
    fc_assert(type < (CITIZEN_SPECIALIST + SP_MAX));
    graphic = &t->sprites.specialist[type - CITIZEN_SPECIALIST];
  }

  if (graphic->count == 0) {
    return NULL;
  }

  return graphic->sprite[gfx_index % graphic->count];
}

/**
   Return the sprite for the nation.
 */
const QPixmap *get_nation_flag_sprite(const struct tileset *t,
                                      const struct nation_type *pnation)
{
  return t->sprites.nation_flag.p[nation_index(pnation)];
}

/**
   Return the shield sprite for the nation.
 */
const QPixmap *get_nation_shield_sprite(const struct tileset *t,
                                        const struct nation_type *pnation)
{
  return t->sprites.nation_shield.p[nation_index(pnation)];
}

/**
   Return the sprite for the technology/advance.
 */
const QPixmap *get_tech_sprite(const struct tileset *t, Tech_type_id tech)
{
  fc_assert_ret_val(0 <= tech && tech < advance_count(), NULL);
  return t->sprites.tech[tech];
}

/**
   Return the sprite for the building/improvement.
 */
const QPixmap *get_building_sprite(const struct tileset *t,
                                   const struct impr_type *pimprove)
{
  fc_assert_ret_val(NULL != pimprove, NULL);
  return t->sprites.building[improvement_index(pimprove)];
}

/**
   Return the sprite for the government.
 */
const QPixmap *get_government_sprite(const struct tileset *t,
                                     const struct government *gov)
{
  fc_assert_ret_val(NULL != gov, NULL);
  return t->sprites.government[government_index(gov)];
}

/**
   Return the sprite for the unit type (the base "unit" sprite).
   If 'facing' is direction8_invalid(), will use an unoriented sprite or
   a default orientation.
 */
const QPixmap *get_unittype_sprite(const struct tileset *t,
                                   const struct unit_type *punittype,
                                   enum direction8 facing)
{
  int uidx = utype_index(punittype);
  bool icon = !direction8_is_valid(facing);

  fc_assert_ret_val(NULL != punittype, NULL);

  if (!direction8_is_valid(facing) || !is_valid_dir(facing)) {
    facing = t->unit_default_orientation;
    /* May not have been specified, but it only matters if we don't
     * turn out to have an icon sprite */
  }

  if (t->sprites.units.icon[uidx]
      && (icon || t->sprites.units.facing[uidx][facing] == NULL)) {
    // Has icon sprite, and we prefer to (or must) use it
    return t->sprites.units.icon[uidx];
  } else {
    /* We should have a valid orientation by now. Failure to have either
     * an icon sprite or default orientation should have been caught at
     * tileset load. */
    fc_assert_ret_val(direction8_is_valid(facing), NULL);
    return t->sprites.units.facing[uidx][facing];
  }
}

/**
   Return a "sample" sprite for this city style.
 */
const QPixmap *get_sample_city_sprite(const struct tileset *t, int style_idx)
{
  int num_thresholds =
      t->sprites.city.tile->styles[style_idx].land_num_thresholds;

  if (num_thresholds == 0) {
    return NULL;
  } else {
    return (t->sprites.city.tile->styles[style_idx]
                .land_thresholds[num_thresholds - 1]
                .sprite);
  }
}

/**
   Return a tax sprite for the given output type (usually gold/lux/sci).
 */
const QPixmap *get_tax_sprite(const struct tileset *t, Output_type_id otype)
{
  switch (otype) {
  case O_SCIENCE:
    return t->sprites.tax_science;
  case O_GOLD:
    return t->sprites.tax_gold;
  case O_LUXURY:
    return t->sprites.tax_luxury;
  case O_TRADE:
  case O_FOOD:
  case O_SHIELD:
  case O_LAST:
    break;
  }
  return NULL;
}

/**
   Return event icon sprite
 */
const QPixmap *get_event_sprite(const struct tileset *t,
                                enum event_type event)
{
  return t->sprites.events[event];
}

/**
 * Return dither sprite
 */
const QPixmap *get_dither_sprite(const struct tileset *t)
{
  return t->sprites.dither_tile;
}

/**
 * Return tile mask sprite
 */
const QPixmap *get_mask_sprite(const struct tileset *t)
{
  return t->sprites.mask.tile;
}

/**
   Return a thumbs-up/thumbs-down sprite to show treaty approval or
   disapproval.
 */
const QPixmap *get_treaty_thumb_sprite(const struct tileset *t, bool on_off)
{
  return t->sprites.treaty_thumb[on_off ? 1 : 0];
}

/**
   Return a sprite_vector containing the animation sprites for a unit
   explosion.
 */
const struct sprite_vector *
get_unit_explode_animation(const struct tileset *t)
{
  return &t->sprites.explode.unit;
}

/**
   Return a sprite contining the single nuke graphic.

   TODO: This should be an animation like the unit explode animation.
 */
const QPixmap *get_nuke_explode_sprite(const struct tileset *t)
{
  return t->sprites.explode.nuke;
}

/**
   Return all the sprites used for city bar drawing.
 */
const struct citybar_sprites *get_citybar_sprites(const struct tileset *t)
{
  return &t->sprites.citybar;
}

/**
   Return all the sprites used for editor icons, images, etc.
 */
const struct editor_sprites *get_editor_sprites(const struct tileset *t)
{
  return &t->sprites.editor;
}

/**
   Returns a sprite for the given cursor.  The "hot" coordinates (the
   active coordinates of the mouse relative to the sprite) are placed int
   (*hot_x, *hot_y).
   A cursor can consist of several frames to be used for animation.
 */
const QPixmap *get_cursor_sprite(const struct tileset *t,
                                 enum cursor_type cursor, int *hot_x,
                                 int *hot_y, int frame)
{
  *hot_x = t->sprites.cursor[cursor].hot_x;
  *hot_y = t->sprites.cursor[cursor].hot_y;
  return t->sprites.cursor[cursor].frame[frame];
}

/**
   Return a sprite for the given icon.  Icons are used by the operating
   system/window manager.  Usually freeciv has to tell the OS what icon to
   use.

   Note that this function will return NULL before the sprites are loaded.
   The GUI code must be sure to call tileset_load_tiles before setting the
   top-level icon.
 */
const QPixmap *get_icon_sprite(const struct tileset *t, enum icon_type icon)
{
  return t->sprites.icon[icon];
}

/**
   Returns a sprite with the "user-attention" crosshair graphic.

   FIXME: This function shouldn't be needed if the attention graphics are
   drawn natively by the tileset code.
 */
const QPixmap *get_attention_crosshair_sprite(const struct tileset *t)
{
  return t->sprites.user.attention;
}

/**
   Returns a sprite for the given indicator with the given index.  The
   index should be in [0, NUM_TILES_PROGRESS).
 */
const QPixmap *get_indicator_sprite(const struct tileset *t,
                                    enum indicator_type indicator, int idx)
{
  idx = CLIP(0, idx, NUM_TILES_PROGRESS - 1);

  fc_assert_ret_val(indicator >= 0 && indicator < INDICATOR_COUNT, NULL);

  return t->sprites.indicator[indicator][idx];
}

/**
   Return a sprite for the unhappiness of the unit - to be shown as an
   overlay on the unit in the city support dialog, for instance.

   May return NULL if there's no unhappiness.
 */
const QPixmap *get_unit_unhappy_sprite(const struct tileset *t,
                                       const struct unit *punit,
                                       int happy_cost)
{
  Q_UNUSED(punit)
  const int unhappy =
      CLIP(0, happy_cost, t->sprites.upkeep.unhappy.size() - 1);

  if (unhappy > 0) {
    return t->sprites.upkeep.unhappy[unhappy - 1];
  } else {
    return NULL;
  }
}

/**
   Return a sprite for the upkeep of the unit - to be shown as an overlay
   on the unit in the city support dialog, for instance.

   May return NULL if there's no upkeep of the kind.
 */
const QPixmap *get_unit_upkeep_sprite(const struct tileset *t,
                                      Output_type_id otype,
                                      const struct unit *punit,
                                      const int *upkeep_cost)
{
  Q_UNUSED(punit)
  const int upkeep =
      CLIP(0, upkeep_cost[otype], t->sprites.upkeep.output[otype].size());

  if (upkeep > 0) {
    return t->sprites.upkeep.output[otype][upkeep - 1];
  } else {
    return NULL;
  }
}

/**
   Return a rectangular sprite containing a fog "color".  This can be used
   for drawing fog onto arbitrary areas (like the overview).
 */
const QPixmap *get_basic_fog_sprite(const struct tileset *t)
{
  return t->sprites.tx.fog;
}

/**
   Return the tileset's color system.
 */
struct color_system *get_color_system(const struct tileset *t)
{
  return t->color_system;
}

/**
   Initialize tileset structure
 */
void tileset_init(struct tileset *t)
{
  int wi;

  // We currently have no city sprites loaded.
  t->sprites.city.tile = NULL;

  for (wi = 0; wi < NUM_WALL_TYPES; wi++) {
    t->sprites.city.wall[wi] = NULL;
  }
  t->sprites.city.single_wall = NULL;

  t->sprites.city.occupied = NULL;

  player_slots_iterate(pslot)
  {
    int edge, j, id = player_slot_index(pslot);

    for (edge = 0; edge < EDGE_COUNT; edge++) {
      for (j = 0; j < 2; j++) {
        t->sprites.player[id].grid_borders[edge][j] = NULL;
      }
    }

    t->sprites.player[id].color = NULL;
  }
  player_slots_iterate_end;

  t->max_upkeep_height = 0;
}

/**
   Fill the sprite array with sprites that together make a representative
   image of the given terrain type. Suitable for use as an icon and in list
   views.

   NB: The 'layer' argument is NOT a LAYER_* value, but rather one of 0,
 1, 2. Using other values for 'layer' here will result in undefined
 behaviour. ;)
 */
std::vector<drawn_sprite>
fill_basic_terrain_layer_sprite_array(struct tileset *t, int layer,
                                      struct terrain *pterrain)
{
  // We create a virtual tile with only the requested terrain, then collect
  // sprites from every layer.
  auto tile = tile_virtual_new(nullptr);
  BV_CLR_ALL(tile->extras);
  tile->terrain = pterrain;

  auto sprs = std::vector<drawn_sprite>();
  for (const auto &layer : t->layers) {
    const auto lsprs = layer->fill_sprite_array(tile, nullptr, nullptr,
                                                nullptr, nullptr, nullptr);
    // Merge by hand because drawn_sprite isn't copyable (but it is
    // copy-constructible)
    for (const auto &sprite : lsprs) {
      sprs.emplace_back(sprite);
    }
  }

  tile_virtual_destroy(tile);
  return sprs;
}

/**
   Return a representative sprite for the given extra type.
 */
std::vector<drawn_sprite>
fill_basic_extra_sprite_array(const struct tileset *t,
                              const struct extra_type *pextra)
{
  // We create a virtual tile with only the requested extra, then collect
  // sprites from every layer.
  auto tile = tile_virtual_new(nullptr);
  BV_CLR_ALL(tile->extras);
  BV_SET(tile->extras, pextra->id);

  auto sprs = std::vector<drawn_sprite>();
  for (const auto &layer : t->layers) {
    const auto lsprs = layer->fill_sprite_array(tile, nullptr, nullptr,
                                                nullptr, nullptr, nullptr);
    // Merge by hand because drawn_sprite isn't copyable (but it is
    // copy-constructible)
    for (const auto &sprite : lsprs) {
      sprs.emplace_back(sprite);
    }
  }

  tile_virtual_destroy(tile);
  return sprs;
}

const std::vector<std::unique_ptr<freeciv::layer>> &
tileset_get_layers(const struct tileset *t)
{
  return t->layers;
}

/**
   Checks that a layer is within a category.
 */
bool tileset_layer_in_category(enum mapview_layer layer,
                               enum layer_category cat)
{
  switch (layer) {
  case LAYER_BACKGROUND:
  case LAYER_TERRAIN1:
  case LAYER_DARKNESS:
  case LAYER_TERRAIN2:
  case LAYER_TERRAIN3:
  case LAYER_WATER:
  case LAYER_ROADS:
  case LAYER_SPECIAL1:
  case LAYER_SPECIAL2:
  case LAYER_SPECIAL3:
  case LAYER_BASE_FLAGS:
    return cat == LAYER_CATEGORY_CITY || cat == LAYER_CATEGORY_TILE;
  case LAYER_CITY1:
  case LAYER_CITY2:
    return cat == LAYER_CATEGORY_CITY;
  case LAYER_UNIT:
  case LAYER_FOCUS_UNIT:
    return cat == LAYER_CATEGORY_UNIT;
  case LAYER_GRID1:
  case LAYER_FOG:
  case LAYER_GRID2:
  case LAYER_OVERLAYS:
  case LAYER_TILELABEL:
  case LAYER_CITYBAR:
  case LAYER_GOTO:
  case LAYER_WORKERTASK:
  case LAYER_EDITOR:
  case LAYER_INFRAWORK:
    return false;
  case LAYER_COUNT:
    break; // and fail below
  }

  fc_assert_msg(false, "Unknown layer category: %d", cat);
  return false;
}

/**
   Setup tiles for one player using the player color.
 */
void tileset_player_init(struct tileset *t, struct player *pplayer)
{
  int plrid, i, j;
  QPixmap *color;

  fc_assert_ret(pplayer != NULL);

  plrid = player_index(pplayer);
  fc_assert_ret(plrid >= 0);
  fc_assert_ret(plrid < ARRAY_SIZE(t->sprites.player));

  // Free all data before recreating it.
  tileset_player_free(t, plrid);

  for (auto &&layer : t->layers) {
    layer->initialize_player(pplayer);
  }

  QColor c = Qt::black;
  if (player_has_color(t, pplayer)) {
    c = *get_player_color(t, pplayer);
  }
  color = tileset_scale(t) == 1.0f
              ? create_sprite(128, 64, &c)
              : create_sprite(tileset_full_tile_width(t),
                              tileset_full_tile_height(t), &c);

  for (i = 0; i < EDGE_COUNT; i++) {
    for (j = 0; j < 2; j++) {
      QPixmap *s;

      if (color && t->sprites.grid.borders[i][j]) {
        s = crop_sprite(color, 0, 0, t->normal_tile_width,
                        t->normal_tile_height, t->sprites.grid.borders[i][j],
                        0, 0, 1.0f, false);
      } else {
        s = t->sprites.grid.borders[i][j];
      }
      t->sprites.player[plrid].grid_borders[i][j] = s;
    }
  }
}

/**
   Free tiles for one player using the player color.
 */
static void tileset_player_free(struct tileset *t, int plrid)
{
  int i, j;

  fc_assert_ret(plrid >= 0);
  fc_assert_ret(plrid < ARRAY_SIZE(t->sprites.player));

  for (auto &&layer : t->layers) {
    layer->free_player(plrid);
  }

  if (t->sprites.player[plrid].color) {
    free_sprite(t->sprites.player[plrid].color);
    t->sprites.player[plrid].color = NULL;
  }

  for (i = 0; i < EDGE_COUNT; i++) {
    for (j = 0; j < 2; j++) {
      if (t->sprites.player[plrid].grid_borders[i][j]) {
        free_sprite(t->sprites.player[plrid].grid_borders[i][j]);
        t->sprites.player[plrid].grid_borders[i][j] = NULL;
      }
    }
  }
}

/**
   Reset tileset data specific to ruleset.
 */
void tileset_ruleset_reset(struct tileset *t)
{
  int i;

  for (i = 0; i < ESTYLE_COUNT; i++) {
    if (t->style_lists[i] != NULL) {
      extra_type_list_destroy(t->style_lists[i]);
      t->style_lists[i] = extra_type_list_new();
    }
  }
}

/**
   Is tileset in sane state?
 */
bool tileset_is_fully_loaded() { return !tileset_update; }

/**
 * Get tileset log (warnings, errors, etc.)
 */
std::vector<tileset_log_entry> tileset_log(const struct tileset *t)
{
  return t->log;
}

/**
 * Checks if the tileset had any error message (LOG_ERROR).
 */
bool tileset_has_error(const struct tileset *t)
{
  return std::any_of(t->log.begin(), t->log.end(),
                     [](auto &entry) { return entry.level == LOG_ERROR; });
}

/**
   Return tileset name
 */
const char *tileset_name_get(struct tileset *t) { return t->given_name; }

/**
   Return tileset version
 */
const char *tileset_version(struct tileset *t) { return t->version; }

/**
   Return tileset description summary
 */
const char *tileset_summary(struct tileset *t) { return t->summary; }

/**
   Return tileset description body
 */
const char *tileset_description(struct tileset *t) { return t->description; }

/**
   Return what ruleset this tileset is primarily meant for
 */
char *tileset_what_ruleset(struct tileset *t) { return t->for_ruleset; }

/**
   Return tileset topology index
 */
int tileset_topo_index(struct tileset *t) { return t->ts_topo_idx; }
