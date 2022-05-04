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

// utility
#include "support.h" // bool type

// common
#include "fc_types.h"

// client
#include "mapctrl_common.h"

void popup_newcity_dialog(struct unit *punit, const char *suggestname);

void set_turn_done_button_state(bool state);

void create_line_at_mouse_pos();
void update_rect_at_mouse_pos();
