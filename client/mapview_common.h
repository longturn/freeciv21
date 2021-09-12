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

#include "tilespec.h"

struct canvas_store; // opaque type, real type is gui-dep

struct view {
  float gui_x0, gui_y0;
  int width, height;           // Size in pixels.
  int tile_width, tile_height; // Size in tiles. Rounded up.
  int store_width, store_height;
  bool can_do_cached_drawing; // TRUE if cached drawing is possible.
  QPixmap *store, *tmp_store;
};

void mapdeco_init();
void mapdeco_free();
void mapdeco_set_highlight(const struct tile *ptile, bool highlight);
bool mapdeco_is_highlight_set(const struct tile *ptile);
void mapdeco_clear_highlights();
void mapdeco_set_crosshair(const struct tile *ptile, bool crosshair);
bool mapdeco_is_crosshair_set(const struct tile *ptile);
void mapdeco_clear_crosshairs();
void mapdeco_set_gotoroute(const struct unit *punit);
void mapdeco_add_gotoline(const struct tile *ptile, enum direction8 dir);
void mapdeco_remove_gotoline(const struct tile *ptile, enum direction8 dir);
bool mapdeco_is_gotoline_set(const struct tile *ptile, enum direction8 dir);
void mapdeco_clear_gotoroutes();

extern struct view mapview;

/* HACK: Callers can set this to FALSE to disable sliding.  It should be
 * reenabled afterwards. */
extern bool can_slide;

#define BORDER_WIDTH 2
#define GOTO_WIDTH 2

/*
 * Iterate over all map tiles that intersect with the given GUI rectangle.
 * The order of iteration is guaranteed to satisfy the painter's algorithm.
 * The iteration covers not only tiles but tile edges and corners.
 *
 * GRI_x0, GRI_y0: gives the GUI origin of the rectangle.
 *
 * GRI_width, GRI_height: gives the GUI width and height of the rectangle.
 * These values may be negative.
 *
 * _t, _e, _c: the tile, edge, or corner that is being iterated, declared
 * inside the macro.  Usually, only one of them will be non-NULL at a time.
 * These values may be passed directly to fill_sprite_array().
 *
 * _x, _y: the canvas position of the current element, declared inside
 * the macro.  Each element is assumed to be tileset_tile_width(tileset) *
 * tileset_tile_height(tileset) in size.  If an element is larger, the
 * caller needs to use a larger rectangle of iteration.
 *
 * The grid of iteration is rather complicated.  For a picture of it see
 * http://article.gmane.org/gmane.games.freeciv.devel/50449
 * (formerly newgrid.png in PR#12085).
 */
#define gui_rect_iterate(GRI_x0, GRI_y0, GRI_width, GRI_height, _t, _e, _c) \
  {                                                                         \
    int _x_##_0 = (GRI_x0), _y_##_0 = (GRI_y0);                             \
    int _x_##_w = (GRI_width), _y_##_h = (GRI_height);                      \
                                                                            \
    if (_x_##_w < 0) {                                                      \
      _x_##_0 += _x_##_w;                                                   \
      _x_##_w = -_x_##_w;                                                   \
    }                                                                       \
    if (_y_##_h < 0) {                                                      \
      _y_##_0 += _y_##_h;                                                   \
      _y_##_h = -_y_##_h;                                                   \
    }                                                                       \
    if (_x_##_w > 0 && _y_##_h > 0) {                                       \
      struct tile_edge _t##_e;                                              \
      struct tile_corner _t##_c;                                            \
      int _t##_xi, _t##_yi, _t##_si, _t##_di;                               \
      const int _t##_r1 = (tileset_is_isometric(tileset) ? 2 : 1);          \
      const int _t##_r2 = _t##_r1 * 2; /* double the ratio */               \
      const int _t##_w = tileset_tile_width(tileset);                       \
      const int _t##_h = tileset_tile_height(tileset);                      \
      /* Don't divide by _r2 yet, to avoid integer rounding errors. */      \
      const int _t##_x0 = DIVIDE(_x_##_0 * _t##_r2, _t##_w) - _t##_r1 / 2;  \
      const int _t##_y0 = DIVIDE(_y_##_0 * _t##_r2, _t##_h) - _t##_r1 / 2;  \
      const int _t##_x1 =                                                   \
          DIVIDE((_x_##_0 + _x_##_w) * _t##_r2 + _t##_w - 1, _t##_w)        \
          + _t##_r1;                                                        \
      const int _t##_y1 =                                                   \
          DIVIDE((_y_##_0 + _y_##_h) * _t##_r2 + _t##_h - 1, _t##_h)        \
          + _t##_r1;                                                        \
      const int _t##_count = (_t##_x1 - _t##_x0) * (_t##_y1 - _t##_y0);     \
      int _t##_index = 0;                                                   \
                                                                            \
      log_debug("Iterating over %d-%d x %d-%d rectangle.", _t##_x1,         \
                _t##_x0, _t##_y1, _t##_y0);                                 \
      for (; _t##_index < _t##_count; _t##_index++) {                       \
        struct tile *_t = NULL;                                             \
        struct tile_edge *_e = NULL;                                        \
        struct tile_corner *_c = NULL;                                      \
                                                                            \
        _t##_xi = _t##_x0 + (_t##_index % (_t##_x1 - _t##_x0));             \
        _t##_yi = _t##_y0 + (_t##_index / (_t##_x1 - _t##_x0));             \
        _t##_si = _t##_xi + _t##_yi;                                        \
        _t##_di = _t##_yi - _t##_xi;                                        \
        if (2 == _t##_r1 /*tileset_is_isometric(tileset)*/) {               \
          if ((_t##_xi + _t##_yi) % 2 != 0) {                               \
            continue;                                                       \
          }                                                                 \
          if (_t##_xi % 2 == 0 && _t##_yi % 2 == 0) {                       \
            if ((_t##_xi + _t##_yi) % 4 == 0) {                             \
              /* Tile */                                                    \
              _t = map_pos_to_tile(&(wld.map), _t##_si / 4 - 1,             \
                                   _t##_di / 4);                            \
            } else {                                                        \
              /* Corner */                                                  \
              _c = &_t##_c;                                                 \
              _c->tile[0] = map_pos_to_tile(&(wld.map), (_t##_si - 6) / 4,  \
                                            (_t##_di - 2) / 4);             \
              _c->tile[1] = map_pos_to_tile(&(wld.map), (_t##_si - 2) / 4,  \
                                            (_t##_di - 2) / 4);             \
              _c->tile[2] = map_pos_to_tile(&(wld.map), (_t##_si - 2) / 4,  \
                                            (_t##_di + 2) / 4);             \
              _c->tile[3] = map_pos_to_tile(&(wld.map), (_t##_si - 6) / 4,  \
                                            (_t##_di + 2) / 4);             \
              if (tileset_hex_width(tileset) > 0) {                         \
                _e = &_t##_e;                                               \
                _e->type = EDGE_UD;                                         \
                _e->tile[0] = _c->tile[0];                                  \
                _e->tile[1] = _c->tile[2];                                  \
              } else if (tileset_hex_height(tileset) > 0) {                 \
                _e = &_t##_e;                                               \
                _e->type = EDGE_LR;                                         \
                _e->tile[0] = _c->tile[1];                                  \
                _e->tile[1] = _c->tile[3];                                  \
              }                                                             \
            }                                                               \
          } else {                                                          \
            /* Edge. */                                                     \
            _e = &_t##_e;                                                   \
            if (_t##_si % 4 == 0) {                                         \
              _e->type = EDGE_NS;                                           \
              _e->tile[0] = map_pos_to_tile(&(wld.map), (_t##_si - 4) / 4,  \
                                            (_t##_di - 2) / 4); /*N*/       \
              _e->tile[1] = map_pos_to_tile(&(wld.map), (_t##_si - 4) / 4,  \
                                            (_t##_di + 2) / 4); /*S*/       \
            } else {                                                        \
              _e->type = EDGE_WE;                                           \
              _e->tile[0] = map_pos_to_tile(&(wld.map), (_t##_si - 6) / 4,  \
                                            _t##_di / 4); /*W*/             \
              _e->tile[1] = map_pos_to_tile(&(wld.map), (_t##_si - 2) / 4,  \
                                            _t##_di / 4); /*E*/             \
            }                                                               \
          }                                                                 \
        } else {                                                            \
          if (_t##_si % 2 == 0) {                                           \
            if (_t##_xi % 2 == 0) {                                         \
              /* Corner. */                                                 \
              _c = &_t##_c;                                                 \
              _c->tile[0] = map_pos_to_tile(&(wld.map), _t##_xi / 2 - 1,    \
                                            _t##_yi / 2 - 1); /*NW*/        \
              _c->tile[1] = map_pos_to_tile(&(wld.map), _t##_xi / 2,        \
                                            _t##_yi / 2 - 1); /*NE*/        \
              _c->tile[2] = map_pos_to_tile(&(wld.map), _t##_xi / 2,        \
                                            _t##_yi / 2); /*SE*/            \
              _c->tile[3] = map_pos_to_tile(&(wld.map), _t##_xi / 2 - 1,    \
                                            _t##_yi / 2); /*SW*/            \
            } else {                                                        \
              /* Tile. */                                                   \
              _t = map_pos_to_tile(&(wld.map), (_t##_xi - 1) / 2,           \
                                   (_t##_yi - 1) / 2);                      \
            }                                                               \
          } else {                                                          \
            /* Edge. */                                                     \
            _e = &_t##_e;                                                   \
            if (_t##_yi % 2 == 0) {                                         \
              _e->type = EDGE_NS;                                           \
              _e->tile[0] = map_pos_to_tile(&(wld.map), (_t##_xi - 1) / 2,  \
                                            _t##_yi / 2 - 1); /*N*/         \
              _e->tile[1] = map_pos_to_tile(&(wld.map), (_t##_xi - 1) / 2,  \
                                            _t##_yi / 2); /*S*/             \
            } else {                                                        \
              _e->type = EDGE_WE;                                           \
              _e->tile[0] = map_pos_to_tile(&(wld.map), _t##_xi / 2 - 1,    \
                                            (_t##_yi - 1) / 2); /*W*/       \
              _e->tile[1] = map_pos_to_tile(&(wld.map), _t##_xi / 2,        \
                                            (_t##_yi - 1) / 2); /*E*/       \
            }                                                               \
          }                                                                 \
        }

#define gui_rect_iterate_end                                                \
  }                                                                         \
  }                                                                         \
  }

#define gui_rect_iterate_coord(GRI_x0, GRI_y0, GRI_width, GRI_height, _t,   \
                               _e, _c, _x, _y)                              \
  gui_rect_iterate(GRI_x0, GRI_y0, GRI_width, GRI_height, _t, _e, _c)       \
  {                                                                         \
    int _x, _y;                                                             \
                                                                            \
    _x = _t##_xi * _t##_w / _t##_r2 - _t##_w / 2;                           \
    _y = _t##_yi * _t##_h / _t##_r2 - _t##_h / 2;

#define gui_rect_iterate_coord_end                                          \
  }                                                                         \
  gui_rect_iterate_end

void refresh_tile_mapcanvas(struct tile *ptile, bool full_refresh,
                            bool write_to_screen);
void refresh_unit_mapcanvas(struct unit *punit, struct tile *ptile,
                            bool full_refresh, bool write_to_screen);
void refresh_city_mapcanvas(struct city *pcity, struct tile *ptile,
                            bool full_refresh, bool write_to_screen);

void unqueue_mapview_updates(bool write_to_screen);
void redraw_visible_map_now();
void map_to_gui_vector(const struct tileset *t, float *gui_dx, float *gui_dy,
                       int map_dx, int map_dy);
bool tile_to_canvas_pos(float *canvas_x, float *canvas_y,
                        struct tile *ptile);
struct tile *canvas_pos_to_tile(float canvas_x, float canvas_y);
struct tile *canvas_pos_to_nearest_tile(float canvas_x, float canvas_y);

void get_mapview_scroll_window(float *xmin, float *ymin, float *xmax,
                               float *ymax, int *xsize, int *ysize);
void get_mapview_scroll_step(int *xstep, int *ystep);
void get_mapview_scroll_pos(int *scroll_x, int *scroll_y);
void set_mapview_scroll_pos(int scroll_x, int scroll_y);

void set_mapview_origin(float gui_x0, float gui_y0);
struct tile *get_center_tile_mapcanvas();
void center_tile_mapcanvas(struct tile *ptile);

bool tile_visible_mapcanvas(struct tile *ptile);
bool tile_visible_and_not_on_border_mapcanvas(struct tile *ptile);

void put_unit(const struct unit *punit, QPixmap *pcanvas, int canvas_x,
              int canvas_y);
void put_unittype(const struct unit_type *putype, QPixmap *pcanvas,
                  int canvas_x, int canvas_y);
void put_city(struct city *pcity, QPixmap *pcanvas, int canvas_x,
              int canvas_y);
void put_terrain(struct tile *ptile, QPixmap *pcanvas, int canvas_x,
                 int canvas_y);

void put_unit_city_overlays(struct unit *punit, QPixmap *pcanvas,
                            int canvas_x, int canvas_y, int *upkeep_cost,
                            int happy_cost);
void toggle_city_color(struct city *pcity);
void toggle_unit_color(struct unit *punit);

void put_nuke_mushroom_pixmaps(struct tile *ptile);

void put_one_element(QPixmap *pcanvas, enum mapview_layer layer,
                     const struct tile *ptile, const struct tile_edge *pedge,
                     const struct tile_corner *pcorner,
                     const struct unit *punit, const struct city *pcity,
                     int canvas_x, int canvas_y,
                     const struct unit_type *putype);

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
bool show_unit_orders(struct unit *punit);

void draw_segment(struct tile *ptile, enum direction8 dir);

void decrease_unit_hp_smooth(struct unit *punit0, int hp0,
                             struct unit *punit1, int hp1);
void move_unit_map_canvas(struct unit *punit, struct tile *ptile, int dx,
                          int dy);

struct city *find_city_or_settler_near_tile(const struct tile *ptile,
                                            struct unit **punit);
struct city *find_city_near_tile(const struct tile *ptile);

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

bool map_canvas_resized(int width, int height);
void init_mapcanvas_and_overview();
void free_mapcanvas_and_overview();

void get_spaceship_dimensions(int *width, int *height);
void put_spaceship(QPixmap *pcanvas, int canvas_x, int canvas_y,
                   const struct player *pplayer);

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
