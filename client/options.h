/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <QFont>

// common
#include "events.h"
#include "featured_text.h" // struct ft_color
#include "mapimg.h"

#define DEFAULT_METASERVER_OPTION "default"

enum overview_layers {
  OLAYER_BACKGROUND,
  OLAYER_RELIEF,
  OLAYER_BORDERS,
  OLAYER_BORDERS_ON_OCEAN,
  OLAYER_UNITS,
  OLAYER_CITIES,
  OLAYER_COUNT
};

// Holds all information about the overview aka minimap.
struct overview {
  // The following fields are controlled by mapview_common.c.
  double map_x0, map_y0; // Origin of the overview, in natural coords.
  int width, height;     // Size in pixels.

  // Holds the map, unwrapped.
  QPixmap *map;

  // A backing store for the window itself, wrapped.
  QPixmap *window;

  bool fog;
  bool layers[OLAYER_COUNT];
};

struct client_options {
  char default_user_name[512] = "\0";
  char default_server_host[512] = "localhost";
  int default_server_port = DEFAULT_SOCK_PORT;
  bool use_prev_server = false;
  bool heartbeat_enabled = false;
  char default_metaserver[512] = DEFAULT_METASERVER_OPTION;
  char default_tileset_square_name[512] = "\0";
  char default_tileset_hex_name[512] = "\0";
  char default_tileset_isohex_name[512] = "\0";
  char default_city_bar_style_name[512] = "Simple";
  char default_sound_set_name[512] = "stdsounds";
  char default_music_set_name[512] = "stdmusic";
  char default_sound_plugin_name[512] = "\0";
  char default_chat_logfile[512] = "freeciv-chat.log";

  bool save_options_on_exit = true;

  /** Migrations **/
  bool first_boot = true; /* There was no earlier options saved.
                           * This affects some migrations, but not all. */
  char default_tileset_overhead_name[512] =
      "\0"; /* 2.6 had separate tilesets for
        ... */
  char default_tileset_iso_name[512] =
      "\0"; // ...overhead and iso topologies.

  bool migrate_fullscreen = false;

  /** Local Options: **/

  bool solid_color_behind_units = false;
  bool sound_bell_at_new_turn = false;
  int smooth_move_unit_msec = 30;
  int smooth_center_slide_msec = 200;
  int smooth_combat_step_msec = 10;
  bool ai_manual_turn_done = true;
  bool auto_center_on_unit = true;
  bool auto_center_on_automated = true;
  bool auto_center_on_combat = false;
  bool auto_center_each_turn = true;
  bool wakeup_focus = true;
  bool goto_into_unknown = true;
  bool center_when_popup_city = true;
  bool show_previous_turn_messages = true;
  bool concise_city_production = false;
  bool auto_turn_done = false;
  bool ask_city_name = true;
  bool popup_new_cities = true;
  bool popup_actor_arrival = true;
  bool popup_attack_actions = true;
  bool popup_last_move_to_allied = true;
  bool keyboardless_goto = false;
  bool enable_cursor_changes = true;
  bool separate_unit_selection = false;
  bool unit_selection_clears_orders = true;
  struct ft_color highlight_our_names = FT_COLOR("#000000", "#FFFF00");

  bool voteinfo_bar_use = true;
  bool voteinfo_bar_always_show = false;
  bool voteinfo_bar_hide_when_not_player = false;
  bool voteinfo_bar_new_at_front = false;

  bool autoaccept_tileset_suggestion = false;
  bool autoaccept_soundset_suggestion = false;
  bool autoaccept_musicset_suggestion = false;

  bool sound_enable_effects = true;
  bool sound_enable_menu_music = true;
  bool sound_enable_game_music = true;
  int sound_effects_volume = 50;

  bool draw_city_outlines = true;
  bool draw_city_output = false;
  bool draw_map_grid = false;
  bool draw_city_names = true;
  bool draw_city_growth = true;
  bool draw_city_productions = true;
  bool draw_city_buycost = false;
  bool draw_city_trade_routes = false;
  bool draw_coastline = false;
  bool draw_roads_rails = true;
  bool draw_irrigation = true;
  bool draw_mines = true;
  bool draw_fortress_airbase = true;
  bool draw_specials = true;
  bool draw_huts = true;
  bool draw_pollution = true;
  bool draw_cities = true;
  bool draw_units = true;
  bool draw_focus_unit = false;
  bool draw_fog_of_war = true;
  bool draw_borders = true;
  bool draw_native = false;
  bool draw_unit_shields = true;
  bool zoom_scale_fonts = true;

  bool player_dlg_show_dead_players = true;
  bool reqtree_show_icons = true;
  bool reqtree_curved_lines = false;

  // options for map images
  char mapimg_format[64] = "png";
  int mapimg_zoom = 2;
  bool mapimg_layer[MAPIMG_LAYER_COUNT] = {
      false, // a - MAPIMG_LAYER_AREA
      true,  // b - MAPIMG_LAYER_BORDERS
      true,  // c - MAPIMG_LAYER_CITIES
      true,  // f - MAPIMG_LAYER_FOGOFWAR
      true,  // k - MAPIMG_LAYER_KNOWLEDGE
      true,  // t - MAPIMG_LAYER_TERRAIN
      true   // u - MAPIMG_LAYER_UNITS
  };
  char mapimg_filename[512] = "mapimage filename";

// gui-qt client specific options.
#define FC_QT_DEFAULT_THEME_NAME "NightStalker"
  bool gui_qt_fullscreen = true;
  bool gui_qt_show_preview = true;
  bool gui_qt_allied_chat_only = true;
  int gui_qt_increase_fonts = 0;
  char gui_qt_default_theme_name[512] = "NightStalker";
  QFont gui_qt_font_default;
  QFont gui_qt_font_notify_label;
  QFont gui_qt_font_help_label;
  QFont gui_qt_font_help_text;
  QFont gui_qt_font_chatline;
  QFont gui_qt_font_city_names;
  QFont gui_qt_font_city_productions;
  QFont gui_qt_font_reqtree_text;
  bool gui_qt_show_titlebar = true;

  struct overview overview = {};
};

extern client_options *gui_options;

#define SPECENUM_NAME option_type
#define SPECENUM_VALUE0 OT_BOOLEAN
#define SPECENUM_VALUE1 OT_INTEGER
#define SPECENUM_VALUE2 OT_STRING
#define SPECENUM_VALUE3 OT_ENUM
#define SPECENUM_VALUE4 OT_BITWISE
#define SPECENUM_VALUE5 OT_FONT
#define SPECENUM_VALUE6 OT_COLOR
#include "specenum_gen.h"

struct option;     // Opaque type.
struct option_set; // Opaque type.

typedef void (*option_save_log_callback)(QtMsgType lvl, const QString &msg);

// Main functions.
void options_init();
void options_free();
void server_options_init();
void server_options_free();
void options_load();
void options_save(option_save_log_callback log_cb);

// Option sets.
extern const struct option_set *client_optset;
extern const struct option_set *server_optset;

struct option *optset_option_by_number(const struct option_set *poptset,
                                       int id);
struct option *optset_option_by_name(const struct option_set *poptset,
                                     const char *name);
struct option *optset_option_first(const struct option_set *poptset);

const char *optset_category_name(const struct option_set *poptset,
                                 int category);

// Common option functions.
const struct option_set *option_optset(const struct option *poption);
int option_number(const struct option *poption);
const char *option_name(const struct option *poption);
const char *option_description(const struct option *poption);
QString option_help_text(const struct option *poption);
enum option_type option_type(const struct option *poption);
QString option_category_name(const struct option *poption);
bool option_is_changeable(const struct option *poption);
struct option *option_next(const struct option *poption);

bool option_reset(struct option *poption);
void option_set_changed_callback(struct option *poption,
                                 void (*callback)(struct option *));
void option_changed(struct option *poption);

// Option gui functions.
void option_set_gui_data(struct option *poption, void *data);
void *option_get_gui_data(const struct option *poption);

// Callback assistance
int option_get_cb_data(const struct option *poption);

// Option type OT_BOOLEAN functions.
bool option_bool_get(const struct option *poption);
bool option_bool_def(const struct option *poption);
bool option_bool_set(struct option *poption, bool val);

// Option type OT_INTEGER functions.
int option_int_get(const struct option *poption);
int option_int_def(const struct option *poption);
int option_int_min(const struct option *poption);
int option_int_max(const struct option *poption);
bool option_int_set(struct option *poption, int val);

// Option type OT_STRING functions.
const char *option_str_get(const struct option *poption);
const char *option_str_def(const struct option *poption);
const QVector<QString> *option_str_values(const struct option *poption);
bool option_str_set(struct option *poption, const char *str);

// Option type OT_ENUM functions.
int option_enum_str_to_int(const struct option *poption, const char *str);
QString option_enum_int_to_str(const struct option *poption, int val);
int option_enum_get_int(const struct option *poption);
int option_enum_def_int(const struct option *poption);
bool option_enum_set_int(struct option *poption, int val);

// Option type OT_BITWISE functions.
unsigned option_bitwise_get(const struct option *poption);
unsigned option_bitwise_def(const struct option *poption);
unsigned option_bitwise_mask(const struct option *poption);
const QVector<QString> *option_bitwise_values(const struct option *poption);
bool option_bitwise_set(struct option *poption, unsigned val);

// Option type OT_FONT functions.
QFont option_font_get(const struct option *poption);
QFont option_font_def(const struct option *poption);
void option_font_set_default(const struct option *poption,
                             const QFont &font);
QString option_font_target(const struct option *poption);
bool option_font_set(struct option *poption, const QFont &font);

// Option type OT_COLOR functions.
struct ft_color option_color_get(const struct option *poption);
struct ft_color option_color_def(const struct option *poption);
bool option_color_set(struct option *poption, struct ft_color color);

#define options_iterate(poptset, poption)                                   \
  {                                                                         \
    struct option *poption = optset_option_first(poptset);                  \
    for (; nullptr != poption; poption = option_next(poption)) {

#define options_iterate_end                                                 \
  }                                                                         \
  }

/** Desired settable options. **/
void desired_settable_options_update();
void desired_settable_option_update(const char *op_name,
                                    const char *op_value,
                                    bool allow_replace);

/** Dialog report options. **/
void options_dialogs_update();
void options_dialogs_set();

/** Message Options: **/

// for specifying which event messages go where:
#define NUM_MW 3
#define MW_OUTPUT 1   // add to the output window
#define MW_MESSAGES 2 // add to the messages window
#define MW_POPUP 4    // popup an individual window

extern int messages_where[]; // OR-ed MW_ values [E_COUNT]

/** Client options **/

#define GUI_DEFAULT_MAPIMG_FILENAME "freeciv"

struct tileset;

const char *tileset_name_for_topology(int topology_id);
void fill_topo_ts_default();
