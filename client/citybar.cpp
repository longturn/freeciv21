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

#include <algorithm>
#include <vector>

// Qt
#include <QPainter>
#include <QTextCharFormat>

// utility
#include "bugs.h"
#include "fcintl.h"
#include "log.h"

// common
#include "city.h"

// client
#include "canvas_g.h"
#include "client_main.h"
#include "colors_common.h"
#include "mapview_common.h"

#include "citybar.h"

/// Pointer to the city bar painter currently in use.
std::unique_ptr<citybar_painter> citybar_painter::s_current = nullptr;

/**
 * Helper class to create a line of text. It's a lot like a dumbed down
 * version of the Qt rich text engine, but handles a few things that we
 * need and are not easy in Qt.
 *
 * A line of text is a series of `blocks`. Each block contains either an
 * icon (QPixmap) or a string and its attributes. In addition, "spacer"
 * blocks can be added to add a stretchable space between items.
 */
class line_of_text {
  struct block {
    enum { TEXT_MODE, ICON_MODE, SPACER_MODE } mode; // What's in this block
    QString text;                                    // Text only
    bool shadow = true;                              // Text only
    QTextCharFormat format;                          // Text only
    double ascent = 0, descent = 0;                  // Text only
    const QPixmap *icon = nullptr;                   // Icon only
    QMargins margins;                                // All modes
    QSizeF base_size; // The base size of the block without margins

    QRectF draw_rect; // The laid out rect
  };

public:
  void add_spacer();
  void add_icon(const QPixmap *icon, const QMargins &margins = QMargins());
  void add_text(const QString &text, const QTextCharFormat &format,
                bool shadow = true, const QMargins &margins = QMargins());

  void set_text_shadow_brush(const QBrush &brush) { m_shadow_brush = brush; }

  double ideal_width() const;
  void do_layout(double width = 0);
  double height() const { return size().height(); }
  QSizeF size() const { return m_size; }
  void paint(QPainter &p, const QPointF &top_left) const;

private:
  QBrush m_shadow_brush;

  QSizeF m_size;
  std::vector<block> m_blocks;
};

/**
 * Adds a spacer to the line. Spacers have zero width by default and expand
 * as needed to fill the available space.
 */
void line_of_text::add_spacer()
{
  m_blocks.emplace_back();
  m_blocks.back().mode = block::SPACER_MODE;
}

/**
 * Adds an icon to the line. It will be centered vertically.
 */
void line_of_text::add_icon(const QPixmap *icon, const QMargins &margins)
{
  m_blocks.emplace_back();
  m_blocks.back().mode = block::ICON_MODE;
  m_blocks.back().icon = icon;
  m_blocks.back().margins = margins;
  m_blocks.back().base_size = icon->size();
}

/**
 * Adds text to the line with the given format. Text items are aligned on
 * their baseline and centered vertically.
 */
void line_of_text::add_text(const QString &text,
                            const QTextCharFormat &format, bool shadow,
                            const QMargins &margins)
{
  m_blocks.emplace_back();
  m_blocks.back().mode = block::TEXT_MODE;
  m_blocks.back().text = text;
  m_blocks.back().format = format;
  m_blocks.back().shadow = shadow;
  m_blocks.back().margins = margins;

  QFontMetricsF metrics(format.font());
  m_blocks.back().base_size =
      QSizeF(metrics.horizontalAdvance(text), metrics.height());
  m_blocks.back().ascent = metrics.ascent();
  m_blocks.back().descent = metrics.descent();
  if (shadow) {
    // Add some space for the shadow
    m_blocks.back().base_size += QSizeF(1, 1);
    // Bake it into the margin
    m_blocks.back().margins.setBottom(margins.bottom() + 1);
  }
}

/**
 * Returns the ideal line width (the sum of the width of all blocks)
 */
double line_of_text::ideal_width() const
{
  double width = 0;
  for (const auto &blk : m_blocks) {
    width +=
        blk.margins.left() + blk.base_size.width() + blk.margins.right();
  }
  return width;
}

/**
 * Lays out the line to fill the given width. Additional space is added
 * between central blocks as required to fill the available space. If the
 * width is zero (the default), no spacing is added between items.
 *
 * Do not specify a width between zero and width().
 */
void line_of_text::do_layout(double width)
{
  // Compute the amount of horizontal space left
  double empty_width;

  if (width <= 0) {
    width = this->ideal_width();
    empty_width = 0;
  } else {
    empty_width = width - this->ideal_width();
  }

  // Set the widths and horizontal positions. Margins are counted outside of
  // the rectangle.

  // Calculate the spacer block width
  long num_spacers =
      std::count_if(m_blocks.begin(), m_blocks.end(), [](const block &blk) {
        return blk.mode == block::SPACER_MODE;
      });
  double spacer = empty_width / std::max(num_spacers, 1L);

  // Set the geometry
  double x = 0;
  for (auto &blk : m_blocks) {
    blk.draw_rect.setX(x + blk.margins.left());
    if (blk.mode == block::SPACER_MODE) {
      x += spacer;
    } else {
      blk.draw_rect.setWidth(blk.base_size.width());
      x += blk.margins.left() + blk.base_size.width() + blk.margins.right();
    }
  }

  // We're done with the horizontal layout. Set the width already.
  m_size.setWidth(x);

  // Now do the vertical layout. The tricky bit here is to set the text
  // position correctly so the baselines align nicely. We calculate the max
  // ascent, descent and total height (for icons), then combine them as
  // needed.
  double ascent = 0, descent = 0, height = 0;
  for (const auto &blk : m_blocks) {
    ascent = std::max(ascent, blk.ascent + blk.margins.top());
    descent = std::max(descent, blk.descent + blk.margins.bottom());
    height = std::max(height, blk.margins.top() + blk.base_size.height()
                                  + blk.margins.bottom());
  }

  // The ascent might come from one block and the descent from another, in
  // which case they're not summed in `height` yet.
  height = std::max(height, ascent + descent);
  m_size.setHeight(height);

  // Text baseline position with respect to the top of the bounding rect
  double baseline = (height + ascent - descent) / 2;

  // Set y and height
  for (auto &blk : m_blocks) {
    if (blk.mode == block::TEXT_MODE) {
      // Align text on the baseline
      blk.draw_rect.setY(baseline - blk.ascent);
    } else {
      // Center the rest vertically
      blk.draw_rect.setY((height - blk.base_size.height()) / 2);
    }
    blk.draw_rect.setHeight(blk.base_size.height());
  }
}

/**
 * Paints the line at the given position. The state of the painter is
 * altered.
 */
void line_of_text::paint(QPainter &p, const QPointF &top_left) const
{
  for (const auto &blk : m_blocks) {
    switch (blk.mode) {
    case block::TEXT_MODE:
      p.setFont(blk.format.font());
      if (blk.shadow) {
        // Draw the shadow
        p.setPen(QPen(m_shadow_brush, 1));
        p.drawText(blk.draw_rect.translated(top_left + QPointF(1, 1)),
                   blk.text);
      }
      p.setPen(QPen(blk.format.foreground(), 1));
      p.drawText(blk.draw_rect.translated(top_left), blk.text);
      break;
    case block::ICON_MODE:
      p.drawPixmap(blk.draw_rect.translated(top_left), *blk.icon,
                   blk.icon->rect());
      break;
    case block::SPACER_MODE:
      continue;
    }
  }
}

/**
 * Returns the list of all available city bar styles. The strings are not
 * translated.
 */
QStringList citybar_painter::available() { return {N_("Simple")}; }

/**
 * Returns the list of all available city bar styles. For compatibility with
 * the option code.
 */
const QVector<QString> *citybar_painter::available_vector(const option *)
{
  static QVector<QString> vector;
  if (vector.isEmpty()) {
    for (auto &name : available()) {
      vector << _(qPrintable(name));
    }
  }
  return &vector;
}

/**
 * Called by the option code when the option has changed. Sets the current
 * painter and refreshes the map.
 */
void citybar_painter::option_changed(option *opt)
{
  set_current(option_str_get(opt));
  update_map_canvas_visible();
}

/**
 * Sets the current city bar style. The name should not be translated.
 * Returns true on success.
 */
bool citybar_painter::set_current(const QString &name)
{
  fc_assert_ret_val(available().contains(name), false);

  if (name == QStringLiteral("Simple")) {
    s_current = std::make_unique<simple_citybar_painter>();
    return true;
  }

  qCCritical(bugs_category, "Could not instantiate known city bar style %s",
             qPrintable(name));
  return false;
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
  line_of_text first, second;

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

  // Enable shadows
  auto dark_color = *get_color(tileset, COLOR_MAPVIEW_CITYTEXT_DARK);
  first.set_text_shadow_brush(dark_color);
  second.set_text_shadow_brush(dark_color);

  // First line
  if (gui_options.draw_city_names) {
    // City name
    format.setFont(*get_font(FONT_CITY_NAME));
    format.setForeground(*get_color(tileset, COLOR_MAPVIEW_CITYTEXT));
    first.add_text(name, format);

    static const QString en_space = "\342\200\200";

    // Growth string (eg "5")
    if (gui_options.draw_city_growth && can_see_inside) {
      // Separator (assuming the em space is wider for the city name)
      first.add_text(en_space, format);

      // Text
      format.setFont(*get_font(FONT_CITY_PROD));
      format.setForeground(*get_color(tileset, growth_color));
      first.add_text(growth, format);
    }

    // Trade routes (eg "3/4")
    if (gui_options.draw_city_trade_routes && can_see_inside) {
      // Separator (can still be the city name format)
      first.add_text(en_space, format);

      // Get the text
      char trade_routes[32];
      color_std trade_routes_color;
      get_city_mapview_trade_routes(
          pcity, trade_routes, sizeof(trade_routes), &trade_routes_color);

      // Add it
      format.setFont(*get_font(FONT_CITY_PROD));
      format.setForeground(*get_color(tileset, trade_routes_color));
      first.add_text(en_space + trade_routes, format);
    }
  }

  // Second line
  if (gui_options.draw_city_productions && can_see_inside) {
    // Get text
    char prod[512];
    get_city_mapview_production(pcity, prod, sizeof(prod));

    // Add text
    format.setFont(*get_font(FONT_CITY_PROD));
    format.setForeground(*get_color(tileset, production_color));
    second.add_text(prod, format);
  }

  // Do the text layout
  double first_width = first.ideal_width();
  double second_width = second.ideal_width();

  double width = std::max(first_width, second_width);
  first.do_layout(width);
  second.do_layout(width);

  // Paint
  first.paint(painter, position - QPointF(first_width / 2, 0));
  second.paint(painter, position + QPointF(-second_width / 2,
                                           first.size().height()));

  return QRect(position.x() - width / 2, position.y(), width,
               first.size().height() + second.size().height());
}
