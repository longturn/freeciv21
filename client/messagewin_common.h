/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#pragma once

/* utility */
#include "support.h" /* bool type */

/* common */
#include "events.h"        /* enum event_type */
#include "fc_types.h"      /* struct tile */
#include "featured_text.h" /* struct text_tag_list */

struct message {
  char *descr;
  struct text_tag_list *tags;
  struct tile *tile;
  enum event_type event;
  bool location_ok;
  bool city_ok;
  bool visited;
  int turn;
  int phase;
};

#define MESWIN_CLEAR_ALL (-1)

void meswin_clear_older(int turn, int phase);
void meswin_add(const char *message, const struct text_tag_list *tags,
                struct tile *ptile, enum event_type event, int turn,
                int phase);

const struct message *meswin_get_message(int message_index);
int meswin_get_num_messages(void);
void meswin_set_visited_state(int message_index, bool state);
void meswin_popup_city(int message_index);
void meswin_goto(int message_index);
void meswin_double_click(int message_index);


