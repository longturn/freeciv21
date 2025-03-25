/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2023 Freeciv21 and Freeciv
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

/**
  Functions for handling the tilespec files which describe
  the files and contents of tilesets.
  original author: David Pfitzner <dwp@mso.anu.edu.au>
 */

#include <QApplication>
#include <QHash>
#include <QImageReader>
#include <QPixmap>
#include <QSet>
#include <QString>
#include <QVector>
#include <cstdarg>
#include <cstdlib> // exit
#include <cstring>

#include "astring.h"
#include "bitvector.h"
#include "capability.h"
#include "city.h"
#include "fcintl.h"
#include "log.h"
#include "registry.h"
#include "registry_ini.h"
#include "shared.h"
#include "support.h"

// common
#include "game.h" // game.control.styles_count
#include "government.h"
#include "helpdata.h"
#include "nation.h"
#include "specialist.h"

/* client/include */
#include "mapview_g.h" // for update_map_canvas_visible
#include "menu_g.h"
#include "sprite_g.h"
// client
#include "citybar.h"
#include "citydlg_common.h" // for generate_citydlg_dimensions()
#include "client_main.h"
#include "climap.h" // for client_tile_get_known()
#include "climisc.h"
#include "colors_common.h"
#include "control.h" // for fill_xxx
#include "helpdlg.h"
#include "layer_background.h"
#include "layer_base_flags.h"
#include "layer_city.h"
#include "layer_city_size.h"
#include "layer_darkness.h"
#include "layer_editor.h"
#include "layer_fog.h"
#include "layer_goto.h"
#include "layer_grid.h"
#include "layer_infrawork.h"
#include "layer_overlays.h"
#include "layer_roads.h"
#include "layer_special.h"
#include "layer_terrain.h"
#include "layer_units.h"
#include "layer_water.h"
#include "layer_workertask.h"
#include "options.h" // for fill_xxx, tileset options
#include "page_game.h"
#include "tilespec.h"
#include "utils/colorizer.h"
#include "views/view_map.h"

Q_LOGGING_CATEGORY(tileset_category, "freeciv.tileset");

#define TILESPEC_CAPSTR                                                     \
  "+Freeciv-tilespec-Devel-2019-Jul-03 duplicates_ok precise-hp-bars "      \
  "unlimited-unit-select-frames unlimited-upkeep-sprites hex_corner "       \
  "terrain-specific-extras options"
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
 * hex_corner
 *    - The sprite type "hex_corner" is supported
 * options
 *    - Signals that tileset options are supported
 */

#define SPEC_CAPSTR "+Freeciv-spec-Devel-2019-Jul-03 options"
/*
 * Individual spec file capabilities acceptable to this program:
 *
 * +Freeciv-3.1-spec
 *    - basic format for Freeciv versions 3.1.x; required
 * options
 *    - Signals that tileset options are supported
 */

#define TILESPEC_SUFFIX ".tilespec"
#define TILE_SECTION_PREFIX "tile_"

/// The prefix for option sections in the tilespec file.
const static char *const OPTION_SECTION_PREFIX = "option_";

#define MAX_NUM_LAYERS 3

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

      *events[E_COUNT],

      // The panel sprites for showing tax % allocations.
      *tax_luxury, *tax_science, *tax_gold,
      *dither_tile; // only used for isometric view

  struct {
    QPixmap *tile;
  } mask;

  const QPixmap *tech[A_LAST];
  const QPixmap *building[B_LAST];
  const QPixmap *government[G_LAST];

  struct {
    std::unique_ptr<freeciv::colorizer> icon[U_LAST];
    std::unique_ptr<freeciv::colorizer> facing[U_LAST][DIR8_MAGIC_MAX];
  } units;

  std::vector<QPixmap *> nation_flag;
  std::vector<QPixmap *> nation_shield;

  struct citizen_graphic citizen[CITIZEN_LAST], specialist[SP_MAX];
  QPixmap *spaceship[SPACESHIP_COUNT];
  struct {
    int hot_x, hot_y;
    QPixmap *frame[NUM_CURSOR_FRAMES];
  } cursor[CURSOR_LAST];
  struct {
    std::vector<QPixmap *> unit;
    QPixmap *nuke;
  } explode;
  struct {
    std::vector<QPixmap *> unhappy, output[O_LAST];
  } upkeep;
  struct citybar_sprites citybar;
  struct editor_sprites editor;
  struct {
    QPixmap *attention;
  } user;
  struct {
    QPixmap *activity, *rmact;
  } extras[MAX_EXTRA_TYPES];
  struct {
    QPixmap *borders[EDGE_COUNT][2];
  } grid;
};

struct specfile {
  QPixmap *big_sprite;
  char *file_name;
};

/**
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
  QString file_name;
  int priority;

  char *summary;
  char *description;

  std::vector<tileset_log_entry> log;

  std::map<QString, tileset_option> options;

  std::vector<std::unique_ptr<freeciv::layer>> layers;
  std::array<freeciv::layer_terrain *, MAX_NUM_LAYERS> terrain_layers;
  freeciv::layer_units *focus_units_layer;

  enum ts_type type;
  int hex_width, hex_height;
  int ts_topo_idx;

  QSize normal_tile_size, full_tile_size, unit_tile_size, small_sprite_size;
  double preferred_scale;

  int max_upkeep_height;

  enum direction8 unit_default_orientation;

  freeciv::darkness_style darkness_style;
  freeciv::fog_style fogstyle;

  QPoint unit_flag_offset, city_flag_offset, unit_offset, city_offset,
      city_size_offset;

  int citybar_offset_y;
  int tilelabel_offset_y;
  QPoint activity_offset, select_offset, occupied_offset;
  int select_step_ms = 100;
  int unit_upkeep_offset_y;
  int unit_upkeep_small_offset_y;

  int num_valid_tileset_dirs, num_cardinal_tileset_dirs;
  int num_index_valid, num_index_cardinal;
  std::array<direction8, 8> valid_tileset_dirs, cardinal_tileset_dirs;
  QSet<specfile *> specfiles;
  QSet<small_sprite *> small_sprites;
  // This hash table maps tilespec tags to struct small_sprites.
  QHash<QString, small_sprite *> sprite_hash;
  QHash<QString, extrastyle_id> estyle_hash;
  struct named_sprites sprites;
  int replaced_hue; // -1 means no replacement
  struct color_system *color_system;
};

struct tileset *tileset;

static bool tileset_update = false;

static struct tileset *tileset_read_toplevel(const QString &tileset_name,
                                             bool verbose, int topology_id);

static bool tileset_setup_options(struct tileset *t,
                                  const section_file *file);

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

  switch (level) {
  case QtFatalMsg:
    qFatal("%s", qUtf8Printable(buf));
    break;
  case QtCriticalMsg:
    qCCritical(tileset_category).noquote() << buf;
    break;
  case QtWarningMsg:
    qCWarning(tileset_category).noquote() << buf;
    break;
  case QtInfoMsg:
    qCInfo(tileset_category).noquote() << buf;
    break;
  case QtDebugMsg:
    qCDebug(tileset_category).noquote() << buf;
    break;
  }
}

/**
   Returns the tileset
 */
struct tileset *get_tileset() { return tileset; }

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
  return t->normal_tile_size.width();
}

/**
   Return the tile height of the current tileset.  This is the tesselation
   height of the tiled plane.  This means it's the height of the bounding box
   of the basic map tile.

   See also tileset_tile_width.
 */
int tileset_tile_height(const struct tileset *t)
{
  return t->normal_tile_size.height();
}

/**
   Return the full tile width of the current tileset.  This is the maximum
   width that any mapview sprite will have.

   Note: currently this is always equal to the tile width.
 */
int tileset_full_tile_width(const struct tileset *t)
{
  return t->full_tile_size.width();
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
  return t->full_tile_size.height();
}

/**
 * Return the x and y offsets of full tiles in the tileset. Use this to draw
 * "full sprites".
 */
QPoint tileset_full_tile_offset(const struct tileset *t)
{
  return QPoint((t->normal_tile_size.width() - t->full_tile_size.width())
                    / 2,
                t->normal_tile_size.height() - t->full_tile_size.height());
}

/**
   Return the unit tile width of the current tileset.
 */
int tileset_unit_width(const struct tileset *t)
{
  return t->unit_tile_size.width();
}

/**
   Return the unit tile height of the current tileset.
 */
int tileset_unit_height(const struct tileset *t)
{
  return t->unit_tile_size.height();
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
   Offset to layout extra unit sprites, such as upkeep.
 */
int tileset_unit_layout_offset_y(const struct tileset *t)
{
  return t->unit_upkeep_offset_y;
}

/**
   Return the small sprite width of the current tileset.  The small sprites
   are used for various theme graphics (e.g., citymap citizens/specialists
   as well as panel indicator icons).
 */
int tileset_small_sprite_width(const struct tileset *t)
{
  return t->small_sprite_size.width();
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
   Return the small sprite height of the current tileset.  The small sprites
   are used for various theme graphics (e.g., citymap citizens/specialists
   as well as panel indicator icons).
 */
int tileset_small_sprite_height(const struct tileset *t)
{
  return t->small_sprite_size.height();
}

/**
   Return TRUE if the client will use the code to generate the fog.
 */
bool tileset_use_hard_coded_fog(const struct tileset *t)
{
  return t->fogstyle == freeciv::FOG_AUTO;
}

/**
 * Returns the preferred scale (zoom level) of the tileset.
 */
double tileset_preferred_scale(const struct tileset *t)
{
  return t->preferred_scale;
}

/**
 * Returns the hue (color) that should be replaced with the player color in
 * player-dependent sprites.
 */
int tileset_replaced_hue(const struct tileset *t) { return t->replaced_hue; }

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
 * @brief Returns the number of valid directions in the tileset.
 */
int tileset_num_valid_dirs(const struct tileset *t)
{
  return t->num_valid_tileset_dirs;
}

/**
 * @brief Returns the valid directions for the tileset.
 *
 * Only the first @ref tileset_num_valid_dirs items should be used.
 */
std::array<direction8, 8> tileset_valid_dirs(const struct tileset *t)
{
  return t->valid_tileset_dirs;
}

/**
   Initialize.
 */
static struct tileset *tileset_new() { return new struct tileset(); }

/**
   Return the tileset name of the direction.  This is similar to
   dir_get_name but you shouldn't change this or all tilesets will break.
 */
QString dir_get_tileset_name(enum direction8 dir)
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
  qCCritical(tileset_category, "Wrong direction8 variant: %d.", dir);
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
bool is_cardinal_tileset_dir(const struct tileset *t, enum direction8 dir)
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
 * Initializes the tilespec system.
 */
void tilespec_init()
{
  const auto type = QEvent::registerEventType();
  fc_assert(type != -1);
  TilesetChanged = static_cast<QEvent::Type>(type);
}

/**
 * An event type sent to all widgets when the current tileset changes.
 */
QEvent::Type TilesetChanged;

/**
   Returns a static list of tilesets available on the system by
   searching all data directories for files matching TILESPEC_SUFFIX.
 */
const QVector<QString> *get_tileset_list(const struct option *poption)
{
  static QVector<QString> *tilesets[3] = {
      new QVector<QString>, new QVector<QString>, new QVector<QString>};
  int topo = option_get_cb_data(poption);
  int idx;

  idx = ts_topology_index(topo);

  fc_assert_ret_val(idx < ARRAY_SIZE(tilesets), nullptr);

  QVector<QString> *list = fileinfolist(get_data_dirs(), TILESPEC_SUFFIX);
  tilesets[idx]->clear();
  for (const auto &file : std::as_const(*list)) {
    struct tileset *t = tileset_read_toplevel(file, false, topo);
    if (t) {
      tilesets[idx]->append(file);
      tileset_free(t);
    }
  }
  delete list;

  return tilesets[idx];
}

/**
 * Gets full filename for tilespec file, based on input name.
 */
static QString tilespec_fullname(QString tileset_name)
{
  if (!tileset_name.isEmpty()) {
    auto fname = tileset_name + QStringLiteral(TILESPEC_SUFFIX);
    auto dname = fileinfoname(get_data_dirs(), fname);

    if (!dname.isEmpty()) {
      return dname;
    }
  }

  return QString();
}

/**
   Checks options in filename match what we require and support.
   Die if not.
   'which' should be "tilespec" or "spec".
 */
static bool check_tilespec_capabilities(struct section_file *file,
                                        const char *which,
                                        const char *us_capstr,
                                        const QString &filename,
                                        bool verbose)
{
  QtMsgType level = verbose ? LOG_ERROR : LOG_DEBUG;

  const char *file_capstr = secfile_lookup_str(file, "%s.options", which);

  if (nullptr == file_capstr) {
    log_base(level, "\"%s\": %s file doesn't have a capability string",
             qUtf8Printable(filename), which);
    return false;
  }
  if (!has_capabilities(us_capstr, file_capstr)) {
    log_base(level, "\"%s\": %s file appears incompatible:",
             qUtf8Printable(filename), which);
    log_base(level, "  datafile options: %s", file_capstr);
    log_base(level, "  supported options: %s", us_capstr);
    return false;
  }
  if (!has_capabilities(file_capstr, us_capstr)) {
    log_base(level,
             "\"%s\": %s file requires option(s) "
             "that client doesn't support:",
             qUtf8Printable(filename), which);
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
  t->estyle_hash.clear();

  if (t->color_system) {
    color_system_free(t->color_system);
    t->color_system = nullptr;
  }

  delete[] t->summary;
  delete[] t->description;
  t->summary = nullptr;
  t->description = nullptr;
}

/**
   Clean up.
 */
void tileset_free(struct tileset *t)
{
  tileset_free_tiles(t);
  tileset_free_toplevel(t);
  for (int i = 0; i < MAX_NUM_PLAYER_SLOTS; i++) {
    tileset_player_free(t, i);
  }
  delete t;
}

/**
   Read a new tilespec in when first starting the game.

   Call this function with the (guessed) name of the tileset, when
   starting the client.

   Returns TRUE iff tileset with suggested tileset_name was loaded.
 */
bool tilespec_try_read(const QString &tileset_name, bool verbose,
                       int topo_id)
{
  bool original;

  if (tileset_name.isEmpty()
      || !(tileset =
               tileset_read_toplevel(tileset_name, verbose, topo_id))) {
    QVector<QString> *list = fileinfolist(get_data_dirs(), TILESPEC_SUFFIX);

    original = false;
    for (const auto &file : std::as_const(*list)) {
      struct tileset *t =
          tileset_read_toplevel(qUtf8Printable(file), false, topo_id);

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

    qCDebug(tileset_category, "Trying tileset \"%s\".", tileset->name);
  } else {
    original = true;
  }

  return original;
}

/**
   Read a new tilespec in from scratch.

   Unlike the initial reading code, which reads pieces one at a time,
   this gets rid of the old data and reads in the new all at once.  If the
   new tileset fails to load the old tileset may be reloaded; otherwise the
   client will exit.

   It will also call the necessary functions to redraw the graphics.

   Returns TRUE iff new tileset has been succesfully loaded.
 */
bool tilespec_reread(const QString &name, bool game_fully_initialized)
{
  int id;
  enum client_states state = client_state();
  bool new_tileset_in_use;

  qCInfo(tileset_category, _("Loading tileset \"%s\"."),
         qUtf8Printable(name));

  /* Step 0:  Record old data.
   *
   * We record the current mapcanvas center, etc.
   */
  auto center_tile = tileset ? get_center_tile_mapcanvas() : nullptr;

  /* Step 1:  Cleanup.
   *
   * Free old tileset.
   */
  const char *old_name = nullptr;
  if (tileset) {
    old_name = tileset->name;
    tileset_free(tileset);
  }

  /* Step 2:  Read.
   *
   * We read in the new tileset.  This should be pretty straightforward.
   */
  tileset = tileset_read_toplevel(name, false, -1);
  if (tileset != nullptr) {
    new_tileset_in_use = true;
  } else {
    new_tileset_in_use = false;

    if (old_name
        && !(tileset = tileset_read_toplevel(old_name, false, -1))) {
      // Always fails.
      fc_assert_exit_msg(nullptr != tileset,
                         "Failed to re-read the currently loaded tileset.");
    }
  }
  tileset_load_tiles(tileset);

  if (game_fully_initialized) {
    players_iterate(pplayer) { tileset_player_init(tileset, pplayer); }
    players_iterate_end;

    // "About Current Tileset"
    popdown_help_dialog();
    boot_help_texts(client_current_nation_set(), tileset_help(tileset));
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
  for (auto &gov : governments) {
    tileset_setup_government(tileset, &gov);
  }
  extra_type_iterate(pextra) { tileset_setup_extra(tileset, pextra); }
  extra_type_iterate_end;
  for (auto &pnation : nations) {
    tileset_setup_nation_flag(tileset, &pnation);
  } // iterate over nations - pnation
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

  if (state < C_S_RUNNING) {
    // Below redraws do not apply before this.
    return new_tileset_in_use;
  }

  /* Step 4:  Draw.
   *
   * Do any necessary redraws.
   */
  // Old style notifications
  generate_citydlg_dimensions();
  tileset_changed();
  /* update_map_canvas_visible forces a full redraw.  Otherwise with fast
   * drawing we might not get one.  Of course this is slower. */
  update_map_canvas_visible();
  queen()->mapview_wdg->center_on_tile(center_tile, false);

  // New style notifications. See QApplication::setStyle for inspiration
  auto widgets = QApplication::allWidgets();
  for (auto *w : std::as_const(widgets)) {
    if (w->windowType() != Qt::Desktop) {
      QEvent e(TilesetChanged);
      QApplication::sendEvent(w, &e);
    }
  }

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

  fc_assert_ret(nullptr != tileset_name && tileset_name[0] != '\0');
  tileset_update = true;
  tilespec_reread(tileset_name, client.conn.established);
  tileset_update = false;
  menus_init();
}

/**
   Read a new tilespec in from scratch. Keep UI frozen while tileset is
   partially loaded; in inconsistent state.

   See tilespec_reread() for details.
 */
void tilespec_reread_frozen_refresh(const QString &name)
{
  tileset_update = true;
  tilespec_reread(name, true);
  tileset_update = false;
  menus_init();
}

/**
 * Makes a dummy "error" pixmap to prevent crashes.
 */
static QPixmap *make_error_pixmap()
{
  auto s = new QPixmap(20, 20);
  s->fill(Qt::red);
  return s;
}

/**
   Loads the given graphics file (found in the data path) into a newly
   allocated sprite.
 */
static QPixmap *load_gfx_file(const char *gfx_filename)
{
  // Try out all supported file extensions to find one that works.
  auto supported = QImageReader::supportedImageFormats();

  // Make sure we try png first (it's the most common and Qt always supports
  // it). This dramatically improves tileset loading performance on Windows.
  supported.prepend("png");

  for (auto gfx_fileext : std::as_const(supported)) {
    QString real_full_name;
    QString full_name =
        QStringLiteral("%1.%2").arg(gfx_filename, gfx_fileext.data());

    real_full_name =
        fileinfoname(get_data_dirs(), qUtf8Printable(full_name));
    if (!real_full_name.isEmpty()) {
      log_debug("trying to load gfx file \"%s\".",
                qUtf8Printable(real_full_name));
      if (const auto s = load_gfxfile(qUtf8Printable(real_full_name)); s) {
        return s;
      }
    }
  }

  qCCritical(tileset_category, "Could not load gfx file \"%s\".",
             gfx_filename);
  return make_error_pixmap();
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
        qCCritical(tileset_category, "Grid \"%s\" invalid: %s", sec_name,
                   secfile_error());
        continue;
      }

      j = -1;
      while (
          nullptr
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
          qCCritical(tileset_category,
                     "Small sprite \"%s.tiles%d\" invalid: %s", sec_name, j,
                     secfile_error());
          continue;
        }

        // Cursor pointing coordinates
        hot_x = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_x",
                                           sec_name, j);
        hot_y = secfile_lookup_int_default(file, 0, "%s.tiles%d.hot_y",
                                           sec_name, j);

        // User-configured options
        auto option = QString::fromUtf8(secfile_lookup_str_default(
            file, "", "%s.tiles%d.option", sec_name, j));
        if (!option.isEmpty()) {
          if (!tileset_has_option(t, option)) {
            // Ignore unknown options
            tileset_error(
                t, QtWarningMsg, "%s: unknown option %s for sprite %s",
                tileset_basename(t), qUtf8Printable(option), tags[0]);
          } else if (!tileset_option_is_enabled(t, option)) {
            // Skip sprites that correspond to disabled options
            continue;
          }
        }

        // there must be at least 1 because of the while():
        fc_assert_action(num_tags > 0, continue);

        xr = x_top_left + (dx + pixel_border_x) * column;
        yb = y_top_left + (dy + pixel_border_y) * row;

        ss = new small_sprite;
        ss->ref_count = 0;
        ss->file = nullptr;
        ss->x = xr;
        ss->y = yb;
        ss->width = dx;
        ss->height = dy;
        ss->sf = sf;
        ss->sprite = nullptr;
        ss->hot_x = hot_x;
        ss->hot_y = hot_y;

        t->small_sprites.insert(ss);

        if (!duplicates_ok) {
          for (k = 0; k < num_tags; k++) {
            if (t->sprite_hash.contains(tags[k]) && !option.isEmpty()) {
              // Warn about duplicated sprites, except if it was enabled by
              // a user option (to override the default).
              qCCritical(tileset_category,
                         "warning: %s: already have a sprite for \"%s\".",
                         t->name, tags[k]);
            }
            t->sprite_hash.insert(tags[k], ss);
          }
        } else {
          for (k = 0; k < num_tags; k++) {
            t->sprite_hash.insert(tags[k], ss);
          }
        }

        delete[] tags;
        tags = nullptr;
      }
    }
    section_list_iterate_end;
    section_list_destroy(sections);
  }

  // Load "extra" sprites.  Each sprite is one file.
  i = -1;
  while (nullptr != secfile_entry_lookup(file, "extra.sprites%d.tag", ++i)) {
    struct small_sprite *ss;
    const char **tags;
    const char *filename;
    size_t num_tags, k;
    int hot_x, hot_y;

    if (!(tags = secfile_lookup_str_vec(file, &num_tags,
                                        "extra.sprites%d.tag", i))
        || !(filename =
                 secfile_lookup_str(file, "extra.sprites%d.file", i))) {
      qCCritical(tileset_category,
                 "Extra sprite \"extra.sprites%d\" invalid: %s", i,
                 secfile_error());
      continue;
    }

    // Cursor pointing coordinates
    hot_x = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_x", i);
    hot_y = secfile_lookup_int_default(file, 0, "extra.sprites%d.hot_y", i);

    // User-configured options
    auto option = QString::fromUtf8(
        secfile_lookup_str_default(file, "", "extras.sprites%d.option", i));
    if (!option.isEmpty()) {
      if (!tileset_has_option(t, option)) {
        // Ignore unknown options
        tileset_error(t, QtWarningMsg, "%s: unknown option %s for sprite %s",
                      tileset_basename(t), qUtf8Printable(option), tags[0]);
      } else if (!tileset_option_is_enabled(t, option)) {
        // Skip sprites that correspond to disabled options
        continue;
      }
    }

    ss = new small_sprite;
    ss->ref_count = 0;
    ss->file = fc_strdup(filename);
    ss->sf = nullptr;
    ss->sprite = nullptr;
    ss->hot_x = hot_x;
    ss->hot_y = hot_y;

    t->small_sprites.insert(ss);

    if (!duplicates_ok) {
      for (k = 0; k < num_tags; k++) {
        if (t->sprite_hash.contains(tags[k])) {
          qCWarning(tileset_category,
                    "%s: already have a sprite for \"%s\".", t->name,
                    tags[k]);
        }
        t->sprite_hash.insert(tags[k], ss);
      }
    } else {
      for (k = 0; k < num_tags; k++) {
        t->sprite_hash.insert(tags[k], ss);
      }
    }
    delete[] tags;
  }

  secfile_check_unused(file);
  secfile_destroy(file);
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
  if (fc_strcasecmp(sprite_type, "hex_corner") == 0) {
    return freeciv::layer_terrain::CELL_HEX_CORNER;
  }
  if (fc_strcasecmp(sprite_type, "single") == 0) {
    return freeciv::layer_terrain::CELL_WHOLE;
  }
  if (fc_strcasecmp(sprite_type, "whole") == 0) {
    return freeciv::layer_terrain::CELL_WHOLE;
  }
  qCCritical(tileset_category, "[%s] unknown sprite_type \"%s\".",
             tile_section, sprite_type);
  return freeciv::layer_terrain::CELL_WHOLE;
}

static bool tileset_invalid_offsets(struct tileset *t,
                                    struct section_file *file)
{
  return !secfile_lookup_int(file, &t->unit_flag_offset.rx(),
                             "tilespec.unit_flag_offset_x")
         || !secfile_lookup_int(file, &t->unit_flag_offset.ry(),
                                "tilespec.unit_flag_offset_y")
         || !secfile_lookup_int(file, &t->city_flag_offset.rx(),
                                "tilespec.city_flag_offset_x")
         || !secfile_lookup_int(file, &t->city_flag_offset.ry(),
                                "tilespec.city_flag_offset_y")
         || !secfile_lookup_int(file, &t->unit_offset.rx(),
                                "tilespec.unit_offset_x")
         || !secfile_lookup_int(file, &t->unit_offset.ry(),
                                "tilespec.unit_offset_y")
         || !secfile_lookup_int(file, &t->activity_offset.rx(),
                                "tilespec.activity_offset_x")
         || !secfile_lookup_int(file, &t->activity_offset.ry(),
                                "tilespec.activity_offset_y")
         || !secfile_lookup_int(file, &t->select_offset.rx(),
                                "tilespec.select_offset_x")
         || !secfile_lookup_int(file, &t->select_offset.ry(),
                                "tilespec.select_offset_y")
         || !secfile_lookup_int(file, &t->city_offset.rx(),
                                "tilespec.city_offset_x")
         || !secfile_lookup_int(file, &t->city_offset.ry(),
                                "tilespec.city_offset_y")
         || !secfile_lookup_int(file, &t->city_size_offset.rx(),
                                "tilespec.city_size_offset_x")
         || !secfile_lookup_int(file, &t->city_size_offset.ry(),
                                "tilespec.city_size_offset_y")
         || !secfile_lookup_int(file, &t->citybar_offset_y,
                                "tilespec.citybar_offset_y")
         || !secfile_lookup_int(file, &t->tilelabel_offset_y,
                                "tilespec.tilelabel_offset_y")
         || !secfile_lookup_int(file, &t->occupied_offset.rx(),
                                "tilespec.occupied_offset_x")
         || !secfile_lookup_int(file, &t->occupied_offset.ry(),
                                "tilespec.occupied_offset_y");
}

static void tileset_set_offsets(struct tileset *t, struct section_file *file)
{
  t->unit_upkeep_offset_y = secfile_lookup_int_default(
      file, tileset_tile_height(t), "tilespec.unit_upkeep_offset_y");
  t->unit_upkeep_small_offset_y = secfile_lookup_int_default(
      file, t->unit_upkeep_offset_y, "tilespec.unit_upkeep_small_offset_y");
}

static void tileset_stop_read(struct tileset *t, struct section_file *file,
                              struct section_list *sections,
                              const char **layer_order)
{
  secfile_destroy(file);
  delete[] layer_order;
  delete t;
  if (nullptr != sections) {
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
  case LAYER_CITY1: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_city>(
        t, t->city_offset, t->city_flag_offset, t->occupied_offset));
  } break;
  case LAYER_CITY2: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_city_size>(
        t, tileset_full_tile_offset(t) + t->city_size_offset));
  } break;
  case LAYER_DARKNESS: {
    t->layers.emplace_back(
        std::make_unique<freeciv::layer_darkness>(t, t->darkness_style));
  } break;
  case LAYER_EDITOR: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_editor>(t));
  } break;
  case LAYER_FOG: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_fog>(
        t, t->fogstyle, t->darkness_style));
  } break;
  case LAYER_GOTO: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_goto>(t));
  } break;
  case LAYER_GRID1:
  case LAYER_GRID2: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_grid>(t, layer));
  } break;
  case LAYER_INFRAWORK: {
    t->layers.emplace_back(
        std::make_unique<freeciv::layer_infrawork>(t, t->activity_offset));
  } break;
  case LAYER_OVERLAYS: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_overlays>(t));
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
  case LAYER_SPECIAL1:
  case LAYER_SPECIAL2:
  case LAYER_SPECIAL3: {
    t->layers.emplace_back(
        std::make_unique<freeciv::layer_special>(t, layer));
  } break;
  case LAYER_ROADS: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_roads>(t));
  } break;
  case LAYER_BASE_FLAGS: {
    auto l = std::make_unique<freeciv::layer_base_flags>(
        t, tileset_full_tile_offset(t) + t->city_flag_offset);
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_UNIT: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_units>(
        t, layer, t->activity_offset, t->select_offset, t->unit_offset,
        t->unit_flag_offset));
  } break;
  case LAYER_FOCUS_UNIT: {
    auto l = std::make_unique<freeciv::layer_units>(
        t, layer, t->activity_offset, t->select_offset, t->unit_offset,
        t->unit_flag_offset);
    t->focus_units_layer = l.get();
    t->layers.emplace_back(std::move(l));
  } break;
  case LAYER_WATER: {
    t->layers.emplace_back(std::make_unique<freeciv::layer_water>(t));
  } break;
  case LAYER_WORKERTASK: {
    t->layers.emplace_back(
        std::make_unique<freeciv::layer_workertask>(t, t->activity_offset));
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
static struct tileset *tileset_read_toplevel(const QString &tileset_name,
                                             bool verbose, int topology_id)
{
  struct section_file *file;
  const char *c;
  int i;
  size_t num_spec_files;
  const char **spec_filenames;
  size_t num_layers;
  const char **layer_order = nullptr;
  struct section_list *sections = nullptr;
  const char *file_capstr;
  bool duplicates_ok, is_hex;
  enum direction8 dir;
  struct tileset *t = nullptr;
  const char *extraname;
  const char *tstr;
  int topo;

  auto fname = tilespec_fullname(tileset_name);
  if (fname.isEmpty()) {
    if (verbose) {
      qCCritical(tileset_category, "Can't find tileset \"%s\".",
                 qUtf8Printable(tileset_name));
    }
    return nullptr;
  }
  qCDebug(tileset_category, "tilespec file is \"%s\".",
          qUtf8Printable(fname));

  if (!(file = secfile_load(fname, true))) {
    qCCritical(tileset_category, "Could not open '%s':\n%s",
               qUtf8Printable(fname), secfile_error());
    return nullptr;
  }

  if (!check_tilespec_capabilities(file, "tilespec", TILESPEC_CAPSTR, fname,
                                   verbose)) {
    secfile_destroy(file);
    return nullptr;
  }

  t = tileset_new();
  t->file_name = fname;

  file_capstr = secfile_lookup_str(file, "%s.options", "tilespec");
  duplicates_ok = (nullptr != file_capstr
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
    delete[] t->summary;
    t->summary = nullptr;
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
    if (t->description != nullptr) {
      delete[] t->description;
      t->description = nullptr;
    }
  }

  sz_strlcpy(t->name, tileset_name.toUtf8().data());
  if (!secfile_lookup_int(file, &t->priority, "tilespec.priority")
      || !secfile_lookup_bool(file, &is_hex, "tilespec.is_hex")) {
    qCCritical(tileset_category, "Tileset \"%s\" invalid: %s", t->name,
               secfile_error());
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  tstr = secfile_lookup_str(file, "tilespec.type");
  if (tstr == nullptr) {
    qCCritical(tileset_category, "Tileset \"%s\": no tileset type", t->name);
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  t->type = ts_type_by_name(tstr, fc_strcasecmp);
  if (!ts_type_is_valid(t->type)) {
    qCCritical(tileset_category,
               "Tileset \"%s\": unknown tileset type \"%s\"", t->name, tstr);
    tileset_stop_read(t, file, sections, layer_order);
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
      qCCritical(tileset_category, "Tileset \"%s\" invalid: %s", t->name,
                 secfile_error());
      tileset_stop_read(t, file, sections, layer_order);
    }
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
      tileset_stop_read(t, file, sections, layer_order);
      return nullptr;
    }
  }

  t->ts_topo_idx = ts_topology_index(topo);

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

  if (!secfile_lookup_int(file, &t->normal_tile_size.rwidth(),
                          "tilespec.normal_tile_width")
      || !secfile_lookup_int(file, &t->normal_tile_size.rheight(),
                             "tilespec.normal_tile_height")) {
    qCCritical(tileset_category, "Tileset \"%s\" invalid: %s", t->name,
               secfile_error());
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }
  if (t->type == TS_ISOMETRIC) {
    t->full_tile_size.rwidth() = t->normal_tile_size.width();
    if (tileset_hex_height(t) > 0) {
      t->full_tile_size.rheight() = t->normal_tile_size.height();
    } else {
      t->full_tile_size.rheight() = 3 * t->normal_tile_size.height() / 2;
    }
  } else {
    t->full_tile_size = t->normal_tile_size;
  }
  t->unit_tile_size.rwidth() = secfile_lookup_int_default(
      file, t->full_tile_size.width(), "tilespec.unit_width");
  t->unit_tile_size.rheight() = secfile_lookup_int_default(
      file, t->full_tile_size.height(), "tilespec.unit_height");
  // Hue to be replaced in unit graphics
  t->replaced_hue =
      secfile_lookup_int_default(file, -1, "tilespec.replaced_hue");

  if (!secfile_lookup_int(file, &t->small_sprite_size.rwidth(),
                          "tilespec.small_tile_width")
      || !secfile_lookup_int(file, &t->small_sprite_size.rheight(),
                             "tilespec.small_tile_height")) {
    qCCritical(tileset_category, "Tileset \"%s\" invalid: %s", t->name,
               secfile_error());
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }
  qCDebug(tileset_category, "tile sizes %dx%d, %dx%d unit, %dx%d small",
          t->normal_tile_size.width(), t->normal_tile_size.height(),
          t->full_tile_size.width(), t->full_tile_size.height(),
          t->small_sprite_size.width(), t->small_sprite_size.height());

  tstr = secfile_lookup_str(file, "tilespec.fog_style");
  if (tstr == nullptr) {
    qCCritical(tileset_category, "Tileset \"%s\": no fog_style", t->name);
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  t->fogstyle = freeciv::fog_style_by_name(tstr, fc_strcasecmp);
  if (!fog_style_is_valid(t->fogstyle)) {
    qCCritical(tileset_category, "Tileset \"%s\": unknown fog_style \"%s\"",
               t->name, tstr);
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  t->preferred_scale = secfile_lookup_int_def_min_max(
                           file, 100, 1, 1000, "tilespec.preferred_scale")
                       / 100.;

  t->select_step_ms = secfile_lookup_int_def_min_max(
      file, 100, 1, 10000, "tilespec.select_step_ms");

  if (tileset_invalid_offsets(t, file)) {
    qCCritical(tileset_category, "Tileset \"%s\" invalid: %s", t->name,
               secfile_error());
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }
  tileset_set_offsets(t, file);

  c = secfile_lookup_str_default(file, nullptr,
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
      tileset_stop_read(t, file, sections, layer_order);
      return nullptr;
    } else {
      /* Default orientation is allowed to not be a valid one for the
       * tileset */
      t->unit_default_orientation = dir;
    }
  }

  tstr = secfile_lookup_str(file, "tilespec.darkness_style");
  if (tstr == nullptr) {
    qCCritical(tileset_category, "Tileset \"%s\": no darkness_style",
               t->name);
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  t->darkness_style = freeciv::darkness_style_by_name(tstr, fc_strcasecmp);
  if (!darkness_style_is_valid(t->darkness_style)) {
    qCCritical(tileset_category,
               "Tileset \"%s\": unknown darkness_style \"%s\"", t->name,
               tstr);
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  if (t->darkness_style == freeciv::DARKNESS_ISORECT
      && (t->type == TS_OVERHEAD || t->hex_width > 0 || t->hex_height > 0)) {
    qCCritical(tileset_category,
               "Invalid darkness style set in tileset \"%s\".", t->name);
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  // Layer order
  num_layers = 0;
  layer_order =
      secfile_lookup_str_vec(file, &num_layers, "tilespec.layer_order");
  if (layer_order != nullptr) {
    mapview_layer order[LAYER_COUNT];
    for (i = 0; i < num_layers; i++) {
      int j;
      enum mapview_layer layer =
          mapview_layer_by_name(layer_order[i], fc_strcasecmp);

      // Check for wrong layer names.
      if (!mapview_layer_is_valid(layer)) {
        qCCritical(tileset_category, "layer_order: Invalid layer \"%s\"",
                   layer_order[i]);
        tileset_stop_read(t, file, sections, layer_order);
        return nullptr;
      }
      // Check for duplicates.
      for (j = 0; j < i; j++) {
        if (order[j] == layer) {
          qCCritical(tileset_category, "layer_order: Duplicate layer \"%s\"",
                     layer_order[i]);
          tileset_stop_read(t, file, sections, layer_order);
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
        qCCritical(tileset_category, "layer_order: Missing layer \"%s\"",
                   mapview_layer_name(static_cast<mapview_layer>(i)));
        tileset_stop_read(t, file, sections, layer_order);
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

  // Terrain layer info.
  for (i = 0; i < MAX_NUM_LAYERS; i++) {
    std::size_t count = 0;
    auto match_types =
        secfile_lookup_str_vec(file, &count, "layer%d.match_types", i);
    for (int j = 0; j < count; j++) {
      if (!t->terrain_layers[i]->create_matching_group(match_types[j])) {
        tileset_stop_read(t, file, sections, layer_order);
        return nullptr;
      }
    }
  }

  // Tile drawing info.
  sections = secfile_sections_by_name_prefix(file, TILE_SECTION_PREFIX);
  if (nullptr == sections || 0 == section_list_size(sections)) {
    tileset_error(t, LOG_ERROR,
                  _("No [%s] sections supported by tileset \"%s\"."),
                  TILE_SECTION_PREFIX, qUtf8Printable(fname));
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  section_list_iterate(sections, psection)
  {
    auto sec_name = section_name(psection);

    QString tag;
    {
      auto c_tag = secfile_lookup_str(file, "%s.tag", sec_name);
      if (c_tag != nullptr) {
        tag = c_tag;
      } else {
        tileset_error(t, LOG_ERROR,
                      _("No terrain tag given in section [%s]."), sec_name);
        tileset_stop_read(t, file, sections, layer_order);
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
          tileset_stop_read(t, file, sections, layer_order);
          return nullptr;
        }

        // Offsets
        {
          auto is_tall = secfile_lookup_bool_default(
              file, false, "%s.layer%d_is_tall", sec_name, l);

          auto offset_x = secfile_lookup_int_default(
              file, 0, "%s.layer%d_offset_x", sec_name, l);
          if (is_tall) {
            offset_x += tileset_full_tile_offset(t).x();
          }

          auto offset_y = secfile_lookup_int_default(
              file, 0, "%s.layer%d_offset_y", sec_name, l);
          if (is_tall) {
            offset_y += tileset_full_tile_offset(t).y();
          }

          if (!t->terrain_layers[l]->set_tag_offsets(tag, offset_x,
                                                     offset_y)) {
            tileset_stop_read(t, file, sections, layer_order);
            return nullptr;
          }
        }

        // Sprite type
        {
          auto type_str = secfile_lookup_str_default(
              file, "whole", "%s.layer%d_sprite_type", sec_name, l);
          if (!t->terrain_layers[l]->set_tag_sprite_type(
                  tag, check_sprite_type(type_str, sec_name))) {
            tileset_stop_read(t, file, sections, layer_order);
            return nullptr;
          }
        }

        // Matching
        {
          auto matching_group = secfile_lookup_str_default(
              file, nullptr, "%s.layer%d_match_type", sec_name, l);

          if (matching_group) {
            if (!t->terrain_layers[l]->set_tag_matching_group(
                    tag, matching_group)) {
              tileset_stop_read(t, file, sections, layer_order);
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
                tileset_stop_read(t, file, sections, layer_order);
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
  sections = nullptr;

  if (!tileset_setup_options(t, file)) {
    return nullptr;
  }

  t->estyle_hash.clear();

  for (i = 0; (extraname = secfile_lookup_str_default(
                   file, nullptr, "extras.styles%d.name", i));
       i++) {
    const char *style_name;

    style_name = secfile_lookup_str_default(file, "Single1",
                                            "extras.styles%d.style", i);
    auto style = extrastyle_id_by_name(style_name, fc_strcasecmp);

    if (t->estyle_hash.contains(extraname)) {
      qCCritical(tileset_category,
                 "warning: duplicate extrastyle entry [%s].", extraname);
      tileset_stop_read(t, file, sections, layer_order);
      return nullptr;
    }
    t->estyle_hash.insert(extraname, style);
  }

  spec_filenames =
      secfile_lookup_str_vec(file, &num_spec_files, "tilespec.files");
  if (nullptr == spec_filenames || 0 == num_spec_files) {
    qCCritical(tileset_category,
               "No tile graphics files specified in \"%s\"",
               qUtf8Printable(fname));
    tileset_stop_read(t, file, sections, layer_order);
    return nullptr;
  }

  t->sprite_hash.clear();
  for (i = 0; i < num_spec_files; i++) {
    struct specfile *sf = new specfile();
    QString dname;

    log_debug("spec file %s", spec_filenames[i]);

    sf->big_sprite = nullptr;
    dname = fileinfoname(get_data_dirs(), spec_filenames[i]);
    if (dname.isEmpty()) {
      if (verbose) {
        qCCritical(tileset_category, "Can't find spec file \"%s\".",
                   spec_filenames[i]);
      }
      delete sf;
      tileset_stop_read(t, file, sections, layer_order);
      return nullptr;
    }
    sf->file_name = fc_strdup(qUtf8Printable(dname));
    scan_specfile(t, sf, duplicates_ok);

    t->specfiles.insert(sf);
  }
  delete[] spec_filenames;

  t->color_system = color_system_read(file);

  secfile_check_unused(file);
  secfile_destroy(file);
  qCDebug(tileset_category, "finished reading \"%s\".",
          qUtf8Printable(fname));
  delete[] layer_order;

  return t;
}

/**
 * Loads tileset options.
 *
 * This function loads options from the a '.tilespec' file and sets up all
 * structures in the tileset.
 */
static bool tileset_setup_options(struct tileset *t,
                                  const section_file *file)
{
  // First load options from the tilespec file.
  auto sections =
      secfile_sections_by_name_prefix(file, OPTION_SECTION_PREFIX);
  if (!sections) {
    return true;
  }

  std::set<QString> all_names;

  section_list_iterate(sections, psection)
  {
    // Mandatory fields: name, description. Optional: enabled_by_default.
    const auto sec_name = section_name(psection);

    tileset_option option;

    auto name = secfile_lookup_str_default(file, "", "%s.name", sec_name);
    if (qstrlen(name) == 0) {
      tileset_error(t, QtCriticalMsg, "Option \"%s\" has no name", sec_name);
      continue; // Skip instead of erroring out: options are optional
    }

    // Check for duplicates
    if (all_names.count(name)) {
      tileset_error(t, QtCriticalMsg, "Duplicated option name \"%s\"", name);
      continue; // Skip instead of erroring out: options are optional
    }
    all_names.insert(name);

    auto description =
        secfile_lookup_str_default(file, "", "%s.description", sec_name);
    if (qstrlen(description) == 0) {
      tileset_error(t, QtCriticalMsg, "Option \"%s\" has no description",
                    name);
      continue; // Skip instead of erroring out: options are optional
    }
    option.description = QString::fromUtf8(description);

    option.enabled_by_default =
        secfile_lookup_bool_default(file, false, "%s.default", sec_name);
    option.enabled = option.enabled_by_default;

    t->options[name] = std::move(option);
  }
  section_list_iterate_end;

  // Then apply client options. They override any default value we may have
  // set.
  const auto tileset_name = tileset_basename(t);
  if (gui_options->tileset_options.count(tileset_name)) {
    for (const auto &[name, value] :
         gui_options->tileset_options[tileset_name]) {
      if (tileset_has_option(t, name)) {
        t->options[name].enabled = value;
      }
      // Silently ignore options that do not exist.
    }
  }

  return true;
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
  qCCritical(tileset_category, "Unknown citizen type: %d.",
             static_cast<int>(citizen));
  return nullptr;
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
QString valid_index_str(const struct tileset *t, int idx)
{
  auto c = QString();
  for (int i = 0; i < t->num_valid_tileset_dirs; i++) {
    int value = (idx >> i) & 1;

    c += QStringLiteral("%1%2").arg(
        dir_get_tileset_name(t->valid_tileset_dirs[i]),
        QString::number(value));
  }

  return c;
}

/**
   Loads the sprite. If the sprite is already loaded a reference
   counter is increased. Can return nullptr if the sprite couldn't be
   loaded.
 */
static QPixmap *load_sprite(struct tileset *t, const QString &tag_name)
{
  struct small_sprite *ss;

  log_debug("load_sprite(tag='%s')", qUtf8Printable(tag_name));
  // Lookup information about where the sprite is found.
  if (!(ss = t->sprite_hash.value(tag_name, nullptr))) {
    return nullptr;
  }

  fc_assert(ss->ref_count >= 0);

  if (!ss->sprite) {
    // If the sprite hasn't been loaded already, then load it.
    fc_assert(ss->ref_count == 0);
    if (ss->file) {
      ss->sprite = load_gfx_file(ss->file);
      if (!ss->sprite) {
        tileset_error(t, LOG_ERROR,
                      _("Couldn't load gfx file \"%s\" for sprite '%s'."),
                      ss->file, qUtf8Printable(tag_name));
        return nullptr;
      }
    } else {
      ensure_big_sprite(t, ss->sf);

      auto sf_w = ss->sf->big_sprite->width();
      auto sf_h = ss->sf->big_sprite->height();
      if (ss->x < 0 || ss->x + ss->width > sf_w || ss->y < 0
          || ss->y + ss->height > sf_h) {
        tileset_error(
            t, LOG_ERROR,
            _("Sprite '%s' in file \"%s\" isn't within the image!"),
            qUtf8Printable(tag_name), ss->sf->file_name);
        return nullptr;
      }
      ss->sprite = crop_sprite(ss->sf->big_sprite, ss->x, ss->y, ss->width,
                               ss->height, nullptr, -1, -1);
    }
  }

  // Track the reference count so we know when to free the sprite.
  ss->ref_count++;

  return ss->sprite;
}

/**
 * Finds the first sprite matching a list of possible names and returns it.
 * Aborts when a required sprite is not found; otherwise, warns and returns
 * nullptr.
 */
QPixmap *load_sprite(struct tileset *t, const QStringList &possible_names,
                     bool required, bool verbose)
{
  // go through the list of possible names until
  // you find a sprite that exists.
  for (const auto &name : possible_names) {
    auto sprite = load_sprite(t, name);
    if (sprite) {
      return sprite;
    }
  }

  if (verbose) {
    // TODO Qt6
    // We should be able to remove the line below and
    // update the tileset_errors in the if statement.
    QVector<QString> names_vec(possible_names.begin(), possible_names.end());
    // if sprite couldn't be found and it is required, crash, else warn.
    if (required) {
      tileset_error(t, LOG_FATAL,
                    _("Could not find required sprite matching %s"),
                    qUtf8Printable(strvec_to_or_list(names_vec)));
    } else {
      tileset_error(t, LOG_NORMAL,
                    _("Could not find optional sprite matching %s"),
                    qUtf8Printable(strvec_to_or_list(names_vec)));
    }
  }

  return nullptr;
}

/**
   Unloads the sprite. Decrease the reference counter. If the last
   reference is removed the sprite is freed.
 */
static void unload_sprite(struct tileset *t, const QString &tag_name)
{
  struct small_sprite *ss = t->sprite_hash.value(tag_name);
  fc_assert_ret(ss);
  fc_assert_ret(ss->ref_count >= 1);
  fc_assert_ret(ss->sprite);

  ss->ref_count--;

  if (ss->ref_count == 0) {
    /* Nobody's using the sprite anymore, so we should free it.  We know
     * where to find it if we need it again. */
    // log_debug("freeing sprite '%s'.", tag_name);
    delete ss->sprite;
    ss->sprite = nullptr;
  }
}

/**
 * finds the first sprite matching a list of possible names
 * and returns it to the field argument.
 */
static void assign_sprite(struct tileset *t, QPixmap *&field,
                          const QStringList &possible_names, bool required)
{
  field = load_sprite(t, possible_names, required);
}

/**
 * Goes through the possible digits and assigns them.
 * See assign_digit_sprites()
 */
static void assign_digit_sprites_helper(struct tileset *t,
                                        QPixmap *sprites[NUM_TILES_DIGITS],
                                        const QStringList &patterns,
                                        const QString &suffix, bool required)
{
  for (int i = 0; i < NUM_TILES_DIGITS; i++) {
    QStringList names;
    for (const auto &pattern : patterns) {
      names.append(pattern.arg(QString::number(i)) + suffix);
    }
    assign_sprite(t, sprites[i], {names}, required);
  }
}

/**
 * Assigns the digits for city or go-to orders, for units, tens,
 * and hundreds (i.e. up to 999)
 */
void assign_digit_sprites(struct tileset *t,
                          QPixmap *units[NUM_TILES_DIGITS],
                          QPixmap *tens[NUM_TILES_DIGITS],
                          QPixmap *hundreds[NUM_TILES_DIGITS],
                          const QStringList &patterns)
{
  assign_digit_sprites_helper(t, units, patterns, QStringLiteral(), true);
  assign_digit_sprites_helper(t, tens, patterns, QStringLiteral("0"), true);
  assign_digit_sprites_helper(t, hundreds, patterns, QStringLiteral("00"),
                              false);
}

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
    t->sprites.specialist[id].sprite[j] = load_sprite(t, buffer);

    // Break if no more index specific sprites are defined
    if (!t->sprites.specialist[id].sprite[j]) {
      break;
    }
  }

  if (j == 0) {
    // Try non-indexed
    t->sprites.specialist[id].sprite[j] = load_sprite(t, tag);

    if (t->sprites.specialist[id].sprite[j]) {
      j = 1;
    }
  }

  if (j == 0) {
    // Try the alt tag
    for (j = 0; j < MAX_NUM_CITIZEN_SPRITES; j++) {
      // Try alt tag name + index number
      buffer = QStringLiteral("%1_%2").arg(graphic_alt, QString::number(j));
      t->sprites.specialist[id].sprite[j] = load_sprite(t, buffer);

      // Break if no more index specific sprites are defined
      if (!t->sprites.specialist[id].sprite[j]) {
        break;
      }
    }
  }

  if (j == 0) {
    // Try alt tag non-indexed
    t->sprites.specialist[id].sprite[j] = load_sprite(t, graphic_alt);

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
      t->sprites.citizen[i].sprite[j] = load_sprite(t, buffer);
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
   Initialize 'sprites' structure based on hardwired tags which
   freeciv always requires.
 */
static void tileset_lookup_sprite_tags(struct tileset *t)
{
  QString buffer, buffer2;
  int i, j, f;

  fc_assert_ret(!t->sprite_hash.isEmpty());

  assign_sprite(t, t->sprites.treaty_thumb[0],
                {"treaty.disagree_thumb_down"}, true);
  assign_sprite(t, t->sprites.treaty_thumb[1], {"treaty.agree_thumb_up"},
                true);

  for (j = 0; j < INDICATOR_COUNT; j++) {
    const char *names[] = {"science_bulb", "warming_sun", "cooling_flake"};

    for (i = 0; i < NUM_TILES_PROGRESS; i++) {
      buffer = QStringLiteral("s.%1_%2").arg(names[j], QString::number(i));
      assign_sprite(t, t->sprites.indicator[j][i], {buffer}, true);
    }
  }

  assign_sprite(t, t->sprites.arrow[ARROW_RIGHT], {"s.right_arrow"}, true);
  assign_sprite(t, t->sprites.arrow[ARROW_PLUS], {"s.plus"}, true);
  assign_sprite(t, t->sprites.arrow[ARROW_MINUS], {"s.minus"}, true);
  if (t->type == TS_ISOMETRIC) {
    assign_sprite(t, t->sprites.dither_tile, {"t.dither_tile"}, true);
  }

  if (tileset_is_isometric(tileset)) {
    assign_sprite(t, t->sprites.mask.tile, {"mask.tile"}, true);
  } else {
    assign_sprite(t, t->sprites.mask.tile, {"mask.tile"}, true);
  }

  assign_sprite(t, t->sprites.tax_luxury, {"s.tax_luxury"}, true);
  assign_sprite(t, t->sprites.tax_science, {"s.tax_science"}, true);
  assign_sprite(t, t->sprites.tax_gold, {"s.tax_gold"}, true);

  tileset_setup_citizen_types(t);

  for (i = 0; i < SPACESHIP_COUNT; i++) {
    const char *names[SPACESHIP_COUNT] = {
        "solar_panels", "life_support", "habitation", "structural",
        "fuel",         "propulsion",   "exhaust"};

    buffer = QStringLiteral("spaceship.%1").arg(names[i]);
    assign_sprite(t, t->sprites.spaceship[i], {buffer}, true);
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
      assign_sprite(t, t->sprites.cursor[i].frame[f], {buffer}, true);
      ss = t->sprite_hash.value(buffer, nullptr);
      if (ss) {
        t->sprites.cursor[i].hot_x = ss->hot_x;
        t->sprites.cursor[i].hot_y = ss->hot_y;
      }
    }
  }

  for (i = 0; i < E_COUNT; i++) {
    const char *tag = get_event_tag(static_cast<event_type>(i));
    assign_sprite(t, t->sprites.events[i], {tag}, true);
  }

  assign_sprite(t, t->sprites.explode.nuke, {"explode.nuke"}, true);

  for (i = 0;; i++) {
    QPixmap *sprite;

    buffer = QStringLiteral("explode.unit_%1").arg(QString::number(i));
    sprite = load_sprite(t, buffer);
    if (!sprite) {
      break;
    }
    t->sprites.explode.unit.push_back(sprite);
  }

  assign_sprite(t, t->sprites.citybar.shields, {"citybar.shields"}, true);
  assign_sprite(t, t->sprites.citybar.food, {"citybar.food"}, true);
  assign_sprite(t, t->sprites.citybar.trade, {"citybar.trade"}, true);
  assign_sprite(t, t->sprites.citybar.occupied, {"citybar.occupied"}, true);
  assign_sprite(t, t->sprites.citybar.background, {"citybar.background"},
                true);
  for (i = 0;; i++) {
    QPixmap *sprite;

    buffer = QStringLiteral("citybar.occupancy_%1").arg(QString::number(i));
    sprite = load_sprite(t, buffer);
    if (!sprite) {
      break;
    }
    t->sprites.citybar.occupancy.push_back(sprite);
  }
  if (t->sprites.citybar.occupancy.size() < 2) {
    tileset_error(t, LOG_FATAL,
                  _("Missing necessary citybar.occupancy_N sprites."));
  }

  assign_sprite(t, t->sprites.editor.erase, {"editor.erase"}, true);
  assign_sprite(t, t->sprites.editor.brush, {"editor.brush"}, true);
  assign_sprite(t, t->sprites.editor.copy, {"editor.copy"}, true);
  assign_sprite(t, t->sprites.editor.paste, {"editor.paste"}, true);
  assign_sprite(t, t->sprites.editor.copypaste, {"editor.copypaste"}, true);
  assign_sprite(t, t->sprites.editor.startpos, {"editor.startpos"}, true);
  assign_sprite(t, t->sprites.editor.terrain, {"editor.terrain"}, true);
  assign_sprite(t, t->sprites.editor.terrain_resource,
                {"editor.terrain_resource"}, true);
  assign_sprite(t, t->sprites.editor.terrain_special,
                {"editor.terrain_special"}, true);
  assign_sprite(t, t->sprites.editor.unit, {"editor.unit"}, true);
  assign_sprite(t, t->sprites.editor.city, {"editor.city"}, true);
  assign_sprite(t, t->sprites.editor.vision, {"editor.vision"}, true);
  assign_sprite(t, t->sprites.editor.territory, {"editor.territory"}, true);
  assign_sprite(t, t->sprites.editor.properties, {"editor.properties"},
                true);
  assign_sprite(t, t->sprites.editor.road, {"editor.road"}, true);
  assign_sprite(t, t->sprites.editor.military_base, {"editor.military_base"},
                true);

  // Must have at least one upkeep sprite per output type (and unhappy)
  // The rest are optional.
  buffer = QStringLiteral("upkeep.unhappy");
  t->sprites.upkeep.unhappy.push_back(load_sprite(t, buffer));
  if (!t->sprites.upkeep.unhappy.back()) {
    tileset_error(t, LOG_FATAL, "Missing sprite upkeep.unhappy");
  }
  // Start from "upkeep.unhappy2"; there is no "upkeep.unhappy1"
  for (i = 2;; i++) {
    buffer = QStringLiteral("upkeep.unhappy%1").arg(QString::number(i));
    auto sprite = load_sprite(t, buffer);
    if (!sprite) {
      break;
    }
    t->sprites.upkeep.unhappy.push_back(sprite);
  }
  output_type_iterate(o)
  {
    buffer = QStringLiteral("upkeep.%1")
                 .arg(get_output_identifier(static_cast<Output_type_id>(o)));
    auto sprite = load_sprite(t, buffer);
    if (sprite) {
      t->sprites.upkeep.output[o].push_back(sprite);
      // Start from "upkeep.food2"; there is no "upkeep.food1"
      for (i = 2;; i++) {
        buffer =
            QStringLiteral("upkeep.%1%2")
                .arg(get_output_identifier(static_cast<Output_type_id>(o)),
                     QString::number(i));
        auto sprite = load_sprite(t, buffer);
        if (!sprite) {
          break;
        }
        t->sprites.upkeep.output[o].push_back(sprite);
      }
    }
  }
  output_type_iterate_end;

  t->max_upkeep_height = calculate_max_upkeep_height(t);

  assign_sprite(t, t->sprites.user.attention, {"user.attention"}, true);

  // Initialize all class-based layers
  for (auto &layer : t->layers) {
    layer->load_sprites();
  }
}

/**
   Frees any internal buffers which are created by load_sprite. Should
   be called after the last (for a given period of time) load_sprite
   call.  This saves a fair amount of memory, but it will take extra time
   the next time we start loading sprites again.
 */
void finish_loading_sprites(struct tileset *t)
{
  for (auto *sf : std::as_const(t->specfiles)) {
    if (sf->big_sprite) {
      delete sf->big_sprite;
      sf->big_sprite = nullptr;
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
  fc_assert_ret(t != nullptr);
  tileset_lookup_sprite_tags(t);
  finish_loading_sprites(t);
}

/**
   Lookup sprite to match tag, or else to match alt if don't find,
   or else return nullptr, and emit log message.
 */
QPixmap *tiles_lookup_sprite_tag_alt(struct tileset *t, QtMsgType level,
                                     const char *tag, const char *alt,
                                     const char *what, const char *name,
                                     bool scale)
{
  QPixmap *sp;

  // (should get sprite_hash before connection)
  fc_assert_ret_val_msg(!t->sprite_hash.isEmpty(), nullptr,
                        "attempt to lookup for %s \"%s\" before "
                        "sprite_hash setup",
                        what, name);

  sp = load_sprite(t, tag);
  if (sp) {
    return sp;
  }

  sp = load_sprite(t, alt);
  if (sp) {
    qCDebug(tileset_category,
            "Using alternate graphic \"%s\" "
            "(instead of \"%s\") for %s \"%s\".",
            alt, tag, what, name);
    return sp;
  }

  tileset_error(
      t, level,
      _("Don't have graphics tags \"%s\" or \"%s\" for %s \"%s\"."), tag,
      alt, what, name);

  return nullptr;
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
  auto sprite = load_sprite(t, buf);
  if (!sprite) {
    return false;
  }

  t->sprites.units.facing[uidx][dir] =
      std::make_unique<freeciv::colorizer>(*sprite, t->replaced_hue);
  return true;
}

/**
   Try to setup all unit type sprites from single tag
 */
static bool tileset_setup_unit_type_from_tag(struct tileset *t, int uidx,
                                             const char *tag)
{
  bool has_icon, facing_sprites = true;

  auto icon = load_sprite(t, tag);
  has_icon = icon != nullptr;
  if (has_icon) {
    t->sprites.units.icon[uidx] =
        std::make_unique<freeciv::colorizer>(*icon, t->replaced_hue);
  }

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
                != nullptr);
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

  if (!t->sprites.building[improvement_index(pimprove)]) {
    t->sprites.building[improvement_index(pimprove)] = make_error_pixmap();
  }
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

    if (!t->sprites.tech[advance_index(padvance)]) {
      t->sprites.tech[advance_index(padvance)] = make_error_pixmap();
    }
  } else {
    t->sprites.tech[advance_index(padvance)] = nullptr;
  }
}

/**
 * Make the list of possible tag names for the extras which
 * may vary depending on the terrain they're on.
 */
QStringList make_tag_terrain_list(const QString &prefix,
                                  const QString &suffix,
                                  const struct terrain *pterrain)
{
  if (!strlen(pterrain->graphic_alt)
      || pterrain->graphic_alt == QStringLiteral("-")) {
    return {
        QStringLiteral("%1_%2%3").arg(prefix, pterrain->graphic_str, suffix),
        QStringLiteral("%1%2").arg(prefix, suffix),
    };
  } else {
    return {
        QStringLiteral("%1_%2%3").arg(prefix, pterrain->graphic_str, suffix),
        QStringLiteral("%1_%2%3").arg(prefix, pterrain->graphic_alt, suffix),
        QStringLiteral("%1%2").arg(prefix, suffix),
    };
  }
}

/**
   Set extra sprite values; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_extra(struct tileset *t, struct extra_type *pextra)
{
  const int id = extra_index(pextra);

  if (!fc_strcasecmp(pextra->graphic_str, "none")) {
    // Extra without graphics; nothing to do
  } else {
    const char *tag;

    tag = pextra->graphic_str;
    if (!t->estyle_hash.contains(tag)) {
      tag = pextra->graphic_alt;
      if (!t->estyle_hash.contains(tag)) {
        tileset_error(
            t, LOG_FATAL,
            _("Missing entry for \"%s\" or \"%s\" in extras.styles in %s."),
            pextra->graphic_str, pextra->graphic_alt,
            qUtf8Printable(t->file_name));
      } else {
        qCDebug(tileset_category,
                "Using alternate graphic \"%s\" "
                "(instead of \"%s\") for extra \"%s\".",
                pextra->graphic_alt, pextra->graphic_str,
                extra_rule_name(pextra));
      }
    }
    auto extrastyle = t->estyle_hash.value(tag);

    // Also init modern class-based layers
    for (auto &layer : t->layers) {
      layer->initialize_extra(pextra, tag, extrastyle);
    }
  }

  if (!fc_strcasecmp(pextra->activity_gfx, "none")) {
    t->sprites.extras[id].activity = nullptr;
  } else {
    QStringList tags = {
        pextra->activity_gfx,
        pextra->act_gfx_alt,
        pextra->act_gfx_alt2,
    };
    assign_sprite(t, t->sprites.extras[id].activity, tags, true);
  }

  if (!fc_strcasecmp(pextra->rmact_gfx, "none")) {
    t->sprites.extras[id].rmact = nullptr;
  } else {
    QStringList tags = {
        pextra->rmact_gfx,
        pextra->rmact_gfx_alt,
    };
    assign_sprite(t, t->sprites.extras[id].rmact, tags, true);
  }
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

  // should probably do something if nullptr, eg generic default?
}

/**
   Set nation flag sprite value; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_nation_flag(struct tileset *t, struct nation_type *nation)
{
  const char *tags[] = {nation->flag_graphic_str, nation->flag_graphic_alt,
                        "unknown", nullptr};
  int i;
  QPixmap *flag = nullptr, *shield = nullptr;
  QString buf;

  for (i = 0; tags[i] && !flag; i++) {
    buf = QStringLiteral("f.%1").arg(tags[i]);
    flag = load_sprite(t, buf);
  }
  for (i = 0; tags[i] && !shield; i++) {
    buf = QStringLiteral("f.shield.%1").arg(tags[i]);
    shield = load_sprite(t, buf);
  }
  if (!flag || !shield) {
    // Should never get here because of the f.unknown fallback.
    tileset_error(t, LOG_FATAL, _("Nation %s: no national flag."),
                  nation_rule_name(nation));
  }

  t->sprites.nation_flag.resize(game.control.nation_count);
  t->sprites.nation_flag[nation_index(nation)] = flag;

  t->sprites.nation_shield.resize(game.control.nation_count);
  t->sprites.nation_shield[nation_index(nation)] = shield;
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
QPixmap *get_unit_nation_flag_sprite(const struct tileset *t,
                                     const struct unit *punit)
{
  struct nation_type *pnation = nation_of_unit(punit);

  if (gui_options->draw_unit_shields) {
    return t->sprites.nation_shield[nation_index(pnation)];
  } else {
    return t->sprites.nation_flag[nation_index(pnation)];
  }
}

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

      if (nullptr != terrain1) {
        tterrain_near[dir] = terrain1;
        textras_near[dir] = *tile_extras(tile1);
        continue;
      }
      qCCritical(tileset_category,
                 "build_tile_data() tile (%d,%d) has no terrain!",
                 TILE_XY(tile1));
    }
    /* At the edges of the (known) map, pretend the same terrain continued
     * past the edge of the map. */
    tterrain_near[dir] = pterrain;
    BV_CLR_ALL(textras_near[dir]);
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
  return gui_options->draw_city_outlines && unit_is_cityfounder(punit)
         && !unit_has_orders(punit)
         && (client_tile_get_known(unit_tile(punit)) != TILE_UNKNOWN
             && city_can_be_built_here(unit_tile(punit), punit))
         && (!check_focus || unit_is_in_focus(punit));
}

/**
   Should the given extra be drawn
   FIXME: Some extras can not be switched
 */
bool is_extra_drawing_enabled(const extra_type *pextra)
{
  bool no_disable = true; // Draw if matches no cause

  if (is_extra_caused_by(pextra, EC_IRRIGATION)) {
    if (gui_options->draw_irrigation) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_POLLUTION)
      || is_extra_caused_by(pextra, EC_FALLOUT)) {
    if (gui_options->draw_pollution) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_MINE)) {
    if (gui_options->draw_mines) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_RESOURCE)) {
    if (gui_options->draw_specials) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_removed_by(pextra, ERM_ENTER)) {
    if (gui_options->draw_huts) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_BASE)) {
    if (gui_options->draw_fortress_airbase) {
      return true;
    }
    no_disable = false;
  }
  if (is_extra_caused_by(pextra, EC_ROAD)) {
    if (gui_options->draw_roads_rails) {
      return true;
    }
    no_disable = false;
  }

  return no_disable;
}

/**
   Set city tiles sprite values; should only happen after
   tilespec_load_tiles().
 */
void tileset_setup_city_tiles(struct tileset *t, int style)
{
  for (auto &&layer : t->layers) {
    layer->initialize_city_style(city_styles[style], style);
  }
}

/**
   Return the amount of time between calls to toggle_focus_unit_state.
   The main loop needs to call toggle_focus_unit_state about this often
   to do the active-unit animation.
 */
int get_focus_unit_toggle_timeout(const struct tileset *t)
{
  if (t->focus_units_layer->focus_unit_state_count() == 0) {
    return 100;
  } else {
    return t->select_step_ms;
  }
}

/**
   Reset the focus unit state.  This should be called when changing
   focus units.
 */
void reset_focus_unit_state(struct tileset *t)
{
  t->focus_units_layer->focus_unit_state() = 0;
}

/**
   Setup tileset for showing combat where focus unit participates.
 */
void focus_unit_in_combat(struct tileset *t)
{
  if (t->focus_units_layer->focus_unit_state_count() == 0) {
    reset_focus_unit_state(t);
  }
}

/**
   Toggle/increment the focus unit state.  This should be called once
   every get_focus_unit_toggle_timeout() seconds.
 */
void toggle_focus_unit_state(struct tileset *t)
{
  t->focus_units_layer->focus_unit_state()++;
  if (t->focus_units_layer->focus_unit_state_count() == 0) {
    t->focus_units_layer->focus_unit_state() %= 2;
  } else {
    t->focus_units_layer->focus_unit_state() %=
        t->focus_units_layer->focus_unit_state_count();
  }
}

/**
   Find unit that we can display from given tile.
 */
struct unit *get_drawable_unit(const struct tileset *t, const ::tile *ptile)
{
  struct unit *punit = find_visible_unit(ptile);

  if (punit == nullptr) {
    return nullptr;
  }

  if (!unit_is_in_focus(punit)
      || t->focus_units_layer->focus_unit_state_count() > 0
      || t->focus_units_layer->focus_unit_state() == 0) {
    return punit;
  } else {
    return nullptr;
  }
}

/**
   This patch unloads all sprites from the sprite hash (the hash itself
   is left intact).
 */
static void unload_all_sprites(struct tileset *t)
{
  QHash<QString, small_sprite *>::const_iterator i =
      t->sprite_hash.constBegin();
  while (i != t->sprite_hash.constEnd()) {
    QByteArray qba = i.key().toLocal8Bit();
    while (i.value()->ref_count > 0) {
      unload_sprite(t, qba.data());
    }
    i++;
  }
}

/**
   Free all sprites from tileset.
 */
void tileset_free_tiles(struct tileset *t)
{
  log_debug("tileset_free_tiles()");

  unload_all_sprites(t);

  t->sprite_hash.clear();

  for (auto *ss : std::as_const(t->small_sprites)) {
    if (ss->file) {
      delete[] ss->file;
    }
    fc_assert(ss->sprite == nullptr);
    delete ss;
  }
  t->small_sprites.clear();

  for (auto *sf : std::as_const(t->specfiles)) {
    delete[] sf->file_name;
    if (sf->big_sprite) {
      delete sf->big_sprite;
      sf->big_sprite = nullptr;
    }
    delete sf;
  }
  t->specfiles.clear();

  t->sprites.explode.unit.clear();
  t->sprites.nation_flag.clear();
  t->sprites.nation_shield.clear();
  t->sprites.citybar.occupancy.clear();
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
   citizen's city can be used to determine which sprite to use (a nullptr
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

  if (pcity != nullptr) {
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
    return nullptr;
  }

  return graphic->sprite[gfx_index % graphic->count];
}

/**
   Return the sprite for the nation.
 */
const QPixmap *get_nation_flag_sprite(const struct tileset *t,
                                      const struct nation_type *pnation)
{
  return t->sprites.nation_flag[nation_index(pnation)];
}

/**
   Return the shield sprite for the nation.
 */
const QPixmap *get_nation_shield_sprite(const struct tileset *t,
                                        const struct nation_type *pnation)
{
  return t->sprites.nation_shield[nation_index(pnation)];
}

/**
   Return the sprite for the technology/advance.
 */
const QPixmap *get_tech_sprite(const struct tileset *t, Tech_type_id tech)
{
  fc_assert_ret_val(0 <= tech && tech < advance_count(), nullptr);
  return t->sprites.tech[tech];
}

/**
   Return the sprite for the building/improvement.
 */
const QPixmap *get_building_sprite(const struct tileset *t,
                                   const struct impr_type *pimprove)
{
  fc_assert_ret_val(nullptr != pimprove, nullptr);
  return t->sprites.building[improvement_index(pimprove)];
}

/**
   Return the sprite for the government.
 */
const QPixmap *get_government_sprite(const struct tileset *t,
                                     const struct government *gov)
{
  fc_assert_ret_val(nullptr != gov, nullptr);
  return t->sprites.government[government_index(gov)];
}

/**
   Return the sprite for the unit type (the base "unit" sprite).
   If 'facing' is direction8_invalid(), will use an unoriented sprite or
   a default orientation.
 */
const QPixmap *get_unittype_sprite(const struct tileset *t,
                                   const struct unit_type *punittype,
                                   enum direction8 facing,
                                   const QColor &replace)
{
  int uidx = utype_index(punittype);
  bool icon = !direction8_is_valid(facing);

  fc_assert_ret_val(nullptr != punittype, nullptr);

  if (!direction8_is_valid(facing) || !is_valid_dir(facing)) {
    facing = t->unit_default_orientation;
    /* May not have been specified, but it only matters if we don't
     * turn out to have an icon sprite */
  }

  if (t->sprites.units.icon[uidx]
      && (icon || t->sprites.units.facing[uidx][facing] == nullptr)) {
    // Has icon sprite, and we prefer to (or must) use it
    return t->sprites.units.icon[uidx]->pixmap(replace);
  } else {
    /* We should have a valid orientation by now. Failure to have either
     * an icon sprite or default orientation should have been caught at
     * tileset load. */
    fc_assert_ret_val(direction8_is_valid(facing), nullptr);
    return t->sprites.units.facing[uidx][facing]->pixmap(replace);
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
  return nullptr;
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
const std::vector<QPixmap *> &
get_unit_explode_animation(const struct tileset *t)
{
  return t->sprites.explode.unit;
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

  fc_assert_ret_val(indicator >= 0 && indicator < INDICATOR_COUNT, nullptr);

  return t->sprites.indicator[indicator][idx];
}

/**
   Return a sprite for the unhappiness of the unit - to be shown as an
   overlay on the unit in the city support dialog, for instance.

   May return nullptr if there's no unhappiness.
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
    return nullptr;
  }
}

/**
   Return a sprite for the upkeep of the unit - to be shown as an overlay
   on the unit in the city support dialog, for instance.

   May return nullptr if there's no upkeep of the kind.
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
    return nullptr;
  }
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
void tileset_init(struct tileset *t) { t->max_upkeep_height = 0; }

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
    const auto lsprs =
        layer->fill_sprite_array(tile, nullptr, nullptr, nullptr);
    sprs.insert(sprs.end(), lsprs.begin(), lsprs.end());
  }

  tile_virtual_destroy(tile);
  return sprs;
}

/**
 * Returns the layer_city of the tileset.
 */
const freeciv::layer_city *tileset_layer_city(const struct tileset *t)
{
  for (const auto &layer : t->layers) {
    if (auto lc = dynamic_cast<const freeciv::layer_city *>(layer.get())) {
      return lc;
    }
  }
  fc_assert_ret_val(false, nullptr);
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
    const auto lsprs =
        layer->fill_sprite_array(tile, nullptr, nullptr, nullptr);
    sprs.insert(sprs.end(), lsprs.begin(), lsprs.end());
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
   Setup tiles for one player using the player color.
 */
void tileset_player_init(struct tileset *t, struct player *pplayer)
{
  fc_assert_ret(pplayer != nullptr);

  auto plrid = player_index(pplayer);
  fc_assert_ret(plrid >= 0);
  fc_assert_ret(plrid < MAX_NUM_PLAYER_SLOTS);

  // Free all data before recreating it.
  tileset_player_free(t, plrid);

  for (auto &&layer : t->layers) {
    layer->initialize_player(pplayer);
  }
}

/**
   Free tiles for one player using the player color.
 */
static void tileset_player_free(struct tileset *t, int plrid)
{
  fc_assert_ret(plrid >= 0);
  fc_assert_ret(plrid < MAX_NUM_PLAYER_SLOTS);

  for (auto &&layer : t->layers) {
    layer->free_player(plrid);
  }
}

/**
   Reset tileset data specific to ruleset.
 */
void tileset_ruleset_reset(struct tileset *t)
{
  for (auto &layer : t->layers) {
    layer->reset_ruleset();
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
 * Checks if the tileset has any user-settable options.
 */
bool tileset_has_options(const struct tileset *t)
{
  return !t->options.empty();
}

/**
 * Checks if the tileset has supports the given user-settable option.
 */
bool tileset_has_option(const struct tileset *t, const QString &option)
{
  return t->options.count(option);
}

/**
 * Gets the user-settable options of the tileset.
 */
std::map<QString, tileset_option>
tileset_get_options(const struct tileset *t)
{
  return t->options;
}

/**
 * Checks if an user-settable tileset option is enabled.
 *
 * The option must exist in the tileset.
 */
bool tileset_option_is_enabled(const struct tileset *t, const QString &name)
{
  return t->options.at(name).enabled;
}

/**
 * Enable or disable a user-settable tileset option.
 *
 * The tileset may be reloaded as a result, invalidating \c t.
 *
 * Returns false if the option does not exist. The game must have been
 * initialized before calling this.
 */
bool tileset_set_option(struct tileset *t, const QString &name, bool enabled)
{
  auto it = t->options.find(name);
  fc_assert_ret_val(it != t->options.end(), false);

  if (it->second.enabled != enabled) {
    // Change the value in the client settings
    gui_options->tileset_options[tileset_basename(t)][name] = enabled;
    tilespec_reread_frozen_refresh(t->name);
  }
  return true;
}

/**
   Return tileset name
 */
const char *tileset_name_get(const struct tileset *t)
{
  return t->given_name;
}

/**
   Return tileset version
 */
const char *tileset_version(const struct tileset *t) { return t->version; }

/**
   Return tileset description summary
 */
const char *tileset_summary(const struct tileset *t) { return t->summary; }

/**
   Return tileset description body
 */
const char *tileset_description(const struct tileset *t)
{
  return t->description;
}

/**
   Return tileset topology index
 */
int tileset_topo_index(const struct tileset *t) { return t->ts_topo_idx; }

/**
 * Creates the help item for the given tileset
 */
help_item *tileset_help(const struct tileset *t)
{
  if (t == nullptr) {
    return nullptr;
  }

  int desc_len;
  int len;

  const char *ts_name = tileset_name_get(t);
  const char *version = tileset_version(t);
  const char *summary = tileset_summary(t);
  const char *description = tileset_description(t);

  auto pitem = new_help_item(HELP_TILESET);
  if (description != nullptr) {
    desc_len = qstrlen("\n\n") + qstrlen(description);
  } else {
    desc_len = 0;
  }
  if (summary != nullptr) {
    if (version[0] != '\0') {
      len = qstrlen(_(ts_name)) + qstrlen(" ") + qstrlen(version)
            + qstrlen("\n\n") + qstrlen(_(summary)) + 1;

      pitem->text = new char[len + desc_len];
      fc_snprintf(pitem->text, len, "%s %s\n\n%s", _(ts_name), version,
                  _(summary));
    } else {
      len = qstrlen(_(ts_name)) + qstrlen("\n\n") + qstrlen(_(summary)) + 1;

      pitem->text = new char[len + desc_len];
      fc_snprintf(pitem->text, len, "%s\n\n%s", _(ts_name), _(summary));
    }
  } else {
    const char *nodesc = _("Current tileset contains no summary.");

    if (version[0] != '\0') {
      len = qstrlen(_(ts_name)) + qstrlen(" ") + qstrlen(version)
            + qstrlen("\n\n") + qstrlen(nodesc) + 1;

      pitem->text = new char[len + desc_len];
      fc_snprintf(pitem->text, len, "%s %s\n\n%s", _(ts_name), version,
                  nodesc);
    } else {
      len = qstrlen(_(ts_name)) + qstrlen("\n\n") + qstrlen(nodesc) + 1;

      pitem->text = new char[len + desc_len];
      fc_snprintf(pitem->text, len, "%s\n\n%s", _(ts_name), nodesc);
    }
  }
  if (description != nullptr) {
    fc_strlcat(pitem->text, "\n\n", len + desc_len);
    fc_strlcat(pitem->text, description, len + desc_len);
  }
  return pitem;
}
