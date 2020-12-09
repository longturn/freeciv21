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
#include "fc_types.h"
#include "unitlist.h"

/* client */
#include "mapview_common.h"

/* client/include */
#include "canvas_g.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, update_info_label, void)
GUI_FUNC_PROTO(void, update_unit_info_label, struct unit_list *punitlist)
GUI_FUNC_PROTO(void, update_mouse_cursor, enum cursor_type new_cursor_type)
GUI_FUNC_PROTO(void, update_timeout_label, void)
GUI_FUNC_PROTO(void, update_turn_done_button, bool do_restore)
GUI_FUNC_PROTO(void, update_city_descriptions, void)
GUI_FUNC_PROTO(void, set_indicator_icons, struct sprite *bulb,
               struct sprite *sol, struct sprite *flake, struct sprite *gov)

GUI_FUNC_PROTO(void, start_turn, void)

GUI_FUNC_PROTO(void, update_minimap, void)

GUI_FUNC_PROTO(void, dirty_rect, int canvas_x, int canvas_y, int pixel_width,
               int pixel_height)
GUI_FUNC_PROTO(void, dirty_all, void)
GUI_FUNC_PROTO(void, flush_dirty, void)
GUI_FUNC_PROTO(void, gui_flush, void)

GUI_FUNC_PROTO(void, update_map_canvas_scrollbars, void)

GUI_FUNC_PROTO(void, put_cross_overlay_tile, struct tile *ptile)

GUI_FUNC_PROTO(void, draw_selection_rectangle, int canvas_x, int canvas_y,
               int w, int h)
GUI_FUNC_PROTO(void, tileset_changed, void)

