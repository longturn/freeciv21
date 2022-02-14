/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

#include "gui_proto_constructor.h"
#include "support.h"

class QColor;
class QPixmap;

GUI_FUNC_PROTO(QPixmap *, load_gfxfile, const char *filename)
GUI_FUNC_PROTO(QPixmap *, crop_sprite, const QPixmap *source, int x, int y,
               int width, int height, const QPixmap *mask, int mask_offset_x,
               int mask_offset_y)
GUI_FUNC_PROTO(void, free_sprite, QPixmap *s)
