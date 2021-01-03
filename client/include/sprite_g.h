/**********************************************************************
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#pragma once

#include "gui_proto_constructor.h"
#include "support.h"

class QPixmap; /* opaque type, real type is gui-dep */
struct color;

GUI_FUNC_PROTO(const char **, gfx_fileextensions, void)

GUI_FUNC_PROTO(QPixmap *, load_gfxfile, const char *filename)
GUI_FUNC_PROTO(QPixmap *, crop_sprite, QPixmap *source, int x, int y,
               int width, int height, QPixmap *mask, int mask_offset_x,
               int mask_offset_y, float scale, bool smooth)
GUI_FUNC_PROTO(QPixmap *, create_sprite, int width, int height,
               QColor *pcolor)
GUI_FUNC_PROTO(void, get_sprite_dimensions, QPixmap *sprite, int *width,
               int *height)
GUI_FUNC_PROTO(void, free_sprite, QPixmap *s)
