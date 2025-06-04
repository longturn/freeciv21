// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

// the maximum number of enumerators is set in generate_specenum.py

extern enum event_type sorted_events[]; /* [E_COUNT], sorted by the
                                           translated message text */

const char *get_event_message_text(enum event_type event);
const char *get_event_tag(enum event_type event);

void events_init();
void events_free();

// Iterates over all events, sorted by the message text string.
#define sorted_event_iterate(event)                                         \
  {                                                                         \
    enum event_type event;                                                  \
    enum event_type event##_index;                                          \
    for (event##_index = event_type_begin();                                \
         event##_index != event_type_end();                                 \
         event##_index = event_type_next(event##_index)) {                  \
      event = sorted_events[event##_index];                                 \
      {

#define sorted_event_iterate_end                                            \
  }                                                                         \
  }                                                                         \
  }
