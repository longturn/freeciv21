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
#include "packets.h"

// client
#include "repodlgs_common.h"

void science_report_dialog_popup(bool raise);
void science_report_dialog_redraw(void);
void economy_report_dialog_popup();
void endgame_report_dialog_start(const struct packet_endgame_report *packet);
void endgame_report_dialog_player(
    const struct packet_endgame_player *packet);

void real_science_report_dialog_update(void *unused);
void real_economy_report_dialog_update(void *unused);
void real_units_report_dialog_update(void *unused);

// Actually defined in update_queue.c
void science_report_dialog_update();
void economy_report_dialog_update();
void units_report_dialog_update();
