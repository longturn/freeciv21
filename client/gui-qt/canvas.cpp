/**  / \                                                        ***********
    / _ \
   | / \ |                  Copyright (c) 1996-2020 Freeciv21 and
   ||   || _______    Freeciv contributors. This file is part of Freeciv21.
   ||   || |\     \    Freeciv21 is free software: you can redistribute it
   ||   || ||\     \               and/or modify it under the terms of the
   ||   || || \    |                       GNU  General Public License  as
   ||   || ||  \__/              published by the Free Software Foundation,
   ||   || ||   ||                         either version 3 of the License,
    \\_/ \_/ \_//                    or (at your option) any later version.
   /   _     _   \                  You should have received a copy of the
  /               \        GNU General Public License along with Freeciv21.
  |    O     O    |                               If not,
  |   \  ___  /   |             see https://www.gnu.org/licenses/.
 /     \ \_/ /     \
/  -----  |  --\    \
|     \__/|\__/ \   |                 FOLLOW THE WHITE RABBIT !
\       |_|_|       /
 \_____       _____/
       \     /                                                      *****/
#include "canvas.h"
#include <cmath>
// Qt
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include "client_main.h"
#include "mapview_common.h"
#include "tilespec.h"
// qt-client
#include "colors.h"
#include "colors_common.h"
#include "fc_client.h"
#include "fonts.h"
#include "qtg_cxxside.h"
#include "sprite.h"

/**
   Create a canvas of the given size.
 */
QPixmap *qtg_canvas_create(int width, int height)
{
  QPixmap *store = new QPixmap(width, height);
  return store;
}

/**
   Free any resources associated with this canvas and the canvas struct
   itself.
 */
void qtg_canvas_free(QPixmap *store) { delete store; }

/**
   Copies an area from the source canvas to the destination canvas.
 */
void qtg_canvas_copy(QPixmap *dest, const QPixmap *src, int src_x, int src_y,
                     int dest_x, int dest_y, int width, int height)
{
  QRectF source_rect(src_x, src_y, width, height);
  QRectF dest_rect(dest_x, dest_y, width, height);
  QPainter p;

  if (!width || !height) {
    return;
  }

  p.begin(dest);
  p.drawPixmap(dest_rect, *src, source_rect);
  p.end();
}

/**
   Copies an area from the source pixmap to the destination pixmap.
 */
void pixmap_copy(QPixmap *dest, const QPixmap *src, int src_x, int src_y,
                 int dest_x, int dest_y, int width, int height)
{
  QRectF source_rect(src_x, src_y, width, height);
  QRectF dest_rect(dest_x, dest_y, width, height);
  QPainter p;

  if (!width || !height) {
    return;
  }

  p.begin(dest);
  p.drawPixmap(dest_rect, *src, source_rect);
  p.end();
}

/**
   Copies an area from the source image to the destination image.
 */
void image_copy(QImage *dest, const QImage *src, int src_x, int src_y,
                int dest_x, int dest_y, int width, int height)
{
  QRectF source_rect(src_x, src_y, width, height);
  QRectF dest_rect(dest_x, dest_y, width, height);
  QPainter p;

  if (!width || !height) {
    return;
  }

  p.begin(dest);
  p.drawImage(dest_rect, *src, source_rect);
  p.end();
}

/**
   Draw some or all of a sprite onto the canvas.
 */
void qtg_canvas_put_sprite(QPixmap *pcanvas, int canvas_x, int canvas_y,
                           const QPixmap *sprite, int offset_x, int offset_y,
                           int width, int height)
{
  QPainter p;

  p.begin(pcanvas);
  p.drawPixmap(canvas_x, canvas_y, *sprite, offset_x, offset_y, width,
               height);
  p.end();
}

/**
   Draw a full sprite onto the canvas.
 */
void qtg_canvas_put_sprite_full(QPixmap *pcanvas, int canvas_x, int canvas_y,
                                const QPixmap *sprite)
{
  int width, height;

  get_sprite_dimensions(sprite, &width, &height);
  canvas_put_sprite(pcanvas, canvas_x, canvas_y, sprite, 0, 0, width,
                    height);
}

/**
   Draw a full sprite onto the canvas.  If "fog" is specified draw it with
   fog.
 */
void qtg_canvas_put_sprite_fogged(QPixmap *pcanvas, int canvas_x,
                                  int canvas_y, const QPixmap *psprite,
                                  bool fog, int fog_x, int fog_y)
{
  Q_UNUSED(fog_x)
  Q_UNUSED(fog_y)

  QPixmap temp(psprite->size());
  temp.fill(Qt::transparent);
  QPainter p(&temp);
  p.setCompositionMode(QPainter::CompositionMode_Source);
  p.drawPixmap(0, 0, *psprite);
  p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
  p.fillRect(temp.rect(), QColor(0, 0, 0, 110));
  p.end();

  p.begin(pcanvas);
  p.drawPixmap(canvas_x, canvas_y, temp);
  p.end();
}

/*****************************************************************************
   Draw fog outside city map when city is opened
 */
void qtg_canvas_put_sprite_citymode(QPixmap *pcanvas, int canvas_x,
                                    int canvas_y, const QPixmap *psprite,
                                    bool fog, int fog_x, int fog_y)
{
  Q_UNUSED(fog_x)
  Q_UNUSED(fog_y)
  QPainter p;

  p.begin(pcanvas);
  p.setCompositionMode(QPainter::CompositionMode_Difference);
  p.setOpacity(0.5);
  p.drawPixmap(canvas_x, canvas_y, *psprite);
  p.end();
}

/*****************************************************************************
   Put unit in city area when city dialog is open
 */
void canvas_put_unit_fogged(QPixmap *pcanvas, int canvas_x, int canvas_y,
                            const QPixmap *psprite, bool fog, int fog_x,
                            int fog_y)
{
  Q_UNUSED(fog_y)
  Q_UNUSED(fog_x)
  QPainter p;

  p.begin(pcanvas);
  p.setOpacity(0.7);
  p.drawPixmap(canvas_x, canvas_y, *psprite);
  p.end();
}
/**
   Draw a filled-in colored rectangle onto canvas.
 */
void qtg_canvas_put_rectangle(QPixmap *pcanvas, const QColor *pcolor,
                              int canvas_x, int canvas_y, int width,
                              int height)
{
  QBrush brush(*pcolor);
  QPen pen(*pcolor);
  QPainter p;

  p.begin(pcanvas);
  p.setPen(pen);
  p.setBrush(brush);
  if (width == 1 && height == 1) {
    p.drawPoint(canvas_x, canvas_y);
  } else if (width == 1) {
    p.drawLine(canvas_x, canvas_y, canvas_x, canvas_y + height - 1);
  } else if (height == 1) {
    p.drawLine(canvas_x, canvas_y, canvas_x + width - 1, canvas_y);
  } else {
    p.drawRect(canvas_x, canvas_y, width, height);
  }

  p.end();
}

/**
   Fill the area covered by the sprite with the given color.
 */
void qtg_canvas_fill_sprite_area(QPixmap *pcanvas, const QPixmap *psprite,
                                 const QColor *pcolor, int canvas_x,
                                 int canvas_y)
{
  int width, height;

  get_sprite_dimensions(psprite, &width, &height);
  qtg_canvas_put_rectangle(pcanvas, pcolor, canvas_x, canvas_y, width,
                           height);
}

/**
   Draw a 1-pixel-width colored line onto the canvas.
 */
void qtg_canvas_put_line(QPixmap *pcanvas, const QColor *pcolor,
                         enum line_type ltype, int start_x, int start_y,
                         int dx, int dy)
{
  QPen pen;
  QPainter p;

  pen.setColor(*pcolor);
  switch (ltype) {
  case LINE_NORMAL:
    pen.setWidth(1);
    break;
  case LINE_BORDER:
    pen.setStyle(Qt::DashLine);
    pen.setDashOffset(4);
    pen.setWidth(1);
    break;
  case LINE_TILE_FRAME:
    pen.setWidth(2);
    break;
  case LINE_GOTO:
    pen.setWidth(2);
    break;
  default:
    pen.setWidth(1);
    break;
  }

  p.begin(pcanvas);
  p.setPen(pen);
  p.setRenderHint(QPainter::Antialiasing);
  p.drawLine(start_x, start_y, start_x + dx, start_y + dy);
  p.end();
}

/**
   Draw a 1-pixel-width colored curved line onto the canvas.
 */
void qtg_canvas_put_curved_line(QPixmap *pcanvas, const QColor *pcolor,
                                enum line_type ltype, int start_x,
                                int start_y, int dx, int dy)
{
  QPen pen;
  pen.setColor(*pcolor);
  QPainter p;
  QPainterPath path;

  switch (ltype) {
  case LINE_NORMAL:
    pen.setWidth(1);
    break;
  case LINE_BORDER:
    pen.setStyle(Qt::DashLine);
    pen.setDashOffset(4);
    pen.setWidth(2);
    break;
  case LINE_TILE_FRAME:
    pen.setWidth(2);
    break;
  case LINE_GOTO:
    pen.setWidth(2);
    break;
  default:
    pen.setWidth(1);
    break;
  }

  p.begin(pcanvas);
  p.setRenderHints(QPainter::Antialiasing);
  p.setPen(pen);

  path.moveTo(start_x, start_y);
  path.cubicTo(start_x + dx / 2, start_y, start_x, start_y + dy / 2,
               start_x + dx, start_y + dy);
  p.drawPath(path);
  p.end();
}

/**
   Return the size of the given text in the given font.  This size should
   include the ascent and descent of the text.  Either of width or height
   may be NULL in which case those values simply shouldn't be filled out.
 */
void qtg_get_text_size(int *width, int *height, enum client_font font,
                       const QString &text)
{
  QFont afont = get_font(font);
  QScopedPointer<QFontMetrics> fm(new QFontMetrics(afont));

  if (width) {
    *width = fm->horizontalAdvance(text);
  }

  if (height) {
    *height = fm->height();
  }
}

/**
   Draw the text onto the canvas in the given color and font.  The canvas
   position does not account for the ascent of the text; this function must
   take care of this manually.  The text will not be NULL but may be empty.
 */
void qtg_canvas_put_text(QPixmap *pcanvas, int canvas_x, int canvas_y,
                         enum client_font font, const QColor *pcolor,
                         const QString &text)
{
  QPainter p;
  QPen pen;
  QFont afont = get_font(font);
  QScopedPointer<QFontMetrics> fm(new QFontMetrics(afont));

  pen.setColor(*pcolor);
  p.begin(pcanvas);
  p.setPen(pen);
  p.setFont(afont);
  p.drawText(canvas_x, canvas_y + fm->ascent(), text);
  p.end();
}

/**
   Returns given font
 */
QFont get_font(client_font font)
{
  QFont qf;

  switch (font) {
  case FONT_CITY_NAME:
    qf = fcFont::instance()->getFont(fonts::city_names, king()->map_scale);
    break;
  case FONT_CITY_PROD:
    qf = fcFont::instance()->getFont(fonts::city_productions,
                                     king()->map_scale);
    break;
  case FONT_REQTREE_TEXT:
    qf = fcFont::instance()->getFont(fonts::reqtree_text);
    break;
  case FONT_COUNT:
    break;
  }

  return qf;
}

/**
   Return rectangle containing pure image (crops transparency)
 */
QRect zealous_crop_rect(QImage &p)
{
  int r, t, b, l;

  l = p.width();
  r = 0;
  t = p.height();
  b = 0;
  for (int y = 0; y < p.height(); ++y) {
    QRgb *row = reinterpret_cast<QRgb *>(p.scanLine(y));
    bool row_filled = false;
    int x;

    for (x = 0; x < p.width(); ++x) {
      if (qAlpha(row[x])) {
        row_filled = true;
        r = qMax(r, x);
        if (l > x) {
          l = x;
          x = r;
        }
      }
    }
    if (row_filled) {
      t = qMin(t, y);
      b = y;
    }
  }
  return QRect(l, t, qMax(0, r - l + 1), qMax(0, b - t + 1));
}
