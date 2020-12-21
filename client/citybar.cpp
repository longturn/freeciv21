/**************************************************************************
 Copyright (c) 1996-2020 Freeciv and Freeciv21 contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QPainter>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

// common
#include "city.h"

// client
#include "canvas_g.h"
#include "client_main.h"
#include "colors_common.h"
#include "mapview_common.h"

#include "citybar.h"

/**
 * Constructor
 */
simple_citybar_painter::simple_citybar_painter()
    : m_document(new QTextDocument), m_dark_document(new QTextDocument)
{
}

/**
 * Destructor
 */
simple_citybar_painter::~simple_citybar_painter()
{
  delete m_document;
  delete m_dark_document;
}

/**
 * Draws a simple city bar.
 */
QRect simple_citybar_painter::paint(QPainter &painter,
                                    const QPointF &position,
                                    const city *pcity) const
{
  /*
   * We draw two lines of centered text under each other, below the requested
   * position: the first with the city name and the second with the
   * production.
   */

  // We use the heavy QTextDocument machinery to avoid dealing with offsets
  // by hand.
  m_document->clear();
  m_dark_document->clear();
  QTextCursor cursor(m_document);
  QTextCursor dark_cursor(m_dark_document);

  // Prepare the cursors
  QTextBlockFormat block_format;
  block_format.setAlignment(Qt::AlignHCenter);
  cursor.setBlockFormat(block_format);
  dark_cursor.setBlockFormat(block_format);

  // Get some city properties
  const bool can_see_inside =
      (client_is_global_observer() || city_owner(pcity) == client_player());

  char name[512], growth[32];
  color_std growth_color, production_color;
  get_city_mapview_name_and_growth(pcity, name, sizeof(name), growth,
                                   sizeof(growth), &growth_color,
                                   &production_color);

  // This is used for both lines
  QTextCharFormat format;
  QTextCharFormat dark_format;
  dark_format.setForeground(
      *get_color(tileset, COLOR_MAPVIEW_CITYTEXT_DARK));

  // First line
  if (gui_options.draw_city_names) {
    // City name
    format.setFont(*get_font(FONT_CITY_NAME));
    format.setForeground(*get_color(tileset, COLOR_MAPVIEW_CITYTEXT));
    cursor.insertText(name, format);

    dark_format.setFont(*get_font(FONT_CITY_NAME));
    dark_cursor.insertText(name, dark_format);

    static const QString en_space = "\342\200\200";

    // Growth string (eg "5")
    if (gui_options.draw_city_growth && can_see_inside) {
      // Separator (assuming the em space is wider for the city name)
      cursor.insertText(en_space, format);
      dark_cursor.insertText(en_space, dark_format);

      // Text
      format.setFont(*get_font(FONT_CITY_PROD));
      format.setForeground(*get_color(tileset, growth_color));
      cursor.insertText(growth, format);

      dark_format.setFont(*get_font(FONT_CITY_PROD));
      dark_cursor.insertText(growth, dark_format);
    }

    // Trade routes (eg "3/4")
    if (gui_options.draw_city_trade_routes && can_see_inside) {
      // Separator
      cursor.insertText(en_space, format);

      // Get the text
      char trade_routes[32];
      color_std trade_routes_color;
      get_city_mapview_trade_routes(
          pcity, trade_routes, sizeof(trade_routes), &trade_routes_color);

      // Add it
      format.setFont(*get_font(FONT_CITY_PROD));
      format.setForeground(*get_color(tileset, trade_routes_color));
      cursor.insertText(trade_routes, format);

      dark_format.setFont(*get_font(FONT_CITY_PROD));
      dark_cursor.insertText(trade_routes, dark_format);
    }
  }

  // Line separator if needed
  if (gui_options.draw_city_names && gui_options.draw_city_productions
      && can_see_inside) {
    cursor.insertBlock();
    dark_cursor.insertBlock();
  }

  // Second line
  if (gui_options.draw_city_productions && can_see_inside) {
    // Get text
    char prod[512];
    get_city_mapview_production(pcity, prod, sizeof(prod));

    // Add text
    format.setFont(*get_font(FONT_CITY_PROD));
    format.setForeground(*get_color(tileset, production_color));
    cursor.insertText(prod, format);

    dark_format.setFont(*get_font(FONT_CITY_PROD));
    dark_cursor.insertText(prod, dark_format);
  }

  // Do the text layout
  m_document->adjustSize();
  m_dark_document->adjustSize();

  // Paint
  int half_width = std::ceil((m_document->size().width() + 1) / 2);

  painter.save();
  painter.translate(position + QPointF(-half_width + 1, -1));
  m_dark_document->drawContents(&painter);
  painter.translate(-1.0, -1.0);
  m_document->drawContents(&painter);
  painter.restore();

  int height = 1 + std::ceil(m_document->size().height());
  return QRect(position.x() - half_width, position.y(), 2 * half_width,
               height);
}
