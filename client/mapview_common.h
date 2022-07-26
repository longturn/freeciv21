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

// common
#include "featured_text.h"
#include "map.h"
// include
#include "colors_g.h"

#include "map_updates_handler.h"
#include "tilespec.h"

struct view {
  float gui_x0, gui_y0;
  int width, height; // Size in pixels.
  int store_width, store_height;
  bool can_do_cached_drawing; // TRUE if cached drawing is possible.
  QPixmap *store, *tmp_store;
  std::unique_ptr<freeciv::map_updates_handler> updates;
};

void mapdeco_init();
void mapdeco_free();
bool mapdeco_is_highlight_set(const struct tile *ptile);
void mapdeco_set_crosshair(const struct tile *ptile, bool crosshair);
bool mapdeco_is_crosshair_set(const struct tile *ptile);
void mapdeco_clear_crosshairs();
void mapdeco_set_gotoroute(const struct unit *punit);
void mapdeco_add_gotoline(const struct tile *ptile, enum direction8 dir,
                          bool safe);
bool mapdeco_is_gotoline_set(const struct tile *ptile, enum direction8 dir,
                             bool *safe);
void mapdeco_clear_gotoroutes();

extern struct view mapview;

/* HACK: Callers can set this to FALSE to disable sliding.  It should be
 * reenabled afterwards. */
extern bool can_slide;

#define GOTO_WIDTH 2

void refresh_tile_mapcanvas(const tile *ptile, bool full_refresh);
void refresh_unit_mapcanvas(struct unit *punit, struct tile *ptile,
                            bool full_refresh);
void refresh_city_mapcanvas(struct city *pcity, struct tile *ptile,
                            bool full_refresh);

void unqueue_mapview_updates(bool write_to_screen);
void map_to_gui_vector(const struct tileset *t, float *gui_dx, float *gui_dy,
                       int map_dx, int map_dy);
bool tile_to_canvas_pos(float *canvas_x, float *canvas_y, const tile *ptile);
struct tile *canvas_pos_to_tile(float canvas_x, float canvas_y);
struct tile *canvas_pos_to_nearest_tile(float canvas_x, float canvas_y);

void get_mapview_scroll_window(float *xmin, float *ymin, float *xmax,
                               float *ymax, int *xsize, int *ysize);
void get_mapview_scroll_pos(int *scroll_x, int *scroll_y);

void set_mapview_origin(float gui_x0, float gui_y0);
struct tile *get_center_tile_mapcanvas();
void center_tile_mapcanvas(struct tile *ptile);

bool tile_visible_mapcanvas(struct tile *ptile);
bool tile_visible_and_not_on_border_mapcanvas(struct tile *ptile);

void put_unit(const struct unit *punit, QPixmap *pcanvas, int canvas_x,
              int canvas_y);
void put_terrain(struct tile *ptile, QPixmap *pcanvas, int canvas_x,
                 int canvas_y);

void put_unit_city_overlays(const unit *punit, QPixmap *pcanvas,
                            int canvas_x, int canvas_y,
                            const int *upkeep_cost, int happy_cost);
void toggle_city_color(struct city *pcity);
void toggle_unit_color(struct unit *punit);

void put_nuke_mushroom_pixmaps(struct tile *ptile);

void put_drawn_sprites(QPixmap *pcanvas, int canvas_x, int canvas_y,
                       const std::vector<drawn_sprite> &sprites, bool fog,
                       bool citydialog = false, bool city_unit = false);

void update_map_canvas(int canvas_x, int canvas_y, int width, int height);
void update_map_canvas_visible();
void update_city_description(struct city *pcity);
void update_tile_label(struct tile *ptile);

void show_city_descriptions(int canvas_base_x, int canvas_base_y,
                            int width_base, int height_base);
void show_tile_labels(int canvas_base_x, int canvas_base_y, int width_base,
                      int height_base);

void draw_segment(const tile *ptile, enum direction8 dir, bool safe);

void decrease_unit_hp_smooth(struct unit *punit0, int hp0,
                             struct unit *punit1, int hp1);
void move_unit_map_canvas(struct unit *punit, struct tile *ptile, int dx,
                          int dy);

struct city *find_city_or_settler_near_tile(const struct tile *ptile,
                                            struct unit **punit);

void get_city_mapview_production(const city *pcity, char *buf,
                                 size_t buf_len);
void get_city_mapview_name_and_growth(const city *pcity, char *name_buffer,
                                      size_t name_buffer_len,
                                      char *growth_buffer,
                                      size_t growth_buffer_len,
                                      enum color_std *growth_color,
                                      enum color_std *production_color);
void get_city_mapview_trade_routes(const city *pcity,
                                   char *trade_routes_buffer,
                                   size_t trade_routes_buffer_len,
                                   enum color_std *trade_routes_color);

void map_canvas_resized(int width, int height);
void init_mapcanvas_and_overview();
void free_mapcanvas_and_overview();

void link_marks_init();
void link_marks_free();

void link_marks_draw_all();
void link_marks_clear_all();
void link_marks_decrease_turn_counters();

void link_mark_add_new(enum text_link_type type, int id);
void link_mark_restore(enum text_link_type type, int id);

enum topo_comp_lvl {
  TOPO_COMPATIBLE = 0,
  TOPO_INCOMP_SOFT = 1,
  TOPO_INCOMP_HARD = 2
};

enum topo_comp_lvl tileset_map_topo_compatible(int topology_id,
                                               struct tileset *tset);
