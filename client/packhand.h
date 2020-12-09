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
#ifndef FC__PACKHAND_H
#define FC__PACKHAND_H


#include "fc_types.h" /* struct connection, struct government */

#include "events.h" /* enum event_type */
#include "map.h"

#include "packhand_gen.h"

void packhand_free(void);

void notify_about_incoming_packet(struct connection *pc, int packet_type,
                                  int size);
void notify_about_outgoing_packet(struct connection *pc, int packet_type,
                                  int size, int request_id);
void set_reports_thaw_request(int request_id);

void play_sound_for_event(enum event_type type);
void target_government_init(void);
void set_government_choice(struct government *government);
void start_revolution(void);


#endif /* FC__PACKHAND_H */
