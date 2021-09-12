/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdlib> // qsort

// utility
#include "fcintl.h"
#include "log.h"
#include "support.h"

// common
#include "combat.h"
#include "game.h"
#include "unitlist.h"

// client
#include "chatline_common.h"
#include "cityrep_g.h"
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "editor.h"
#include "fcintl.h"
#include "goto.h"
#include "governor.h"
#include "mapctrl_common.h"
#include "mapctrl_g.h"
#include "mapview_g.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"

// Selection Rectangle
static float rec_anchor_x, rec_anchor_y; // canvas coordinates for anchor
static struct tile *rec_canvas_center_tile;
static int rec_corner_x, rec_corner_y; // corner to iterate from
static int rec_w, rec_h;               // width, heigth in pixels

bool rbutton_down = false;
bool rectangle_active = false;

/* This changes the behaviour of left mouse
   button in Area Selection mode. */
bool tiles_hilited_cities = false;

// The mapcanvas clipboard
struct universal clipboard = {.value = {.building = NULL}, .kind = VUT_NONE};

// Goto with drag and drop.
bool keyboardless_goto_button_down = false;
bool keyboardless_goto_active = false;
struct tile *keyboardless_goto_start_tile;

// Update the workers for a city on the map, when the update is received
struct city *city_workers_display = NULL;

/*************************************************************************/

static void clipboard_send_production_packet(struct city *pcity);
static void define_tiles_within_rectangle(bool append);

/**
   Called when Right Mouse Button is depressed. Record the canvas
   coordinates of the center of the tile, which may be unreal. This
   anchor is not the drawing start point, but is used to calculate
   width, height. Also record the current mapview centering.
 */
void anchor_selection_rectangle(int canvas_x, int canvas_y)
{
  struct tile *ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

  tile_to_canvas_pos(&rec_anchor_x, &rec_anchor_y, ptile);
  rec_anchor_x += tileset_tile_width(tileset) / 2;
  rec_anchor_y += tileset_tile_height(tileset) / 2;
  // FIXME: This may be off-by-one.
  rec_canvas_center_tile = get_center_tile_mapcanvas();
  rec_w = rec_h = 0;
}

/**
   Iterate over the pixel boundaries of the rectangle and pick the tiles
   whose center falls within. Axis pixel incrementation is half tile size to
   accomodate tilesets with varying tile shapes and proportions of X/Y.

   These operations are performed on the tiles:
   -  Make tiles that contain owned cities hilited
      on the map and hilited in the City List Window.

   Later, I'll want to add unit hiliting for mass orders.       -ali

   NB: At the end of this function the current selection rectangle will be
   erased (by being redrawn).
 */
static void define_tiles_within_rectangle(bool append)
{
  const int W = tileset_tile_width(tileset), half_W = W / 2;
  const int H = tileset_tile_height(tileset), half_H = H / 2;
  const int segments_x = abs(rec_w / half_W);
  const int segments_y = abs(rec_h / half_H);

  // Iteration direction
  const int inc_x = (rec_w > 0 ? half_W : -half_W);
  const int inc_y = (rec_h > 0 ? half_H : -half_H);
  int x, y, xx, yy;
  float x2, y2;
  struct unit_list *units = unit_list_new();
  const struct city *pcity;
  bool found_any_cities = false;

  y = rec_corner_y;
  for (yy = 0; yy <= segments_y; yy++, y += inc_y) {
    x = rec_corner_x;
    for (xx = 0; xx <= segments_x; xx++, x += inc_x) {
      struct tile *ptile;

      /*  For diamond shaped tiles, every other row is indented.
       */
      if ((yy % 2 ^ xx % 2) != 0) {
        continue;
      }

      ptile = canvas_pos_to_tile(x, y);
      if (!ptile) {
        continue;
      }

      /*  "Half-tile" indentation must match, or we'll process
       *  some tiles twice in the case of rectangular shape tiles.
       */
      tile_to_canvas_pos(&x2, &y2, ptile);

      if ((yy % 2) != 0
          && ((rec_corner_x % W) ^ abs(static_cast<int>(x2) % W)) != 0) {
        continue;
      }

      /*  Tile passed all tests; process it.
       */
      pcity = tile_city(ptile);
      if (pcity != NULL && city_owner(pcity) == client_player()) {
        mapdeco_set_highlight(ptile, true);
        found_any_cities = tiles_hilited_cities = true;
      }
      unit_list_iterate(ptile->units, punit)
      {
        if (unit_owner(punit) == client.conn.playing) {
          unit_list_append(units, punit);
        }
      }
      unit_list_iterate_end;
    }
  }

  if (!(gui_options.separate_unit_selection && found_any_cities)
      && unit_list_size(units) > 0) {
    if (!append) {
      struct unit *punit = unit_list_get(units, 0);

      unit_focus_set(punit);
      unit_list_remove(units, punit);
    }
    unit_list_iterate(units, punit) { unit_focus_add(punit); }
    unit_list_iterate_end;
  }
  unit_list_destroy(units);

  // Clear previous rectangle.
  draw_selection_rectangle(rec_corner_x, rec_corner_y, rec_w, rec_h);
}

/**
   Called when mouse pointer moves and rectangle is active.
 */
void update_selection_rectangle(float canvas_x, float canvas_y)
{
  const int W = tileset_tile_width(tileset), half_W = W / 2;
  const int H = tileset_tile_height(tileset), half_H = H / 2;
  static struct tile *rec_tile = NULL;
  int diff_x, diff_y;
  struct tile *center_tile;
  struct tile *ptile;

  ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

  /*  Did mouse pointer move beyond the current tile's
   *  boundaries? Avoid macros; tile may be unreal!
   */
  if (ptile == rec_tile) {
    return;
  }
  rec_tile = ptile;

  // Clear previous rectangle.
  draw_selection_rectangle(rec_corner_x, rec_corner_y, rec_w, rec_h);

  /*  Fix canvas coords to the center of the tile.
   */
  tile_to_canvas_pos(&canvas_x, &canvas_y, ptile);
  canvas_x += half_W;
  canvas_y += half_H;

  rec_w = rec_anchor_x - canvas_x; // width
  rec_h = rec_anchor_y - canvas_y; // height

  // FIXME: This may be off-by-one.
  center_tile = get_center_tile_mapcanvas();
  map_distance_vector(&diff_x, &diff_y, center_tile, rec_canvas_center_tile);

  /*  Adjust width, height if mapview has recentered.
   */
  if (diff_x != 0 || diff_y != 0) {
    if (tileset_is_isometric(tileset)) {
      rec_w += (diff_x - diff_y) * half_W;
      rec_h += (diff_x + diff_y) * half_H;

      // Iso wrapping
      if (abs(rec_w) > wld.map.xsize * half_W / 2) {
        int wx = wld.map.xsize * half_W, wy = wld.map.xsize * half_H;

        rec_w > 0 ? (rec_w -= wx, rec_h -= wy) : (rec_w += wx, rec_h += wy);
      }

    } else {
      rec_w += diff_x * W;
      rec_h += diff_y * H;

      // X wrapping
      if (abs(rec_w) > wld.map.xsize * half_W) {
        int wx = wld.map.xsize * W;

        rec_w > 0 ? (rec_w -= wx) : (rec_w += wx);
      }
    }
  }

  if (rec_w == 0 && rec_h == 0) {
    rectangle_active = false;
    return;
  }

  // It is currently drawn only to the screen, not backing store
  rectangle_active = true;
  draw_selection_rectangle(canvas_x, canvas_y, rec_w, rec_h);
  rec_corner_x = canvas_x;
  rec_corner_y = canvas_y;
}

/**
   Redraws the selection rectangle after a map flush.
 */
void redraw_selection_rectangle()
{
  if (rectangle_active) {
    draw_selection_rectangle(rec_corner_x, rec_corner_y, rec_w, rec_h);
  }
}

/**
   Redraws the selection rectangle after a map flush.
 */
void cancel_selection_rectangle()
{
  if (rectangle_active) {
    rectangle_active = false;
    rbutton_down = false;

    // Erase the previously drawn selection rectangle.
    draw_selection_rectangle(rec_corner_x, rec_corner_y, rec_w, rec_h);
  }
}

/**
   Action depends on whether the mouse pointer moved
   a tile between press and release.
 */
void release_right_button(int canvas_x, int canvas_y, bool shift)
{
  if (rectangle_active) {
    define_tiles_within_rectangle(shift);
  } else {
    recenter_button_pressed(canvas_x, canvas_y);
  }
  rectangle_active = false;
  rbutton_down = false;
}

/**
   The user pressed the overlay-city button (t) while the mouse was at the
   given canvas position.
 */
void key_city_overlay(int canvas_x, int canvas_y)
{
  struct tile *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    struct unit *punit;
    struct city *pcity = find_city_or_settler_near_tile(ptile, &punit);

    if (pcity) {
      toggle_city_color(pcity);
    } else if (punit) {
      toggle_unit_color(punit);
    }
  }
}

void key_city_show_open(struct city *pcity)
{
  if (can_client_change_view() && pcity) {
    if (pcity) {
      pcity->client.city_opened = true;
      refresh_city_mapcanvas(pcity, pcity->tile, true, false);
    }
  }
}

void key_city_hide_open(struct city *pcity)
{
  if (can_client_change_view() && pcity) {
    if (pcity) {
      pcity->client.city_opened = false;
      refresh_city_mapcanvas(pcity, pcity->tile, true, false);
    }
  }
}

/**
   Shift-Left-Click on owned city or any visible unit to copy.
   Returns whether it found anything to try to copy.
 */
bool clipboard_copy_production(struct tile *ptile)
{
  char buffer[256];
  struct city *pcity = tile_city(ptile);

  if (!can_client_issue_orders()) {
    return false;
  }

  if (pcity) {
    if (city_owner(pcity) != client.conn.playing) {
      return false;
    }
    clipboard = pcity->production;
  } else {
    struct unit *punit = find_visible_unit(ptile);
    if (!punit) {
      return false;
    }
    if (!can_player_build_unit_direct(client.conn.playing,
                                      unit_type_get(punit))) {
      create_event(ptile, E_BAD_COMMAND, ftc_client,
                   _("You don't know how to build %s!"),
                   unit_name_translation(punit));
      return true;
    }
    clipboard.kind = VUT_UTYPE;
    clipboard.value.utype = unit_type_get(punit);
  }
  upgrade_canvas_clipboard();

  create_event(
      ptile, E_CITY_PRODUCTION_CHANGED, // ?
      ftc_client, _("Copy %s to clipboard."),
      universal_name_translation(&clipboard, buffer, sizeof(buffer)));
  return true;
}

/**
   If City tiles are hilited, paste into all those cities.
   Otherwise paste into the one city under the mouse pointer.
 */
void clipboard_paste_production(struct city *pcity)
{
  if (!can_client_issue_orders()) {
    return;
  }
  if (NULL == clipboard.value.building) {
    create_event(city_tile(pcity), E_BAD_COMMAND, ftc_client,
                 _("Clipboard is empty."));
    return;
  }
  if (!tiles_hilited_cities) {
    if (NULL != pcity && city_owner(pcity) == client.conn.playing) {
      clipboard_send_production_packet(pcity);
    }
    return;
  }
}

/**
   Send request to build production in clipboard to server.
 */
static void clipboard_send_production_packet(struct city *pcity)
{
  if (are_universals_equal(&pcity->production, &clipboard)
      || !can_city_build_now(pcity, &clipboard)) {
    return;
  }

  dsend_packet_city_change(&client.conn, pcity->id, clipboard.kind,
                           universal_number(&clipboard));
}

/**
   A newer technology may be available for units.
   Also called from packhand.c.
 */
void upgrade_canvas_clipboard()
{
  if (!can_client_issue_orders()) {
    return;
  }
  if (VUT_UTYPE == clipboard.kind) {
    const struct unit_type *u =
        can_upgrade_unittype(client.conn.playing, clipboard.value.utype);

    if (u) {
      clipboard.value.utype = u;
    }
  }
}

/**
   Goto button has been released. Finish goto.
 */
void release_goto_button(int canvas_x, int canvas_y)
{
  struct tile *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (keyboardless_goto_active
      && (hover_state == HOVER_GOTO || hover_state == HOVER_GOTO_SEL_TGT)
      && ptile) {
    do_unit_goto(ptile);
    clear_hover_state();
    update_unit_info_label(get_units_in_focus());
  }
  keyboardless_goto_active = false;
  keyboardless_goto_button_down = false;
  keyboardless_goto_start_tile = NULL;
}

/**
  The goto hover state is only activated when the mouse pointer moves
  beyond the tile where the button was depressed, to avoid mouse typos.
 */
void maybe_activate_keyboardless_goto(int canvas_x, int canvas_y)
{
  struct tile *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (ptile && get_num_units_in_focus() > 0
      && !same_pos(keyboardless_goto_start_tile, ptile)
      && can_client_issue_orders()) {
    keyboardless_goto_active = true;
    request_unit_goto(ORDER_LAST, ACTION_NONE, -1);
  }
}

/**
   Return TRUE iff the turn done button should be enabled.
 */
bool get_turn_done_button_state()
{
  return can_end_turn()
         && (is_human(client.conn.playing)
             || gui_options.ai_manual_turn_done);
}

/**
   Return TRUE iff client can end turn.
 */
bool can_end_turn()
{
  struct option *opt;

  opt = optset_option_by_name(server_optset, "fixedlength");
  if (opt != NULL && option_bool_get(opt)) {
    return false;
  }

  return (can_client_issue_orders() && client.conn.playing->is_alive
          && !client.conn.playing->phase_done && !is_server_busy()
          && is_player_phase(client.conn.playing, game.info.phase)
          && governor::i()->hot());
}

/**
   Scroll the mapview half a screen in the given direction.  This is a GUI
   direction; i.e., DIR8_NORTH is "up" on the mapview.
 */
void scroll_mapview(enum direction8 gui_dir)
{
  int gui_x = mapview.gui_x0, gui_y = mapview.gui_y0;

  if (!can_client_change_view()) {
    return;
  }

  gui_x += DIR_DX[gui_dir] * mapview.width / 2;
  gui_y += DIR_DY[gui_dir] * mapview.height / 2;
  set_mapview_origin(gui_x, gui_y);
}

/**
   Do some appropriate action when the "main" mouse button (usually
   left-click) is pressed.  For more sophisticated user control use (or
   write) a different xxx_button_pressed function.
 */
void action_button_pressed(int canvas_x, int canvas_y,
                           enum quickselect_type qtype)
{
  struct tile *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    /* FIXME: Some actions here will need to check can_client_issue_orders.
     * But all we can check is the lowest common requirement. */
    do_map_click(ptile, qtype);
  }
}

/**
   Wakeup sentried units on the tile of the specified location.
 */
void wakeup_button_pressed(int canvas_x, int canvas_y)
{
  struct tile *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (can_client_issue_orders() && ptile) {
    wakeup_sentried_units(ptile);
  }
}

/**
   Adjust the position of city workers from the mapview.
 */
void adjust_workers_button_pressed(int canvas_x, int canvas_y)
{
  struct tile *ptile = canvas_pos_to_tile(canvas_x, canvas_y);

  if (NULL != ptile && can_client_issue_orders()) {
    struct city *pcity = find_city_near_tile(ptile);

    if (pcity && !cma_is_city_under_agent(pcity, NULL)) {
      int city_x, city_y;

      fc_assert_ret(city_base_to_city_map(&city_x, &city_y, pcity, ptile));

      if (NULL != tile_worked(ptile) && tile_worked(ptile) == pcity) {
        dsend_packet_city_make_specialist(&client.conn, pcity->id,
                                          ptile->index);
      } else if (city_can_work_tile(pcity, ptile)) {
        dsend_packet_city_make_worker(&client.conn, pcity->id, ptile->index);
      } else {
        return;
      }

      /* When the city info packet is received, update the workers on the
       * map.  This is a bad hack used to selectively update the mapview
       * when we receive the corresponding city packet. */
      city_workers_display = pcity;
    }
  }
}

/**
   Recenter the map on the canvas location, on user request.  Usually this
   is done with a right-click.
 */
void recenter_button_pressed(int canvas_x, int canvas_y)
{
  // We use the "nearest" tile here so off-map clicks will still work.
  struct tile *ptile = canvas_pos_to_nearest_tile(canvas_x, canvas_y);

  if (can_client_change_view() && ptile) {
    center_tile_mapcanvas(ptile);
  }
}

/**
   Update the turn done button state.
 */
void update_turn_done_button_state()
{
  bool turn_done_state = get_turn_done_button_state();

  set_turn_done_button_state(turn_done_state);

  if (can_end_turn()) {
    if (waiting_for_end_turn) {
      send_turn_done();
    } else {
      update_turn_done_button(true);
    }
  }
}

/**
   Update the goto/patrol line to the given map canvas location.
 */
void update_line(int canvas_x, int canvas_y)
{
  struct tile *ptile;
  struct unit_list *punits;

  switch (hover_state) {
  case HOVER_GOTO:
  case HOVER_PATROL:
  case HOVER_CONNECT:
    ptile = canvas_pos_to_tile(canvas_x, canvas_y);

    is_valid_goto_draw_line(ptile);
    break;
  case HOVER_GOTO_SEL_TGT:
    ptile = canvas_pos_to_tile(canvas_x, canvas_y);
    punits = get_units_in_focus();
    fc_assert_ret(ptile);
    set_hover_state(punits, hover_state, connect_activity, connect_tgt,
                    ptile->index, goto_last_sub_tgt, goto_last_action,
                    goto_last_order);
    break;
  case HOVER_NONE:
  case HOVER_PARADROP:
  case HOVER_ACT_SEL_TGT:
  case HOVER_DEBUG_TILE:
    break;
  };
}

/**
   Update the goto/patrol line to the given overview canvas location.
 */
void overview_update_line(int overview_x, int overview_y)
{
  struct tile *ptile;
  struct unit_list *punits;
  int x, y;

  switch (hover_state) {
  case HOVER_GOTO:
  case HOVER_PATROL:
  case HOVER_CONNECT:
    overview_to_map_pos(&x, &y, overview_x, overview_y);
    ptile = map_pos_to_tile(&(wld.map), x, y);

    is_valid_goto_draw_line(ptile);
    break;
  case HOVER_GOTO_SEL_TGT:
    overview_to_map_pos(&x, &y, overview_x, overview_y);
    ptile = map_pos_to_tile(&(wld.map), x, y);
    punits = get_units_in_focus();
    fc_assert_ret(ptile);
    set_hover_state(punits, hover_state, connect_activity, connect_tgt,
                    ptile->index, goto_last_sub_tgt, goto_last_action,
                    goto_last_order);
    break;
  case HOVER_NONE:
  case HOVER_PARADROP:
  case HOVER_ACT_SEL_TGT:
  case HOVER_DEBUG_TILE:
    break;
  };
}

/**
   We sort according to the following logic:

   - Transported units should immediately follow their transporter (note that
     transporting may be recursive).
   - Otherwise we sort by ID (which is what the list is originally sorted
 by).
 */
static int unit_list_compare(const void *a, const void *b)
{
  const struct unit *punit1 = *(struct unit **) a;
  const struct unit *punit2 = *(struct unit **) b;

  if (unit_transport_get(punit1) == unit_transport_get(punit2)) {
    // For units with the same transporter or no transporter: sort by id.
    // Perhaps we should sort by name instead?
    return punit1->id - punit2->id;
  } else if (unit_transport_get(punit1) == punit2) {
    return 1;
  } else if (unit_transport_get(punit2) == punit1) {
    return -1;
  } else {
    /* If the transporters aren't the same, put in order by the
     * transporters. */
    const struct unit *ptrans1 = unit_transport_get(punit1);
    const struct unit *ptrans2 = unit_transport_get(punit2);

    if (!ptrans1) {
      ptrans1 = punit1;
    }
    if (!ptrans2) {
      ptrans2 = punit2;
    }

    return unit_list_compare(&ptrans1, &ptrans2);
  }
}

/**
   Fill and sort the list of units on the tile.
 */
void fill_tile_unit_list(const struct tile *ptile, struct unit **unit_list)
{
  int i = 0;

  // First populate the unit list.
  unit_list_iterate(ptile->units, punit)
  {
    unit_list[i] = punit;
    i++;
  }
  unit_list_iterate_end;

  // Then sort it.
  qsort(unit_list, i, sizeof(*unit_list), unit_list_compare);
}
