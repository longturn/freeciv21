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

/* client/include */
#include "sprite_g.h"

/* client */
#include "tilespec.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(bool, is_view_supported, enum ts_type type)
GUI_FUNC_PROTO(void, tileset_type_set, enum ts_type type)

GUI_FUNC_PROTO(void, load_intro_gfx, void)
GUI_FUNC_PROTO(void, load_cursors, void)

GUI_FUNC_PROTO(void, free_intro_radar_sprites, void)

