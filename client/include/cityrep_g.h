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

#include "support.h" // bool type

#include "fc_types.h"

#include "gui_proto_constructor.h"

void city_report_dialog_popup();
GUI_FUNC_PROTO(void, real_city_report_dialog_update, void *unused)
GUI_FUNC_PROTO(void, real_city_report_update_city, struct city *pcity)

// Actually defined in update_queue.c
void city_report_dialog_update_city(struct city *pcity);
void city_report_dialog_update();
