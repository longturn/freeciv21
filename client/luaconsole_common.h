/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
                   part of Freeciv21. Freeciv21 is free software: you can
    ^oo^      redistribute it and/or modify it under the terms of the GNU
    (..)        General Public License  as published by the Free Software
   ()  ()       Foundation, either version 3 of the License,  or (at your
   ()__()             option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

struct ft_color;
struct text_tag_list;

void luaconsole_append(const struct ft_color color,
                       const char *featured_text);
void luaconsole_vprintf(const struct ft_color color, const char *format,
                        va_list args);
void luaconsole_printf(const struct ft_color color, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)));
void luaconsole_event(const char *plain_text,
                      const struct text_tag_list *tags);
void luaconsole_welcome_message();
