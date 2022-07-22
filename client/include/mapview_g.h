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

// utility
#include "support.h" // bool type

// common
#include "fc_types.h"
#include "unitlist.h"

// client
#include "mapview_common.h"

void update_info_label(void);
void update_unit_info_label(struct unit_list *punitlist);
void update_mouse_cursor(enum cursor_type new_cursor_type);
void update_turn_done_button(bool do_restore);
void update_city_descriptions(void);
void update_minimap(void);

void dirty_rect(int canvas_x, int canvas_y, int pixel_width,
                int pixel_height);
void dirty_all(void);
void flush_dirty(void);

void update_map_canvas_scrollbars(void);

void put_cross_overlay_tile(struct tile *ptile);

void draw_selection_rectangle(int canvas_x, int canvas_y, int w, int h);
void tileset_changed(void);
void show_city_desc(QPixmap *pcanvas, int canvas_x, int canvas_y,
                    struct city *pcity, int *width, int *height);

void debug_tile(tile *t);
