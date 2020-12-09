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

/* utility */
#include "support.h" /* bool type */

/* common */
#include "fc_types.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, ui_init, void)
GUI_FUNC_PROTO(void, ui_main, void)
GUI_FUNC_PROTO(void, ui_exit, void)
GUI_FUNC_PROTO(void, options_extra_init, void)

GUI_FUNC_PROTO(void, real_conn_list_dialog_update, void *)
GUI_FUNC_PROTO(void, sound_bell, void)
GUI_FUNC_PROTO(void, add_net_input, QTcpSocket *)
GUI_FUNC_PROTO(void, remove_net_input, void)

GUI_FUNC_PROTO(void, set_unit_icon, int idx, struct unit *punit)
GUI_FUNC_PROTO(void, set_unit_icons_more_arrow, bool onoff)
GUI_FUNC_PROTO(void, real_focus_units_changed, void)

GUI_FUNC_PROTO(void, add_idle_callback, void(callback)(void *), void *data)

GUI_FUNC_PROTO(enum gui_type, get_gui_type, void)
GUI_FUNC_PROTO(void, insert_client_build_info, char *outbuf, size_t outlen)

GUI_FUNC_PROTO(void, gui_update_font, const char *font_name,
               const char *font_value)

extern const char *client_string;

/* Actually defined in update_queue.c */
void gui_update_allfonts();
void conn_list_dialog_update(void);

