/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// client
#include "gui_interface.h"
// gui-qt
#include "fonts.h"
#include "qtg_cxxside.h"

/**
   Setup the gui callback table.
 */
void setup_gui_funcs()
{
  struct gui_funcs *funcs = get_gui_funcs();

  funcs->canvas_put_sprite_fogged = qtg_canvas_put_sprite_fogged;
  funcs->canvas_put_sprite_citymode = qtg_canvas_put_sprite_citymode;
  funcs->canvas_put_line = qtg_canvas_put_line;
  funcs->canvas_put_curved_line = qtg_canvas_put_curved_line;
  funcs->get_text_size = qtg_get_text_size;
  funcs->canvas_put_text = qtg_canvas_put_text;

  funcs->set_rulesets = qtg_set_rulesets;
  funcs->options_extra_init = qtg_options_extra_init;
  funcs->add_net_input = qtg_add_net_input;
  funcs->remove_net_input = qtg_remove_net_input;
  funcs->real_conn_list_dialog_update = qtg_real_conn_list_dialog_update;
  funcs->close_connection_dialog = qtg_close_connection_dialog;
  funcs->add_idle_callback = qtg_add_idle_callback;
  funcs->sound_bell = qtg_sound_bell;

  funcs->real_set_client_page = qtg_real_set_client_page;
  funcs->get_current_client_page = qtg_get_current_client_page;

  funcs->set_unit_icon = qtg_set_unit_icon;
  funcs->set_unit_icons_more_arrow = qtg_set_unit_icons_more_arrow;
  funcs->real_focus_units_changed = qtg_real_focus_units_changed;
  funcs->gui_update_font = qtg_gui_update_font;

  funcs->editgui_refresh = qtg_editgui_refresh;
  funcs->editgui_notify_object_created = qtg_editgui_notify_object_created;
  funcs->editgui_notify_object_changed = qtg_editgui_notify_object_changed;
  funcs->editgui_popup_properties = qtg_editgui_popup_properties;
  funcs->editgui_tileset_changed = qtg_editgui_tileset_changed;
  funcs->editgui_popdown_all = qtg_editgui_popdown_all;

  funcs->popup_combat_info = qtg_popup_combat_info;
  funcs->update_timeout_label = qtg_update_timeout_label;
  funcs->start_turn = qtg_start_turn;
  funcs->real_city_dialog_popup = qtg_real_city_dialog_popup;
  funcs->real_city_dialog_refresh = qtg_real_city_dialog_refresh;
  funcs->popdown_city_dialog = qtg_popdown_city_dialog;
  funcs->popdown_all_city_dialogs = qtg_popdown_all_city_dialogs;
  funcs->handmade_scenario_warning = qtg_handmade_scenario_warning;
  funcs->refresh_unit_city_dialogs = qtg_refresh_unit_city_dialogs;
  funcs->city_dialog_is_open = qtg_city_dialog_is_open;

  funcs->request_transport = qtg_request_transport;

  funcs->update_infra_dialog = [] {};

  funcs->gui_load_theme = qtg_gui_load_theme;
  funcs->gui_clear_theme = qtg_gui_clear_theme;
  funcs->get_gui_specific_themes_directories =
      qtg_get_gui_specific_themes_directories;
  funcs->get_useable_themes_in_directory =
      qtg_get_useable_themes_in_directory;
}
