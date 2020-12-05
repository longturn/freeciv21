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
#ifndef FC__REPODLGS_G_H
#define FC__REPODLGS_G_H

/* utility */
#include "support.h" /* bool type */

/* common */
#include "packets.h"

/* client */
#include "repodlgs_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, science_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, science_report_dialog_redraw, void)
GUI_FUNC_PROTO(void, economy_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, units_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, endgame_report_dialog_start,
               const struct packet_endgame_report *packet)
GUI_FUNC_PROTO(void, endgame_report_dialog_player,
               const struct packet_endgame_player *packet)

GUI_FUNC_PROTO(void, real_science_report_dialog_update, void *unused)
GUI_FUNC_PROTO(void, real_economy_report_dialog_update, void *unused)
GUI_FUNC_PROTO(void, real_units_report_dialog_update, void *unused)

/* Actually defined in update_queue.c */
void science_report_dialog_update(void);
void economy_report_dialog_update(void);
void units_report_dialog_update(void);

#endif /* FC__REPODLGS_G_H */
