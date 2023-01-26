/**************************************************************************
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
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
#include "tileset/tilespec.h"
// gui-qt
#include "pages_g.h"

class QTcpSocket;

void options_extra_init();
void set_rulesets(int num_rulesets, QStringList rulesets);
void add_net_input(QTcpSocket *sock);
void remove_net_input();
void real_conn_list_dialog_update(void *unused);
void sound_bell();

void real_set_client_page(client_pages page);
client_pages get_current_client_page();

void popup_combat_info(int attacker_unit_id, int defender_unit_id,
                       int attacker_hp, int defender_hp,
                       bool make_att_veteran, bool make_def_veteran);
void set_unit_icon(int idx, unit *punit);
void set_unit_icons_more_arrow(bool onoff);
void real_focus_units_changed();
void gui_update_font(const QString &font_name, const QFont &font_value);

void editgui_refresh();
void editgui_notify_object_created(int tag, int id);
void editgui_notify_object_changed(int objtype, int object_id, bool removal);
void editgui_popup_properties(const tile_list *tiles, int objtype);
void editgui_tileset_changed();
void editgui_popdown_all();

void start_turn();
void real_city_dialog_popup(struct city *pcity);
void real_city_dialog_refresh(struct city *pcity);
void popdown_city_dialog();
bool handmade_scenario_warning();
void refresh_unit_city_dialogs(struct unit *punit);
bool city_dialog_is_open(struct city *pcity);

bool request_transport(struct unit *pcargo, struct tile *ptile);

void gui_clear_theme();
QStringList get_gui_specific_themes_directories(int *count);
