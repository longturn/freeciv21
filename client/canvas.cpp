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
// Qt
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

// qt-client
#include "colors.h"
#include "colors_common.h"
#include "fc_client.h"
#include "fonts.h"
#include "sprite.h"

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
   Returns given font
 */
QFont get_font(client_font font)
{
  QFont qf;

  switch (font) {
  case FONT_CITY_NAME:
    qf = fcFont::instance()->getFont(fonts::city_names, 1);
    break;
  case FONT_CITY_PROD:
    qf = fcFont::instance()->getFont(fonts::city_productions, 1);
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
