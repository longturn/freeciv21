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

#include "citydlg_common.h"
#include "fc_types.h"

// void real_city_dialog_popup(city *pcity);
// void popdown_city_dialog(city* pcity);
// void popdown_all_city_dialogs();
// void real_city_dialog_refresh(city* pcity);
// void refresh_unit_city_dialogs(unit* punit);
// bool city_dialog_is_open(city* pcity);

// Actually defined in update_queue.c
void popup_city_dialog(city *pcity);
void refresh_city_dialog(city *pcity);
city *is_any_city_dialog_open();
