/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

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
#include "client_main.h"
#include "climisc.h"
#include "control.h"
#include "goto.h"
#include "governor.h"
#include "mapctrl_common.h"
#include "mapview.h"
#include "mapview_common.h"
#include "mapview_g.h"
#include "minimap_panel.h"
#include "options.h"
#include "page_game.h"

static int rec_corner_x, rec_corner_y; // corner to iterate from
static int rec_w, rec_h;               // width, heigth in pixels

bool rectangle_active = false;

/* This changes the behaviour of left mouse
   button in Area Selection mode. */
bool tiles_hilited_cities = false;

// The mapcanvas clipboard
struct universal clipboard = {.value = {.building = nullptr},
                              .kind = VUT_NONE};

// Goto with drag and drop.
bool keyboardless_goto_button_down = false;
bool keyboardless_goto_active = false;
struct tile *keyboardless_goto_start_tile;

// Update the workers for a city on the map, when the update is received
struct city *city_workers_display = nullptr;

/*************************************************************************/

static void clipboard_send_production_packet(struct city *pcity);

/**
   Redraws the selection rectangle after a map flush.
 */
void cancel_selection_rectangle()
{
  if (rectangle_active) {
    rectangle_active = false;

    // Erase the previously drawn selection rectangle.
    draw_selection_rectangle(rec_corner_x, rec_corner_y, rec_w, rec_h);
  }
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
  if (nullptr == clipboard.value.building) {
    create_event(city_tile(pcity), E_BAD_COMMAND, ftc_client,
                 _("Clipboard is empty."));
    return;
  }
  if (!tiles_hilited_cities) {
    if (nullptr != pcity && city_owner(pcity) == client.conn.playing) {
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
  keyboardless_goto_start_tile = nullptr;
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
  if (opt != nullptr && option_bool_get(opt)) {
    return false;
  }

  return (can_client_issue_orders() && client.conn.playing->is_alive
          && !client.conn.playing->phase_done && !is_server_busy()
          && is_player_phase(client.conn.playing, game.info.phase)
          && governor::i()->hot());
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

  if (nullptr != ptile && can_client_issue_orders()) {
    struct city *pcity = find_city_or_settler_near_tile(ptile, nullptr);

    if (pcity && !cma_is_city_under_agent(pcity, nullptr)) {
      int city_x, city_y;

      fc_assert_ret(city_base_to_city_map(&city_x, &city_y, pcity, ptile));

      if (nullptr != tile_worked(ptile) && tile_worked(ptile) == pcity) {
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
    queen()->mapview_wdg->center_on_tile(ptile);
  }
}

/**
   Update the turn done button state.
 */
void update_turn_done_button_state()
{
  queen()->minimap_panel->turn_done()->setEnabled(
      get_turn_done_button_state());

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

  switch (hover_state) {
  case HOVER_GOTO:
  case HOVER_PATROL:
  case HOVER_CONNECT:
    ptile = canvas_pos_to_tile(canvas_x, canvas_y);

    is_valid_goto_draw_line(ptile);
    break;
  case HOVER_GOTO_SEL_TGT:
    ptile = canvas_pos_to_tile(canvas_x, canvas_y);
    fc_assert_ret(ptile);
    set_hover_state(get_units_in_focus(), hover_state, connect_activity,
                    connect_tgt, ptile->index, goto_last_sub_tgt,
                    goto_last_action, goto_last_order);
    break;
  case HOVER_NONE:
  case HOVER_PARADROP:
  case HOVER_ACT_SEL_TGT:
  case HOVER_DEBUG_TILE:
    break;
  };
}
