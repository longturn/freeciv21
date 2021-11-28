/***********************************************************************
Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

#include "support.h" // bool type

#include "gui_proto_constructor.h"

class QColor;
class QFont;
class QPixmap; // opaque type, real type is gui-dep
class QString;

enum line_type { LINE_NORMAL, LINE_BORDER, LINE_TILE_FRAME, LINE_GOTO };

// Creator and destructor
GUI_FUNC_PROTO(QPixmap *, canvas_create, int width, int height)
GUI_FUNC_PROTO(void, canvas_free, QPixmap *store)

// Drawing functions
GUI_FUNC_PROTO(void, canvas_copy, QPixmap *dest, const QPixmap *src,
               int src_x, int src_y, int dest_x, int dest_y, int width,
               int height)
GUI_FUNC_PROTO(void, canvas_put_sprite, QPixmap *pcanvas, int canvas_x,
               int canvas_y, const QPixmap *sprite, int offset_x,
               int offset_y, int width, int height);
GUI_FUNC_PROTO(void, canvas_put_sprite_full, QPixmap *pcanvas, int canvas_x,
               int canvas_y, const QPixmap *sprite)
GUI_FUNC_PROTO(void, canvas_put_sprite_fogged, QPixmap *pcanvas,
               int canvas_x, int canvas_y, const QPixmap *psprite, bool fog,
               int fog_x, int fog_y)
GUI_FUNC_PROTO(void, canvas_put_sprite_citymode, QPixmap *pcanvas,
               int canvas_x, int canvas_y, const QPixmap *psprite, bool fog,
               int fog_x, int fog_y)
GUI_FUNC_PROTO(void, canvas_put_rectangle, QPixmap *pcanvas,
               const QColor *pcolor, int canvas_x, int canvas_y, int width,
               int height)
GUI_FUNC_PROTO(void, canvas_fill_sprite_area, QPixmap *pcanvas,
               QPixmap *psprite, const QColor *pcolor, int canvas_x,
               int canvas_y)
GUI_FUNC_PROTO(void, canvas_put_line, QPixmap *pcanvas, const QColor *pcolor,
               enum line_type ltype, int start_x, int start_y, int dx,
               int dy)
GUI_FUNC_PROTO(void, canvas_put_curved_line, QPixmap *pcanvas,
               const QColor *pcolor, enum line_type ltype, int start_x,
               int start_y, int dx, int dy)
void canvas_put_unit_fogged(QPixmap *pcanvas, int canvas_x, int canvas_y,
                            const QPixmap *psprite, bool fog, int fog_x,
                            int fog_y);
// Text drawing functions
enum client_font {
  FONT_CITY_NAME,
  FONT_CITY_PROD,
  FONT_REQTREE_TEXT,
  FONT_COUNT
};
GUI_FUNC_PROTO(void, get_text_size, int *width, int *height,
               enum client_font font, const QString &text)
GUI_FUNC_PROTO(void, canvas_put_text, QPixmap *pcanvas, int canvas_x,
               int canvas_y, enum client_font font, const QColor *pcolor,
               const QString &text)

QFont get_font(enum client_font font);
