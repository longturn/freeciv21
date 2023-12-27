/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// Qt
#include <QApplication>
#include <QMouseEvent>
// common
#include "control.h"
#include "goto.h"
#include "map.h"
// client
#include "chatline_common.h"
#include "citydlg_common.h"
#include "client_main.h"
#include "mapctrl.h"
#include "mapctrl_common.h"
#include "themes_common.h"
#include "tile.h"
#include "tileset/tilespec.h"
#include "unit.h"
#include "views/view_map_common.h"
// gui-qt
#include "citydlg.h"
#include "fc_client.h"
#include "messagewin.h"
#include "page_game.h"
#include "shortcuts.h"
#include "unitselect.h"
#include "views/view_map.h"

extern void qload_lua_script();
extern void qreload_lua_script();

/**
   Popup a dialog to ask for the name of a new city.  The given string
   should be used as a suggestion.
 */
void popup_newcity_dialog(struct unit *punit, const char *suggestname)
{
  hud_input_box *ask = new hud_input_box(king()->central_wdg);
  int index = tile_index(unit_tile(punit));

  ask->set_text_title_definput(_("What should we call our new city?"),
                               _("Build New City"), suggestname);
  ask->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(ask, &hud_input_box::finished, [=](int result) {
    if (result == QDialog::Accepted) {
      QByteArray ask_bytes;

      ask_bytes = ask->input_edit.text().toLocal8Bit();
      finish_city(index_to_tile(&(wld.map), index), ask_bytes.data());
    } else {
      cancel_city(index_to_tile(&(wld.map), index));
    }
  });
  ask->show();
}

/**
   Draw a goto or patrol line at the current mouse position.
 */
void create_line_at_mouse_pos(void)
{
  QPoint global_pos, local_pos;
  int x, y;

  global_pos = QCursor::pos();
  local_pos = queen()->mapview_wdg->mapFromGlobal(global_pos)
              / queen()->mapview_wdg->scale();
  x = local_pos.x();
  y = local_pos.y();

  if (x >= 0 && y >= 0 && x < mapview.width && y < mapview.width) {
    update_line(x, y);
  }
}

/**
   The Area Selection rectangle. Called by center_tile_mapcanvas() and
   when the mouse pointer moves.
 */
void update_rect_at_mouse_pos(void)
{ // PLS DONT PORT IT
}

/**
   Keyboard handler for map_view
 */
void map_view::keyPressEvent(QKeyEvent *event)
{
  Qt::KeyboardModifiers key_mod = QApplication::keyboardModifiers();
  bool is_shift = key_mod.testFlag(Qt::ShiftModifier);

  if (C_S_RUNNING == client_state()) {
    if (is_shift) {
      switch (event->key()) {
      case Qt::Key_Return:
      case Qt::Key_Enter:
        key_end_turn();
        return;
      default:
        break;
      }
    }

    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_8:
      if (is_shift) {
        recenter_button_pressed(width() / 2 / scale(), 0);
      } else {
        key_unit_move(DIR8_NORTH);
      }
      return;
    case Qt::Key_Left:
    case Qt::Key_4:
      if (is_shift) {
        recenter_button_pressed(0, height() / 2 / scale());
      } else {
        key_unit_move(DIR8_WEST);
      }
      return;
    case Qt::Key_Right:
    case Qt::Key_6:
      if (is_shift) {
        recenter_button_pressed(width() / scale(), height() / 2 / scale());
      } else {
        key_unit_move(DIR8_EAST);
      }
      return;
    case Qt::Key_Down:
    case Qt::Key_2:
      if (is_shift) {
        recenter_button_pressed(width() / 2 / scale(), height() / scale());
      } else {
        key_unit_move(DIR8_SOUTH);
      }
      return;
    case Qt::Key_PageUp:
    case Qt::Key_9:
      key_unit_move(DIR8_NORTHEAST);
      return;
    case Qt::Key_PageDown:
    case Qt::Key_3:
      key_unit_move(DIR8_SOUTHEAST);
      return;
    case Qt::Key_Home:
    case Qt::Key_7:
      key_unit_move(DIR8_NORTHWEST);
      return;
    case Qt::Key_End:
    case Qt::Key_1:
      key_unit_move(DIR8_SOUTHWEST);
      return;
    case Qt::Key_5:
    case Qt::Key_Clear:
      key_recall_previous_focus_unit();
      return;
    case Qt::Key_Escape:
      key_cancel_action();
      return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
      // Ensure chat is visible
      queen()->chat->take_focus();
      return;
    default:
      break;
    }
  }
}

/**
   Pressed mouse or keyboard
 */
void map_view::shortcut_pressed(shortcut_id id)
{
  // FIXME mouse handling
  auto pos = mapFromGlobal(QCursor::pos()) / scale();

  auto ptile = canvas_pos_to_tile(pos.x(), pos.y());
  auto pcity = ptile ? tile_city(ptile) : nullptr;

  if (pcity && pcity->owner != client_player()) {
    pcity = nullptr;
  }

  switch (id) {
  case SC_SELECT_BUTTON:
    // Trade Generator - skip
    if (king()->trade_gen.hover_city) {
      king()->trade_gen.add_tile(ptile);
      queen()->mapview_wdg->repaint();
      return;
    }

    // Rally point - select city - skip
    if (king()->rallies.hover_city) {
      char text[1024];

      if (ptile && tile_city(ptile)) {
        king()->rallies.hover_tile = true;
        king()->rallies.rally_city = tile_city(ptile);

        fc_snprintf(text, sizeof(text),
                    _("Selected city %s. Now choose rally point."),
                    city_link(tile_city(ptile)));
        output_window_append(ftc_client, text);
      } else {
        output_window_append(ftc_client, _("No city selected. Aborted"));
      }
      return;
    }

    // Rally point - select tile  - skip
    if (king()->rallies.hover_tile && ptile != nullptr) {
      char text[1024];

      struct city *pcity = king()->rallies.rally_city;
      fc_assert_ret(pcity != nullptr);

      if (send_rally_tile(pcity, ptile)) {
        fc_snprintf(text, sizeof(text),
                    _("Tile %s set as rally point from city %s."),
                    tile_link(ptile), city_link(pcity));
        output_window_append(ftc_client, text);
      } else {
        fc_snprintf(text, sizeof(text),
                    _("Could not set rally point for city %s."),
                    city_link(pcity));
        output_window_append(ftc_client, text);
      }

      king()->rallies.rally_city = nullptr;
      king()->rallies.hover_tile = false;
      return;
    }

    if (king()->menu_bar->delayed_order && ptile) {
      king()->menu_bar->set_tile_for_order(ptile);
      clear_hover_state();
      exit_goto_state();
      king()->menu_bar->delayed_order = false;
      return;
    }

    if (king()->menu_bar->quick_airlifting && ptile) {
      if (tile_city(ptile)) {
        multiairlift(tile_city(ptile), king()->menu_bar->airlift_type_id);
      } else {
        output_window_append(ftc_client, "No city selected for airlift");
      }
      king()->menu_bar->quick_airlifting = false;
      return;
    }

    if (!goto_is_active()) {
      set_auto_center_enabled(false);
      action_button_pressed(pos.x(), pos.y(), SELECT_FOCUS);
      return;
    }
    break;

  case SC_QUICK_SELECT:
    if (pcity != nullptr && !king()->menu_bar->delayed_order) {
      auto pw = new production_widget(this, pcity, false, 0, 0, true);
      pw->show();
    }
    break;

  case SC_SHOW_UNITS:
    if (ptile != nullptr && unit_list_size(ptile->units) > 0) {
      toggle_unit_sel_widget(ptile);
    }
    break;

  case SC_COPY_PROD:
    if (ptile != nullptr) {
      clipboard_copy_production(ptile);
    }
    break;

  case SC_POPUP_COMB_INF:
    if (queen()->battlelog_wdg != nullptr) {
      queen()->battlelog_wdg->show();
    }
    break;

  case SC_PASTE_PROD:
    if (pcity != nullptr) {
      clipboard_paste_production(pcity);
    }
    break;

  case SC_RELOAD_THEME:
    load_theme(gui_options->gui_qt_default_theme_name);
    break;

  case SC_RELOAD_TILESET:
    QPixmapCache::clear();
    tilespec_reread(tileset_basename(tileset), true);
    break;

  case SC_LOAD_LUA:
    qload_lua_script();
    break;

  case SC_RELOAD_LUA:
    qreload_lua_script();
    break;

  case SC_HIDE_WORKERS:
    key_city_overlay(pos.x(), pos.y());
    break;

  case SC_MAKE_LINK:
    if (ptile != nullptr) {
      queen()->chat->make_link(ptile);
    }
    break;

  case SC_BUY_MAP:
    if (pcity != nullptr) {
      city_buy_production(pcity);
    }
    break;

  case SC_QUICK_BUY:
    if (pcity != nullptr) {
      auto pw = new production_widget(this, pcity, false, 0, 0, true, true);
      pw->show();
    }
    break;

  case SC_APPEND_FOCUS:
    action_button_pressed(pos.x(), pos.y(), SELECT_APPEND);
    break;

  case SC_ADJUST_WORKERS:
    adjust_workers_button_pressed(pos.x(), pos.y());
    break;

  case SC_SCROLL_MAP:
    recenter_button_pressed(pos.x(), pos.y());
    break;

  case SC_POPUP_INFO:
    if (ptile != nullptr) {
      popup_tile_info(ptile);
    }
    break;

  case SC_WAKEUP_SENTRIES:
    wakeup_button_pressed(pos.x(), pos.y());
    break;

  default:
    // Many actions aren't handled here
    break;
  }
}

/**
   Released mouse buttons
 */
void map_view::shortcut_released(Qt::MouseButton bt)
{
  auto md = QApplication::keyboardModifiers();
  auto pos = mapFromGlobal(QCursor::pos()) / scale();

  if (info_tile::shown()) {
    popdown_tile_info();
  }

  auto sc = fc_shortcuts::sc()->get_shortcut(SC_SELECT_BUTTON);
  if (bt == sc.buttons && md == sc.modifiers) {
    if (king()->trade_gen.hover_city || king()->rallies.hover_city) {
      king()->trade_gen.hover_city = false;
      king()->rallies.hover_city = false;
      return;
    }
    if (!keyboardless_goto_active || goto_is_active()) {
      action_button_pressed(pos.x(), pos.y(), SELECT_POPUP);
    }
    set_auto_center_enabled(true);
    release_goto_button(pos.x(), pos.y());
    return;
  }
}

/**
   Mouse buttons handler for map_view
 */
void map_view::mousePressEvent(QMouseEvent *event)
{
  fc_shortcuts::sc()->maybe_route_mouse_shortcut(event, this);
}

/**
   Mouse release event for map_view
 */
void map_view::mouseReleaseEvent(QMouseEvent *event)
{
  shortcut_released(event->button());
}

/**
   Mouse movement handler for map_view
 */
void map_view::mouseMoveEvent(QMouseEvent *event)
{
  update_line(event->pos().x() / scale(), event->pos().y() / scale());
  if (keyboardless_goto_button_down && hover_state == HOVER_NONE) {
    maybe_activate_keyboardless_goto(event->pos().x() / scale(),
                                     event->pos().y() / scale());
  }
  control_mouse_cursor(canvas_pos_to_tile(event->pos().x() / scale(),
                                          event->pos().y() / scale()));
}
