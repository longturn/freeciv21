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

#include "gui_proto_constructor.h"

struct tile_list;

GUI_FUNC_PROTO(void, editgui_tileset_changed, void)
GUI_FUNC_PROTO(void, editgui_refresh, void)
GUI_FUNC_PROTO(void, editgui_popup_properties, const struct tile_list *tiles,
               int objtype)
GUI_FUNC_PROTO(void, editgui_popdown_all, void)

/* OBJTYPE_* enum values defined in client/editor.h */
GUI_FUNC_PROTO(void, editgui_notify_object_changed, int objtype, int id,
               bool removal)
GUI_FUNC_PROTO(void, editgui_notify_object_created, int tag, int id)


