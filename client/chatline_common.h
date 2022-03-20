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

// common
#include "events.h"
#include "featured_text.h" // struct text_tag_list, struct ft_color

int send_chat(const char *message);
int send_chat_printf(const char *format, ...)
    fc__attribute((__format__(__printf__, 1, 2)));

void output_window_append(event_type event, const struct ft_color color,
                          const char *featured_text);
void output_window_vprintf(event_type event, const struct ft_color color,
                           const char *format, va_list args);
void output_window_printf(event_type event, const struct ft_color color,
                          const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));
void output_window_event(event_type event, const char *plain_text,
                         const struct text_tag_list *tags);

void chat_welcome_message();

void fc_allocate_ow_mutex();
void fc_release_ow_mutex();
void fc_init_ow_mutex();
void fc_destroy_ow_mutex();
