/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// common
#include "fc_types.h"
// client
#include "tilespec.h"
// gui-qt
#include "pages_g.h"

class QTcpSocket;

// void setup_gui_funcs();

void qtg_options_extra_init();
void qtg_set_rulesets(int num_rulesets, QStringList rulesets);
void qtg_add_net_input(QTcpSocket *sock);
void qtg_remove_net_input();
void qtg_real_conn_list_dialog_update(void *unused);
void qtg_close_connection_dialog();
void qtg_add_idle_callback(void(callback)(void *), void *data);
void qtg_sound_bell();

void qtg_real_set_client_page(client_pages page);
client_pages qtg_get_current_client_page();

void qtg_popup_combat_info(int attacker_unit_id, int defender_unit_id,
                           int attacker_hp, int defender_hp,
                           bool make_att_veteran, bool make_def_veteran);
void qtg_set_unit_icon(int idx, unit *punit);
void qtg_set_unit_icons_more_arrow(bool onoff);
void qtg_real_focus_units_changed();
void qtg_gui_update_font(const QString &font_name,
                         const QString &font_value);

void qtg_editgui_refresh();
void qtg_editgui_notify_object_created(int tag, int id);
void qtg_editgui_notify_object_changed(int objtype, int object_id,
                                       bool removal);
void qtg_editgui_popup_properties(const tile_list *tiles, int objtype);
void qtg_editgui_tileset_changed();
void qtg_editgui_popdown_all();

void qtg_update_timeout_label();
void qtg_start_turn();
void qtg_real_city_dialog_popup(struct city *pcity);
void qtg_real_city_dialog_refresh(struct city *pcity);
void qtg_popdown_city_dialog(struct city *pcity);
void qtg_popdown_all_city_dialogs();
bool qtg_handmade_scenario_warning();
void qtg_refresh_unit_city_dialogs(struct unit *punit);
bool qtg_city_dialog_is_open(struct city *pcity);

bool qtg_request_transport(struct unit *pcargo, struct tile *ptile);

// void qtg_update_infra_dialog();

void qtg_gui_clear_theme();
QStringList qtg_get_gui_specific_themes_directories(int *count);
