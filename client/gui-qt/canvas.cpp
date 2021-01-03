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
#include <math.h>
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

static QFont *get_font(enum client_font font);
/************************************************************************/ /**
   Create a canvas of the given size.
 ****************************************************************************/
QPixmap *qtg_canvas_create(int width, int height)
{
  QPixmap *store = new QPixmap(width, height);
  return store;
}

/************************************************************************/ /**
   Free any resources associated with this canvas and the canvas struct
   itself.
 ****************************************************************************/
void qtg_canvas_free(QPixmap *store) { delete store; }

/************************************************************************/ /**
   Copies an area from the source canvas to the destination canvas.
 ****************************************************************************/
void qtg_canvas_copy(QPixmap *dest, QPixmap *src, int src_x, int src_y,
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

/************************************************************************/ /**
   Copies an area from the source pixmap to the destination pixmap.
 ****************************************************************************/
void pixmap_copy(QPixmap *dest, QPixmap *src, int src_x, int src_y,
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

/************************************************************************/ /**
   Copies an area from the source image to the destination image.
 ****************************************************************************/
void image_copy(QImage *dest, QImage *src, int src_x, int src_y, int dest_x,
                int dest_y, int width, int height)
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

/************************************************************************/ /**
   Draw some or all of a sprite onto the canvas.
 ****************************************************************************/
void qtg_canvas_put_sprite(QPixmap *pcanvas, int canvas_x, int canvas_y,
                           QPixmap *sprite, int offset_x, int offset_y,
                           int width, int height)
{
  QPainter p;

  p.begin(pcanvas);
  p.drawPixmap(canvas_x, canvas_y, *sprite, offset_x, offset_y, width,
               height);
  p.end();
}

/************************************************************************/ /**
   Draw a full sprite onto the canvas.
 ****************************************************************************/
void qtg_canvas_put_sprite_full(QPixmap *pcanvas, int canvas_x, int canvas_y,
                                QPixmap *sprite)
{
  int width, height;

  get_sprite_dimensions(sprite, &width, &height);
  canvas_put_sprite(pcanvas, canvas_x, canvas_y, sprite, 0, 0, width,
                    height);
}

/************************************************************************/ /**
   Draw a full sprite onto the canvas.  If "fog" is specified draw it with
   fog.
 ****************************************************************************/
void qtg_canvas_put_sprite_fogged(QPixmap *pcanvas, int canvas_x,
                                  int canvas_y, QPixmap *psprite, bool fog,
                                  int fog_x, int fog_y)
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
 ****************************************************************************/
void qtg_canvas_put_sprite_citymode(QPixmap *pcanvas, int canvas_x,
                                    int canvas_y, QPixmap *psprite, bool fog,
                                    int fog_x, int fog_y)
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
 ****************************************************************************/
void canvas_put_unit_fogged(QPixmap *pcanvas, int canvas_x, int canvas_y,
                            QPixmap *psprite, bool fog, int fog_x, int fog_y)
{
  Q_UNUSED(fog_y)
  Q_UNUSED(fog_x)
  QPainter p;

  p.begin(pcanvas);
  p.setOpacity(0.7);
  p.drawPixmap(canvas_x, canvas_y, *psprite);
  p.end();
}
/************************************************************************/ /**
   Draw a filled-in colored rectangle onto canvas.
 ****************************************************************************/
void qtg_canvas_put_rectangle(QPixmap *pcanvas, QColor *pcolor, int canvas_x,
                              int canvas_y, int width, int height)
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

/************************************************************************/ /**
   Fill the area covered by the sprite with the given color.
 ****************************************************************************/
void qtg_canvas_fill_sprite_area(QPixmap *pcanvas, QPixmap *psprite,
                                 QColor *pcolor, int canvas_x, int canvas_y)
{
  int width, height;

  get_sprite_dimensions(psprite, &width, &height);
  qtg_canvas_put_rectangle(pcanvas, pcolor, canvas_x, canvas_y, width,
                           height);
}

/************************************************************************/ /**
   Draw a 1-pixel-width colored line onto the canvas.
 ****************************************************************************/
void qtg_canvas_put_line(QPixmap *pcanvas, QColor *pcolor,
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

/************************************************************************/ /**
   Draw a 1-pixel-width colored curved line onto the canvas.
 ****************************************************************************/
void qtg_canvas_put_curved_line(QPixmap *pcanvas, QColor *pcolor,
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

/************************************************************************/ /**
   Return the size of the given text in the given font.  This size should
   include the ascent and descent of the text.  Either of width or height
   may be NULL in which case those values simply shouldn't be filled out.
 ****************************************************************************/
void qtg_get_text_size(int *width, int *height, enum client_font font,
                       const QString &text)
{
  QFont *afont;
  QFontMetrics *fm;

  afont = get_font(font);
  fm = new QFontMetrics(*afont);
  if (width) {
    *width = fm->horizontalAdvance(text);
  }

  if (height) {
    *height = fm->height();
  }
  delete fm;
}

/************************************************************************/ /**
   Draw the text onto the canvas in the given color and font.  The canvas
   position does not account for the ascent of the text; this function must
   take care of this manually.  The text will not be NULL but may be empty.
 ****************************************************************************/
void qtg_canvas_put_text(QPixmap *pcanvas, int canvas_x, int canvas_y,
                         enum client_font font, QColor *pcolor,
                         const QString &text)
{
  QPainter p;
  QPen pen;
  QFont *afont;
  QFontMetrics *fm;

  afont = get_font(font);
  pen.setColor(*pcolor);
  fm = new QFontMetrics(*afont);

  p.begin(pcanvas);
  p.setPen(pen);
  p.setFont(*afont);
  p.drawText(canvas_x, canvas_y + fm->ascent(), text);
  p.end();
  delete fm;
}

/************************************************************************/ /**
   Returns given font
 ****************************************************************************/
QFont *get_font(client_font font)
{
  QFont *qf;
  int ssize;

  switch (font) {
  case FONT_CITY_NAME:
    qf = fcFont::instance()->getFont(fonts::city_names);
    if (king()->map_scale != 1.0f && king()->map_font_scale) {
      ssize = ceil(king()->map_scale * fcFont::instance()->city_fontsize);
      if (qf->pointSize() != ssize) {
        qf->setPointSize(ssize);
      }
    }
    break;
  case FONT_CITY_PROD:
    qf = fcFont::instance()->getFont(fonts::city_productions);
    if (king()->map_scale != 1.0f && king()->map_font_scale) {
      ssize = ceil(king()->map_scale * fcFont::instance()->prod_fontsize);
      if (qf->pointSize() != ssize) {
        qf->setPointSize(ssize);
      }
    }
    break;
  case FONT_REQTREE_TEXT:
    qf = fcFont::instance()->getFont(fonts::reqtree_text);
    break;
  case FONT_COUNT:
    qf = NULL;
    break;
  default:
    qf = NULL;
    break;
  }
  return qf;
}

/************************************************************************/ /**
   Return rectangle containing pure image (crops transparency)
 ****************************************************************************/
QRect zealous_crop_rect(QImage &p)
{
  int r, t, b, l;

  l = p.width();
  r = 0;
  t = p.height();
  b = 0;
  for (int y = 0; y < p.height(); ++y) {
    QRgb *row = (QRgb *) p.scanLine(y);
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

void draw_full_city_bar(struct city *pcity, QPixmap *pcanvas, int x, int y,
                        int *ref_width, int *ref_height)
{
  QBrush blackBrush, brush, grow2Brush, growBrush, redBrush, prod2Brush,
      prodBrush, ownerBrush;
  QColor *owner_color = get_player_color(tileset, city_owner(pcity));
  owner_color->setAlpha(90);
  QColor *textcolors[2] = {get_color(tileset, COLOR_MAPVIEW_CITYTEXT),
                           get_color(tileset, COLOR_MAPVIEW_CITYTEXT_DARK)};
  QColor *pcolor =
      color_best_contrast(owner_color, textcolors, ARRAY_SIZE(textcolors));
  QFont *afont;
  QFontMetrics *fm;
  QPainter p;
  QPen blackPen, grow2Pen, growPen, pen, prod2Pen, prodPen, ownerPen, redPen;
  QPixmap flagPix;
  int fonttext_height, cWidth;

  x = x + tileset_tile_width(tileset) / 2;
  y = y + tileset_citybar_offset_y(tileset);

  afont = get_font(FONT_CITY_NAME);
  blackBrush = QBrush(Qt::SolidPattern);
  blackBrush.setColor(Qt::black);
  blackPen.setColor(Qt::black);
  brush = QBrush(Qt::SolidPattern);
  brush.setColor(*pcolor);
  fm = new QFontMetrics(*afont);
  grow2Brush = QBrush(Qt::SolidPattern);
  grow2Brush.setColor(QColor(200, 200, 60));
  grow2Pen.setColor(QColor(200, 200, 60));
  growBrush = QBrush(Qt::SolidPattern);
  growBrush.setColor(QColor(70, 120, 50));
  growPen.setColor(QColor(70, 120, 50));
  ownerBrush = QBrush(Qt::SolidPattern);
  ownerBrush.setColor(*owner_color);
  ownerPen.setColor(QColor(*owner_color));
  pen.setColor(QColor(*pcolor));
  prod2Brush = QBrush(Qt::SolidPattern);
  prod2Brush.setColor(QColor(200, 200, 60));
  prod2Pen.setColor(QColor(200, 200, 60));
  prodBrush = QBrush(Qt::SolidPattern);
  prodBrush.setColor(Qt::blue);
  prodPen.setColor(Qt::blue);
  redPen.setColor(Qt::red);
  redBrush = QBrush(Qt::SolidPattern);
  redBrush.setColor(Qt::red);

  if (city_owner(pcity) == client_player()) {
    ownerPen.setColor(QColor(0, 0, 0, 100));
    ownerBrush.setColor(QColor(0, 0, 0, 100));
    pen.setColor(QColor(255, 255, 255));
  }

  QString text = pcity->name;
  QPixmap *flag = get_city_flag_sprite(tileset, pcity);
  fonttext_height = fm->ascent();
  QString city_size = QString::number(pcity->size);
  const struct citybar_sprites *citybar = get_citybar_sprites(tileset);
  QPixmap *occupy = nullptr;
  QPixmap occupyPix;

  if (can_player_see_units_in_city(client.conn.playing, pcity)) {
    int count = unit_list_size(pcity->tile->units);

    count = CLIP(0, count, citybar->occupancy.size - 1);
    occupy = citybar->occupancy.p[count];
  } else {
    if (pcity->client.occupied) {
      occupy = citybar->occupied;
    } else {
      occupy = citybar->occupancy.p[0];
    }
  }
  const bool can_see =
      (client_is_global_observer() || city_owner(pcity) == client_player());

  QString growth_time;
  int granary_max = 0;

  struct universal *target;
  target = &pcity->production;
  QPixmap *xsprite = nullptr;
  if (can_see && (VUT_UTYPE == target->kind)) {
    xsprite = get_unittype_sprite(get_tileset(), target->value.utype,
                                  direction8_invalid());
  } else if (can_see && (target->kind == VUT_IMPROVEMENT)) {
    xsprite = get_building_sprite(get_tileset(), target->value.building);
  }
  QPixmap prodPix;
  if (xsprite) {
    prodPix =
        xsprite->scaledToHeight(fonttext_height, Qt::SmoothTransformation);
  } else {
    prodPix = QPixmap(1, 1);
  }

  flagPix =
      (*flag).scaledToHeight(fonttext_height, Qt::SmoothTransformation);
  occupyPix = (*occupy).scaledToHeight((fonttext_height * 3) / 2,
                                       Qt::SmoothTransformation);

  // count width
  int draw_width;
  draw_width = fm->horizontalAdvance(city_size) + occupyPix.width()
               + flagPix.width() + fm->horizontalAdvance(text) + 2 + 2;

  if (can_see) {
    draw_width = draw_width + 6 + 6 + prodPix.width();
    if (city_owner(pcity) == client_player()) {
      draw_width -= flagPix.width();
    }
  }
  *ref_width = draw_width + 10;
  *ref_height = fm->height() * 3;

  int mid = x;
  x = x - draw_width / 2;

  // draw
  // we draw using negative values to 3 pixels add first so add 3 pixels
  x = x + 3;
  y = y + 3;

  p.begin(pcanvas);

  p.setPen(ownerPen);
  p.setBrush(ownerBrush);
  p.drawRoundedRect(x - 3, y - 3, draw_width + 3, fonttext_height + 3, 7, 7);

  if (can_see && gui_options.draw_city_trade_routes) {
    char trade_routes[32];
    enum color_std trade_routes_color = COLOR_MAPVIEW_CITYTEXT;
    get_city_mapview_trade_routes(pcity, trade_routes, sizeof(trade_routes),
                                  &trade_routes_color);

    QString trade_text = QString(trade_routes);
    mid = mid - fm->horizontalAdvance(trade_text) / 2;
    QPixmap tradePix = citybar->trade->scaledToHeight(fonttext_height);

    p.drawRoundedRect(mid - 3, y + fonttext_height,
                      tradePix.width() + fm->horizontalAdvance(trade_text),
                      fonttext_height + 3, 4, 4);
    p.drawPixmap(mid, y + fonttext_height + 3, tradePix);
    p.setPen(*get_color(tileset, trade_routes_color));
    p.drawText(mid + tradePix.width(), y + 2 * fonttext_height - 2,
               trade_text);
  }

  p.setPen(pen);
  p.setBrush(brush);
  p.setFont(*afont);

  // city size
  p.drawText(x, y + fm->ascent() - 4, city_size);
  cWidth = fm->horizontalAdvance(city_size);
  x = x + cWidth + 2;

  if (can_see) { // draw grow later bc it overlaps occupy
    x = x + 6;
  }
  // occupy
  int half_miss_height = (occupyPix.height() - fonttext_height) / 2;
  p.drawPixmap(x, y - half_miss_height, occupyPix);
  cWidth = occupyPix.width();

  // grow
  if (can_see) {
    x = x - 6;
    int gtime = city_turns_to_grow(pcity);
    if (gtime < 1000) {
      growth_time = QString::number(gtime);
    } else {
      growth_time = QStringLiteral("∞");
    }
    // avoid div by 0
    granary_max = qMax(1, city_granary_size(city_size_get(pcity)));

    int hstock = (fonttext_height * pcity->food_stock) / granary_max;
    int hhstock = (fonttext_height * pcity->surplus[O_FOOD]) / granary_max;

    p.setPen(blackPen);
    p.setBrush(blackBrush);
    int miss_stock = fonttext_height - hstock - hhstock;
    if (miss_stock > 0) {
      p.drawRect(x, y, 6, miss_stock);
    } else {
      miss_stock = 0;
    }
    // surplus
    if (hhstock > 0) {
      p.setPen(grow2Pen);
      p.setBrush(grow2Brush);
      p.drawRect(x, y + miss_stock, 6, hhstock);
    }

    // food in stock
    int max_height = qMin(fonttext_height, miss_stock + hhstock);
    p.setPen(growPen);
    p.setBrush(growBrush);
    p.drawRect(x, y + max_height, 6, hstock);

    if (hhstock < 0) {
      p.setPen(redPen);
      p.setBrush(redBrush);
      p.drawRect(x, y + fonttext_height - hstock, 6, -hhstock);
    }

    // reset pens
    p.setPen(ownerPen);
    p.setBrush(ownerBrush);

    // draw number of turns for city to grow
    int font_size;
    QFont font;
    font = p.font();
    font_size = font.pointSize();
    font.setPointSize((font_size * 2) / 3);
    p.setFont(font);
    p.setPen(pen);
    p.drawText(x, y + fonttext_height + fm->ascent() / 3, growth_time);
    font.setPointSize(font_size);
    p.setFont(font);
    p.setBrush(brush);
    x = x + 6;
  }
  x = x + cWidth;
  // flag
  if (city_owner(pcity) != client_player()) {
    p.drawPixmap(x, y, flagPix);
    cWidth = flagPix.width();
    x = x + cWidth;
  }

  // city name
  p.drawText(x, y + fm->ascent() - 4, text);
  cWidth = fm->horizontalAdvance(text);
  fonttext_height = fm->ascent();
  x = x + cWidth + 2;

  if (can_see) {
    QString prod_time;
    int prod_max;

    int ptime = city_production_turns_to_build(pcity, true);
    if (pcity->surplus[O_SHIELD] < 0) {
      prodPix.fill(Qt::red);
    }
    if (ptime < 1000) {
      prod_time = QString::number(ptime);
    } else {
      prod_time = QStringLiteral("∞");
    }
    prod_max = universal_build_shield_cost(pcity, &pcity->production);

    int hstock = (fonttext_height * pcity->shield_stock) / prod_max;
    hstock = qMin(fonttext_height, hstock);
    int hhstock = (fonttext_height * pcity->surplus[O_SHIELD]) / prod_max;

    p.setPen(blackPen);
    p.setBrush(blackBrush);
    int miss_stock = fonttext_height - hstock - hhstock;
    if (miss_stock > 0) {
      p.drawRect(x, y, 6, miss_stock);
    } else {
      miss_stock = 0;
    }
    // surplus prod
    hhstock = qMax(0, hhstock); // it shouldn't be lower probably
    hhstock = qMin(fonttext_height - hstock, hhstock);
    p.setPen(prod2Pen);
    p.setBrush(prod2Brush);
    p.drawRect(x, y + miss_stock, 6, hhstock);

    // produced by now
    p.setPen(prodPen);
    p.setBrush(prodBrush);
    p.drawRect(x, y + miss_stock + hhstock, 6, hstock);

    x = x + 6;

    p.setBrush(QColor(200, 200, 200));
    p.drawRect(x, y, prodPix.width(), prodPix.height());
    p.drawPixmap(x, y, prodPix);

    x = x - 10;
    // draw number of turns to finish prod
    int font_size;
    QFont font;
    font = p.font();
    font_size = font.pointSize();
    font.setPointSize((font_size * 2) / 3);
    p.setFont(font);
    p.setPen(pen);
    p.drawText(x, y + fonttext_height + fm->ascent() / 3, prod_time);
    font.setPointSize(font_size);
    p.setFont(font);
    p.setBrush(brush);
  }

  p.end();
  delete fm;
}