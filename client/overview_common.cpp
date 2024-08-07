/*
Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
 */

#include <cmath> // floor

// utility
#include "log.h"

// client
#include "client_main.h" // can_client_change_view()
#include "climap.h"
#include "control.h"
#include "mapview_g.h"
#include "options.h"

#include "overview_common.h"

// Qt
#include <QPainter>
#include <QPixmap>
#include <qnamespace.h>

int OVERVIEW_TILE_SIZE = 2;

#if 0
struct overview overview = {
  // These are the default values.  All others are zeroed automatically.
  .fog = TRUE,
  .layers = {[OLAYER_BACKGROUND] = TRUE,
	     [OLAYER_UNITS] = TRUE,
	     [OLAYER_CITIES] = TRUE,
	     [OLAYER_BORDERS_ON_OCEAN] = TRUE}
};
#endif

namespace /* anonymous */ {

/*
 * Set to TRUE if the backing store is more recent than the version
 * drawn into overview.window.
 */
static bool overview_dirty = false;

// Tracks map updates for the overview
static std::unique_ptr<freeciv::map_updates_handler> updates = nullptr;

} // anonymous namespace

static void overview_update_tile(const tile *ptile);

/**
   Translate from gui to natural coordinate systems.  This provides natural
   coordinates as a floating-point value so there is no loss of information
   in the resulting values.
 */
void gui_to_natural_pos(const struct tileset *t, double *ntl_x,
                        double *ntl_y, int gui_x, int gui_y)
{
  const double gui_xd = gui_x, gui_yd = gui_y;
  const double W = tileset_tile_width(t);
  const double H = tileset_tile_height(t);
  double map_x, map_y;

  /* First convert to map positions.  This ignores hex conversions; we're
   * not looking for an exact tile. */
  if (tileset_is_isometric(t)) {
    // Includes hex cases.
    map_x = (gui_xd * H + gui_yd * W) / (W * H);
    map_y = (gui_yd * W - gui_xd * H) / (W * H);
  } else {
    map_x = gui_xd / W;
    map_y = gui_yd / H;
  }

  /* Now convert to natural positions.  Note this assumes the macro form
   * of the conversion will work with floating-point values. */
  MAP_TO_NATURAL_POS(ntl_x, ntl_y, map_x, map_y);
}

/**
   Translate from gui to overview coordinate systems.
 */
void gui_to_overview_pos(const struct tileset *t, int *ovr_x, int *ovr_y,
                         int gui_x, int gui_y)
{
  double ntl_x, ntl_y;

  gui_to_natural_pos(t, &ntl_x, &ntl_y, gui_x, gui_y);

  // Now convert straight to overview coordinates.
  *ovr_x =
      floor((ntl_x - gui_options->overview.map_x0) * OVERVIEW_TILE_SIZE);
  *ovr_y =
      floor((ntl_y - gui_options->overview.map_y0) * OVERVIEW_TILE_SIZE);

  // Now do additional adjustments.
  if (current_topo_has_flag(TF_WRAPX)) {
    *ovr_x = FC_WRAP(*ovr_x, NATURAL_WIDTH * OVERVIEW_TILE_SIZE);
  } else {
    if (MAP_IS_ISOMETRIC) {
      /* HACK: For iso-maps that don't wrap in the X direction we clip
       * a half-tile off of the left and right of the overview.  This
       * means some tiles only are halfway shown.  However it means we
       * don't show any unreal tiles, which we'd otherwise be doing.  The
       * rest of the code can't handle unreal tiles in the overview. */
      *ovr_x -= OVERVIEW_TILE_SIZE;
    }
  }
  if (current_topo_has_flag(TF_WRAPY)) {
    *ovr_y = FC_WRAP(*ovr_y, NATURAL_HEIGHT * OVERVIEW_TILE_SIZE);
  }
}

/**
   Return color for overview map tile.
 */
static QColor overview_tile_color(const tile *ptile)
{
  if (gui_options->overview.layers[OLAYER_CITIES]) {
    struct city *pcity = tile_city(ptile);

    if (pcity) {
      if (nullptr == client.conn.playing
          || city_owner(pcity) == client.conn.playing) {
        return get_color(tileset, COLOR_OVERVIEW_MY_CITY);
      } else if (pplayers_allied(city_owner(pcity), client.conn.playing)) {
        // Includes teams.
        return get_color(tileset, COLOR_OVERVIEW_ALLIED_CITY);
      } else {
        return get_color(tileset, COLOR_OVERVIEW_ENEMY_CITY);
      }
    }
  }
  if (gui_options->overview.layers[OLAYER_UNITS]) {
    struct unit *punit = find_visible_unit(ptile);

    if (punit) {
      if (nullptr == client.conn.playing
          || unit_owner(punit) == client.conn.playing) {
        return get_color(tileset, COLOR_OVERVIEW_MY_UNIT);
      } else if (pplayers_allied(unit_owner(punit), client.conn.playing)) {
        // Includes teams.
        return get_color(tileset, COLOR_OVERVIEW_ALLIED_UNIT);
      } else {
        return get_color(tileset, COLOR_OVERVIEW_ENEMY_UNIT);
      }
    }
  }
  if (gui_options->overview.layers[OLAYER_BORDERS]) {
    struct player *owner = tile_owner(ptile);

    if (owner) {
      if (gui_options->overview.layers[OLAYER_BORDERS_ON_OCEAN]) {
        return get_player_color(tileset, owner);
      } else if (!is_ocean_tile(ptile)) {
        return get_player_color(tileset, owner);
      }
    }
  }
  if (gui_options->overview.layers[OLAYER_RELIEF]
      && tile_terrain(ptile) != T_UNKNOWN) {
    return get_terrain_color(tileset, tile_terrain(ptile));
  }
  if (gui_options->overview.layers[OLAYER_BACKGROUND]
      && tile_terrain(ptile) != T_UNKNOWN) {
    if (terrain_has_flag(tile_terrain(ptile), TER_FROZEN)) {
      return get_color(tileset, COLOR_OVERVIEW_FROZEN);
    } else {
      if (is_ocean_tile(ptile)) {
        return get_color(tileset, COLOR_OVERVIEW_OCEAN);
      } else {
        return get_color(tileset, COLOR_OVERVIEW_LAND);
      }
    }
  }

  return get_color(tileset, COLOR_OVERVIEW_UNKNOWN);
}

/**
   Copies the overview image from the backing store to the window and
   draws the viewrect on top of it.
 */
static void redraw_overview()
{
  int i, x[4], y[4];

  if (!gui_options->overview.map) {
    return;
  }

  {
    QPixmap *src = gui_options->overview.map;
    QPixmap *dst = gui_options->overview.window;
    int x_left = gui_options->overview.map_x0 * OVERVIEW_TILE_SIZE;
    int y_top = gui_options->overview.map_y0 * OVERVIEW_TILE_SIZE;
    int ix = gui_options->overview.width - x_left;
    int iy = gui_options->overview.height - y_top;

    QPainter p(dst);
    p.drawPixmap(ix, iy, *src, 0, 0, x_left, y_top);
    p.drawPixmap(ix, 0, *src, 0, y_top, x_left, iy);
    p.drawPixmap(0, iy, *src, x_left, 0, ix, y_top);
    p.drawPixmap(0, 0, *src, x_left, y_top, ix, iy);
    p.end();
  }

  gui_to_overview_pos(tileset, &x[0], &y[0], mapview.gui_x0, mapview.gui_y0);
  gui_to_overview_pos(tileset, &x[1], &y[1], mapview.gui_x0 + mapview.width,
                      mapview.gui_y0);
  gui_to_overview_pos(tileset, &x[2], &y[2], mapview.gui_x0 + mapview.width,
                      mapview.gui_y0 + mapview.height);
  gui_to_overview_pos(tileset, &x[3], &y[3], mapview.gui_x0,
                      mapview.gui_y0 + mapview.height);

  QPainter p(gui_options->overview.window);
  p.setPen(QPen(get_color(tileset, COLOR_OVERVIEW_VIEWRECT), 2));
  for (i = 0; i < 4; i++) {
    int src_x = x[i];
    int src_y = y[i];
    int dst_x = x[(i + 1) % 4];
    int dst_y = y[(i + 1) % 4];

    p.drawLine(src_x, src_y, dst_x, dst_y);
  }
  p.end();

  update_minimap();

  overview_dirty = false;
}

/**
   Mark the overview as "dirty" so that it will be redrawn soon.
 */
static void dirty_overview() { overview_dirty = true; }

/**
   Redraw the overview if it is "dirty".
 */
void flush_dirty_overview()
{
  /* Currently this function is called from mapview_common.  However it
   * should be made static eventually. */
  if (overview_dirty) {
    redraw_overview();
  }
}

/**
   Finds the map coordinates for a given overview (canvas) position.
 */
void overview_to_map_pos(int *map_x, int *map_y, int overview_x,
                         int overview_y)
{
  int ntl_x = overview_x / OVERVIEW_TILE_SIZE + gui_options->overview.map_x0;
  int ntl_y = overview_y / OVERVIEW_TILE_SIZE + gui_options->overview.map_y0;

  if (MAP_IS_ISOMETRIC && !current_topo_has_flag(TF_WRAPX)) {
    // Clip half tile left and right.
    // See comment in calculate_overview_dimensions().
    ntl_x++;
  }

  NATURAL_TO_MAP_POS(map_x, map_y, ntl_x, ntl_y);
  // All positions on the overview should be valid.
  fc_assert(normalize_map_pos(&(wld.map), map_x, map_y));
}

/**
   Redraw the entire backing store for the overview minimap.
 */
void refresh_overview_canvas()
{
  if (!can_client_change_view()) {
    return;
  }
  whole_map_iterate(&(wld.map), ptile) { overview_update_tile(ptile); }
  whole_map_iterate_end;
  redraw_overview();
}

/**
   Draws the color for this tile onto the given rectangle of the canvas.

   This is just a simple helper function for overview_update_tile, since
   sometimes a tile may cover more than one rectangle.
 */
static void put_overview_tile_area(QPixmap *pcanvas, const tile *ptile,
                                   int x, int y, int w, int h)
{
  QPainter p(pcanvas);
  p.fillRect(x, y, w, h, overview_tile_color(ptile));
  if (gui_options->overview.fog
      && TILE_KNOWN_UNSEEN == client_tile_get_known(ptile)) {
    p.fillRect(x, y, w, h, QColor(0, 0, 0, 128));
  }
  p.end();
}

/**
   Redraw the given map position in the overview canvas.
 */
static void overview_update_tile(const tile *ptile)
{
  if (!can_client_change_view()) {
    return;
  }

  int tile_x, tile_y;

  /* Base overview positions are just like natural positions, but scaled to
   * the overview tile dimensions. */
  index_to_map_pos(&tile_x, &tile_y, tile_index(ptile));
  do_in_natural_pos(ntl_x, ntl_y, tile_x, tile_y)
  {
    int overview_y = ntl_y * OVERVIEW_TILE_SIZE;
    int overview_x = ntl_x * OVERVIEW_TILE_SIZE;

    if (MAP_IS_ISOMETRIC) {
      if (current_topo_has_flag(TF_WRAPX)) {
        if (overview_x > gui_options->overview.width - OVERVIEW_TILE_WIDTH) {
          /* This tile is shown half on the left and half on the right
           * side of the overview.  So we have to draw it in two parts. */
          put_overview_tile_area(gui_options->overview.map, ptile,
                                 overview_x - gui_options->overview.width,
                                 overview_y, OVERVIEW_TILE_WIDTH,
                                 OVERVIEW_TILE_HEIGHT);
        }
      } else {
        /* Clip half tile left and right.
         * See comment in calculate_overview_dimensions(). */
        overview_x -= OVERVIEW_TILE_SIZE;
      }
    }

    put_overview_tile_area(gui_options->overview.map, ptile, overview_x,
                           overview_y, OVERVIEW_TILE_WIDTH,
                           OVERVIEW_TILE_HEIGHT);

    dirty_overview();
  }
  do_in_natural_pos_end;
}

/**
   Called if the map size is know or changes.
 */
void calculate_overview_dimensions()
{
  int w, h;
  int xfact = MAP_IS_ISOMETRIC ? 2 : 1;

  static int recursion = 0; // Just to be safe.

  // Clip half tile left and right.
  /* HACK: For iso-maps that don't wrap in the X direction we clip
   * a half-tile off of the left and right of the overview.  This
   * means some tiles only are halfway shown.  However it means we
   * don't show any unreal tiles, which we'd otherwise be doing.  The
   * rest of the code can't handle unreal tiles in the overview. */
  int shift =
      (MAP_IS_ISOMETRIC && !current_topo_has_flag(TF_WRAPX)) ? -1 : 0;

  if (recursion > 0 || wld.map.xsize <= 0 || wld.map.ysize <= 0) {
    return;
  }
  recursion++;
  w = 0;
  h = 0;
  /* Set the scale of the overview map.  This attempts to limit the
   * overview to the size of the area available.
   *
   * It rounds up since this gives good results with the default settings.
   * It may need tweaking if the panel resizes itself. */
  OVERVIEW_TILE_SIZE = MIN((w - 1) / (wld.map.xsize * xfact) + 1,
                           (h - 1) / wld.map.ysize + 1);
  OVERVIEW_TILE_SIZE = MAX(OVERVIEW_TILE_SIZE, 1);

  log_debug("Map size %d,%d - area size %d,%d - scale: %d", wld.map.xsize,
            wld.map.ysize, w, h, OVERVIEW_TILE_SIZE);

  gui_options->overview.width =
      OVERVIEW_TILE_WIDTH * wld.map.xsize + shift * OVERVIEW_TILE_SIZE;
  gui_options->overview.height = OVERVIEW_TILE_HEIGHT * wld.map.ysize;

  if (gui_options->overview.map) {
    delete gui_options->overview.map;
    delete gui_options->overview.window;
  }
  gui_options->overview.map =
      new QPixmap(gui_options->overview.width, gui_options->overview.height);
  gui_options->overview.window =
      new QPixmap(gui_options->overview.width, gui_options->overview.height);
  gui_options->overview.map->fill(
      get_color(tileset, COLOR_OVERVIEW_UNKNOWN));

  update_minimap();
  if (can_client_change_view()) {
    refresh_overview_canvas();
  }

  recursion--;
}

namespace /* anonymous */ {
/**
 * Unqueues pending updates of the overview.
 */
void unqueue_overview_updates()
{
  if (updates->full()) {
    refresh_overview_canvas();
  } else {
    for (const auto [tile, _] : updates->list()) {
      overview_update_tile(tile);
    }
  }
  updates->clear();
}
} // anonymous namespace

/**
 * Allocates overview resources.
 */
void overview_init()
{
  updates = std::make_unique<freeciv::map_updates_handler>();
  updates->connect(
      updates.get(), &freeciv::map_updates_handler::repaint_needed,
      updates.get(), unqueue_overview_updates, Qt::QueuedConnection);
}

/**
   Free overview resources.
 */
void overview_free()
{
  if (gui_options->overview.map) {
    delete gui_options->overview.map;
    delete gui_options->overview.window;
    gui_options->overview.map = nullptr;
    gui_options->overview.window = nullptr;
  }
  updates = nullptr;
}

/**
   Callback to be called when an overview option is changed.
 */
void overview_redraw_callback(struct option *option)
{
  Q_UNUSED(option)
  /* This is called once for each option changed so it is slower than
   * necessary.  If this becomes a problem it could be switched to use a
   * queue system like the mapview drawing code does. */
  refresh_overview_canvas();
}
