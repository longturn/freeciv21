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

// Forward declarations
class QTcpSocket;

// utility
#include "support.h" // bool type

// common
#include "fc_types.h"

void ui_main();
void ui_exit();

// void options_extra_init();

// void real_conn_list_dialog_update(void*);
// void sound_bell();
// void add_net_input(QTcpSocket *)
// void remove_net_input();

// void set_unit_icon(int idx, unit *punit);
// void set_unit_icons_more_arrow(bool onoff);
// void real_focus_units_changed();

// void add_idle_callback(void(callback)(void *), void *data);

void insert_client_build_info(char *outbuf, size_t outlen);

// void gui_update_font(const QString &font_name, const QString &font_value);

extern const char *client_string;

// Actually defined in update_queue.c
void gui_update_allfonts();
void conn_list_dialog_update();
