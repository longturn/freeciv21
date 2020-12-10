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

/* utility */
#include "support.h" /* bool type */

/* common */
#include "events.h"
#include "fc_types.h"      /* enum gui_type */
#include "featured_text.h" /* struct ft_color */
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

/* Holds all information about the overview aka minimap. */
struct overview {
  /* The following fields are controlled by mapview_common.c. */
  double map_x0, map_y0; /* Origin of the overview, in natural coords. */
  int width, height;     /* Size in pixels. */

  /* Holds the map, unwrapped. */
  struct canvas *map;

  /* A backing store for the window itself, wrapped. */
  struct canvas *window;

  bool fog;
  bool layers[OLAYER_COUNT];
};

struct client_options {
  char default_user_name[512];
  char default_server_host[512];
  int default_server_port;
  bool use_prev_server;
  bool heartbeat_enabled;
  char default_metaserver[512];
  char default_tileset_square_name[512];
  char default_tileset_hex_name[512];
  char default_tileset_isohex_name[512];
  char default_sound_set_name[512];
  char default_music_set_name[512];
  char default_sound_plugin_name[512];
  char default_chat_logfile[512];

  bool save_options_on_exit;

  /** Migrations **/
  bool first_boot; /* There was no earlier options saved.
                    * This affects some migrations, but not all. */
  char
      default_tileset_name[512]; /* pre-2.6 had just this one tileset name */
  char default_tileset_overhead_name[512]; /* 2.6 had separate tilesets for
                                              ... */
  char default_tileset_iso_name[512]; /* ...overhead and iso topologies. */
  bool gui_qt_migrated_from_2_5;

  bool migrate_fullscreen;

  /** Local Options: **/

  bool solid_color_behind_units;
  bool sound_bell_at_new_turn;
  int smooth_move_unit_msec;
  int smooth_center_slide_msec;
  int smooth_combat_step_msec;
  bool ai_manual_turn_done;
  bool auto_center_on_unit;
  bool auto_center_on_automated;
  bool auto_center_on_combat;
  bool auto_center_each_turn;
  bool wakeup_focus;
  bool goto_into_unknown;
  bool center_when_popup_city;
  bool show_previous_turn_messages;
  bool concise_city_production;
  bool auto_turn_done;
  bool meta_accelerators;
  bool ask_city_name;
  bool popup_new_cities;
  bool popup_actor_arrival;
  bool popup_attack_actions;
  bool popup_last_move_to_allied;
  bool update_city_text_in_refresh_tile;
  bool keyboardless_goto;
  bool enable_cursor_changes;
  bool separate_unit_selection;
  bool unit_selection_clears_orders;
  struct ft_color highlight_our_names;

  bool voteinfo_bar_use;
  bool voteinfo_bar_always_show;
  bool voteinfo_bar_hide_when_not_player;
  bool voteinfo_bar_new_at_front;

  bool autoaccept_tileset_suggestion;
  bool autoaccept_soundset_suggestion;
  bool autoaccept_musicset_suggestion;

  bool sound_enable_effects;
  bool sound_enable_menu_music;
  bool sound_enable_game_music;
  int sound_effects_volume;

  bool draw_city_outlines;
  bool draw_city_output;
  bool draw_map_grid;
  bool draw_city_names;
  bool draw_city_growth;
  bool draw_city_productions;
  bool draw_city_buycost;
  bool draw_city_trade_routes;
  bool draw_terrain;
  bool draw_coastline;
  bool draw_roads_rails;
  bool draw_irrigation;
  bool draw_mines;
  bool draw_fortress_airbase;
  bool draw_specials;
  bool draw_huts;
  bool draw_pollution;
  bool draw_cities;
  bool draw_units;
  bool draw_focus_unit;
  bool draw_fog_of_war;
  bool draw_borders;
  bool draw_native;
  bool draw_full_citybar;
  bool draw_unit_shields;

  bool player_dlg_show_dead_players;
  bool reqtree_show_icons;
  bool reqtree_curved_lines;

  /* options for map images */
  char mapimg_format[64];
  int mapimg_zoom;
  bool mapimg_layer[MAPIMG_LAYER_COUNT];
  char mapimg_filename[512];

/* gui-qt client specific options. */
#define FC_QT_DEFAULT_THEME_NAME "NightStalker"
  bool gui_qt_fullscreen;
  bool gui_qt_show_preview;
  bool gui_qt_allied_chat_only;
  bool gui_qt_sidebar_left;
  int gui_qt_increase_fonts;
  char gui_qt_default_theme_name[512];
  char gui_qt_font_default[512];
  char gui_qt_font_notify_label[512];
  char gui_qt_font_help_label[512];
  char gui_qt_font_help_text[512];
  char gui_qt_font_chatline[512];
  char gui_qt_font_city_names[512];
  char gui_qt_font_city_productions[512];
  char gui_qt_font_reqtree_text[512];
  bool gui_qt_show_titlebar;
  int gui_qt_sidebar_width;

  struct overview overview;
};

extern struct client_options gui_options;

#define SPECENUM_NAME option_type
#define SPECENUM_VALUE0 OT_BOOLEAN
#define SPECENUM_VALUE1 OT_INTEGER
#define SPECENUM_VALUE2 OT_STRING
#define SPECENUM_VALUE3 OT_ENUM
#define SPECENUM_VALUE4 OT_BITWISE
#define SPECENUM_VALUE5 OT_FONT
#define SPECENUM_VALUE6 OT_COLOR
#include "specenum_gen.h"

struct option;     /* Opaque type. */
struct option_set; /* Opaque type. */

typedef void (*option_save_log_callback)(QtMsgType lvl, const QString &msg);

/* Main functions. */
void options_init(void);
void options_free(void);
void server_options_init(void);
void server_options_free(void);
void options_load(void);
void options_save(option_save_log_callback log_cb);

/* Option sets. */
extern const struct option_set *client_optset;
extern const struct option_set *server_optset;

struct option *optset_option_by_number(const struct option_set *poptset,
                                       int id);
#define optset_option_by_index optset_option_by_number
struct option *optset_option_by_name(const struct option_set *poptset,
                                     const char *name);
struct option *optset_option_first(const struct option_set *poptset);

int optset_category_number(const struct option_set *poptset);
const char *optset_category_name(const struct option_set *poptset,
                                 int category);

/* Common option functions. */
const struct option_set *option_optset(const struct option *poption);
int option_number(const struct option *poption);
#define option_index option_number
const char *option_name(const struct option *poption);
const char *option_description(const struct option *poption);
const char *option_help_text(const struct option *poption);
enum option_type option_type(const struct option *poption);
int option_category(const struct option *poption);
const char *option_category_name(const struct option *poption);
bool option_is_changeable(const struct option *poption);
struct option *option_next(const struct option *poption);

bool option_reset(struct option *poption);
void option_set_changed_callback(struct option *poption,
                                 void (*callback)(struct option *));
void option_changed(struct option *poption);

/* Option gui functions. */
void option_set_gui_data(struct option *poption, void *data);
void *option_get_gui_data(const struct option *poption);

/* Callback assistance */
int option_get_cb_data(const struct option *poption);

/* Option type OT_BOOLEAN functions. */
bool option_bool_get(const struct option *poption);
bool option_bool_def(const struct option *poption);
bool option_bool_set(struct option *poption, bool val);

/* Option type OT_INTEGER functions. */
int option_int_get(const struct option *poption);
int option_int_def(const struct option *poption);
int option_int_min(const struct option *poption);
int option_int_max(const struct option *poption);
bool option_int_set(struct option *poption, int val);

/* Option type OT_STRING functions. */
const char *option_str_get(const struct option *poption);
const char *option_str_def(const struct option *poption);
const struct strvec *option_str_values(const struct option *poption);
bool option_str_set(struct option *poption, const char *str);

/* Option type OT_ENUM functions. */
int option_enum_str_to_int(const struct option *poption, const char *str);
const char *option_enum_int_to_str(const struct option *poption, int val);
int option_enum_get_int(const struct option *poption);
const char *option_enum_get_str(const struct option *poption);
int option_enum_def_int(const struct option *poption);
const char *option_enum_def_str(const struct option *poption);
const struct strvec *option_enum_values(const struct option *poption);
bool option_enum_set_int(struct option *poption, int val);
bool option_enum_set_str(struct option *poption, const char *str);

/* Option type OT_BITWISE functions. */
unsigned option_bitwise_get(const struct option *poption);
unsigned option_bitwise_def(const struct option *poption);
unsigned option_bitwise_mask(const struct option *poption);
const struct strvec *option_bitwise_values(const struct option *poption);
bool option_bitwise_set(struct option *poption, unsigned val);

/* Option type OT_FONT functions. */
const char *option_font_get(const struct option *poption);
const char *option_font_def(const struct option *poption);
const char *option_font_target(const struct option *poption);
bool option_font_set(struct option *poption, const char *font);

/* Option type OT_COLOR functions. */
struct ft_color option_color_get(const struct option *poption);
struct ft_color option_color_def(const struct option *poption);
bool option_color_set(struct option *poption, struct ft_color color);

#define options_iterate(poptset, poption)                                   \
  {                                                                         \
    struct option *poption = optset_option_first(poptset);                  \
    for (; NULL != poption; poption = option_next(poption)) {

#define options_iterate_end                                                 \
  }                                                                         \
  }

/** Desired settable options. **/
void desired_settable_options_update(void);
void desired_settable_option_update(const char *op_name,
                                    const char *op_value,
                                    bool allow_replace);

/** Dialog report options. **/
void options_dialogs_update(void);
void options_dialogs_set(void);

/** Message Options: **/

/* for specifying which event messages go where: */
#define NUM_MW 3
#define MW_OUTPUT 1   /* add to the output window */
#define MW_MESSAGES 2 /* add to the messages window */
#define MW_POPUP 4    /* popup an individual window */

extern int messages_where[]; /* OR-ed MW_ values [E_COUNT] */

/** Client options **/

#define GUI_DEFAULT_CHAT_LOGFILE "freeciv-chat.log"
#define GUI_DEFAULT_MAPIMG_FILENAME "freeciv"

struct tileset;

const char *tileset_name_for_topology(int topology_id);
void option_set_default_ts(struct tileset *t);
void fill_topo_ts_default(void);


