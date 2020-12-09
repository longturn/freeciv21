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

#include "colors_common.h"

#include "gui_proto_constructor.h"

struct color;

GUI_FUNC_PROTO(QColor *, color_alloc, int r, int g, int b)
GUI_FUNC_PROTO(void, color_free, QColor *color)

GUI_FUNC_PROTO(int, color_brightness_score, QColor *color)
QColor *get_diag_color(int);


