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

#include "fc_types.h"

#include "citydlg_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, real_city_dialog_popup, struct city *pcity)
GUI_FUNC_PROTO(void, popdown_city_dialog, struct city *pcity)
GUI_FUNC_PROTO(void, popdown_all_city_dialogs, void)
GUI_FUNC_PROTO(void, real_city_dialog_refresh, struct city *pcity)
GUI_FUNC_PROTO(void, refresh_unit_city_dialogs, struct unit *punit)
GUI_FUNC_PROTO(bool, city_dialog_is_open, struct city *pcity)

/* Actually defined in update_queue.c */
void popup_city_dialog(struct city *pcity);
void refresh_city_dialog(struct city *pcity);
struct city * is_any_city_dialog_open();


