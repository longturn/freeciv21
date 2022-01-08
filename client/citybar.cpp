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

#include <cmath>
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
  // +1 to be on the safe side and avoid wrapping
  m_blocks.back().base_size =
      QSizeF(metrics.horizontalAdvance(text) + 1, metrics.height());
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
    auto rect = blk.draw_rect.translated(top_left);
    switch (blk.mode) {
    case block::TEXT_MODE:
      if (blk.format.background() != Qt::transparent) {
        // Fill the background
        auto fill_rect = rect.marginsAdded(blk.margins);
        fill_rect.setY(top_left.y());
        fill_rect.setHeight(height());
        p.fillRect(fill_rect, blk.format.background());
      }
      p.setFont(blk.format.font());
      if (blk.shadow) {
        // Draw the shadow
        p.setPen(QPen(m_shadow_brush, 1));
        p.drawText(rect.translated(1, 1), blk.text);
      }
      p.setPen(QPen(blk.format.foreground(), 1));
      p.drawText(rect, blk.text);
      break;
    case block::ICON_MODE:
      p.drawPixmap(rect, *blk.icon, blk.icon->rect());
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
QStringList citybar_painter::available()
{
  // TRANS: City bar style
  return {N_("Simple"), N_("Traditional"), N_("Polished")};
}

/**
 * Returns the list of all available city bar styles. For compatibility with
 * the option code.
 */
const QVector<QString> *citybar_painter::available_vector(const option *)
{
  static QVector<QString> vector;
  if (vector.isEmpty()) {
    for (auto &name : available()) {
      vector << _(qUtf8Printable(name));
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
 * Returns the current painter (never null).
 */
citybar_painter *citybar_painter::current()
{
  if (!s_current) {
    set_current(gui_options.default_city_bar_style_name);
  }
  return s_current.get();
}

/**
 * Sets the current city bar style. The name should not be translated.
 * Returns true on success.
 */
void citybar_painter::set_current(const QString &name)
{
  if (name == QStringLiteral("Simple")) {
    s_current = std::make_unique<simple_citybar_painter>();
    return;
  } else if (name == QStringLiteral("Traditional")) {
    s_current = std::make_unique<traditional_citybar_painter>();
    return;
  } else if (name == QStringLiteral("Polished")) {
    s_current = std::make_unique<polished_citybar_painter>();
    return;
  } else if (available().contains(name)) {
    qCCritical(bugs_category,
               "Could not instantiate known city bar style %s",
               qUtf8Printable(name));
  } else {
    qCCritical(bugs_category, "Unknown city bar style %s",
               qUtf8Printable(name));
  }

  // Allocate the default to avoid crashes
  s_current = std::make_unique<polished_citybar_painter>();
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
    format.setFont(get_font(FONT_CITY_NAME));
    format.setForeground(*get_color(tileset, COLOR_MAPVIEW_CITYTEXT));
    first.add_text(name, format);

    static const QString en_space = QStringLiteral(" ");

    // Growth string (eg "5")
    if (gui_options.draw_city_growth && can_see_inside) {
      // Separator (assuming the em space is wider for the city name)
      first.add_text(en_space, format);

      // Text
      format.setFont(get_font(FONT_CITY_PROD));
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
      format.setFont(get_font(FONT_CITY_PROD));
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
    format.setFont(get_font(FONT_CITY_PROD));
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
  second.paint(painter,
               position + QPointF(-second_width / 2, first.height()));

  return QRect(position.x() - width / 2, position.y(), width,
               first.height() + second.height());
}

/**
 * Draws the traditional city bar with a dark background, two lines of text
 * and colored borders.
 */
QRect traditional_citybar_painter::paint(QPainter &painter,
                                         const QPointF &position,
                                         const city *pcity) const
{
  /*
   * We draw two lines of centered text under each other, below the requested
   * position: the first with the city name and the second with the
   * production.
   */

  // Decide what to do
  const bool can_see_inside =
      (client_is_global_observer() || city_owner(pcity) == client_player());
  const bool should_draw_productions =
      can_see_inside && gui_options.draw_city_productions;
  const bool should_draw_growth =
      can_see_inside && gui_options.draw_city_growth;
  const bool should_draw_trade_routes =
      can_see_inside && gui_options.draw_city_trade_routes;
  const bool should_draw_lower_bar = should_draw_productions
                                     || should_draw_growth
                                     || should_draw_trade_routes;

  if (!gui_options.draw_city_names && !should_draw_lower_bar) {
    // Nothing to draw.
    return QRect();
  }

  // Select the tileset to grab stuff from
  const auto t = (gui_options.zoom_scale_fonts || !unscaled_tileset)
                     ? tileset
                     : unscaled_tileset;

  // Get some city properties
  const citybar_sprites *citybar = get_citybar_sprites(t);

  char name[512], growth[32];
  color_std growth_color, production_color;
  get_city_mapview_name_and_growth(pcity, name, sizeof(name), growth,
                                   sizeof(growth), &growth_color,
                                   &production_color);
  const QMargins text_margins(3, 0, 3, 0);
  QColor owner_color = *get_player_color(t, city_owner(pcity));

  // This is used for both lines
  QTextCharFormat format;

  // Fill the lines
  line_of_text first, second;

  // First line
  if (gui_options.draw_city_names) {
    // Flag
    first.add_icon(get_city_flag_sprite(t, pcity));

    // Units in city
    if (can_player_see_units_in_city(client.conn.playing, pcity)) {
      unsigned long count = unit_list_size(pcity->tile->units);
      count =
          qBound(0UL, count,
                 static_cast<unsigned long>(citybar->occupancy.size - 1));
      first.add_icon(citybar->occupancy.p[count]);
    } else {
      if (pcity->client.occupied) {
        first.add_icon(citybar->occupied);
      } else {
        first.add_icon(citybar->occupancy.p[0]);
      }
    }

    // City name
    format.setForeground(*get_color(t, COLOR_MAPVIEW_CITYTEXT));
    first.add_spacer(); // Center it
    first.add_text(name, format, false, text_margins);
    first.add_spacer();

    // City size (on colored background)
    format.setFont(get_font(FONT_CITY_NAME));
    format.setBackground(owner_color);

    // Try to pick a color for city size text that contrasts with player
    // color
    QColor *textcolors[2] = {get_color(t, COLOR_MAPVIEW_CITYTEXT),
                             get_color(t, COLOR_MAPVIEW_CITYTEXT_DARK)};
    format.setForeground(*color_best_contrast(&owner_color, textcolors,
                                              ARRAY_SIZE(textcolors)));

    first.add_text(QStringLiteral("%1").arg(city_size_get(pcity)), format,
                   false, text_margins);

    format.setBackground(Qt::transparent); // Reset
  }

  // Second line
  if (should_draw_lower_bar) {
    // All items share the same font
    format.setFont(get_font(FONT_CITY_PROD));

    if (should_draw_productions) {
      // Icon
      second.add_icon(citybar->shields);

      // Text
      char prod[512];
      get_city_mapview_production(pcity, prod, sizeof(prod));

      format.setForeground(*get_color(t, production_color));
      second.add_text(prod, format, false, text_margins);
    }

    // Flush production to the left and the rest to the right
    second.add_spacer();

    if (should_draw_growth) {
      // Icon
      second.add_icon(citybar->food);

      // Text
      format.setForeground(*get_color(t, growth_color));
      second.add_text(growth, format, false, text_margins);
    }

    if (should_draw_trade_routes) {
      // Icon
      second.add_icon(citybar->trade);

      // Text
      char trade_routes[32];
      color_std trade_routes_color;
      get_city_mapview_trade_routes(
          pcity, trade_routes, sizeof(trade_routes), &trade_routes_color);

      format.setForeground(*get_color(t, trade_routes_color));
      second.add_text(trade_routes, format, false, text_margins);
    }
  }

  // Do the text layout. We need to align everything to integer pixels to
  // avoid surprises with the rounding (but we still assume integer heights).
  double width =
      std::ceil(std::max(first.ideal_width(), second.ideal_width()));
  first.do_layout(width);
  second.do_layout(width);

  double x = std::floor(position.x() - width / 2 - 1);
  double y = std::floor(position.y());

  int num_lines = (should_draw_lower_bar ? 2 : 1);
  QRectF bounds =
      QRectF(x, y, width + 2, first.height() + num_lines + second.height());

  // Paint the background, frame and separator
  painter.drawTiledPixmap(bounds, *citybar->background);
  painter.setPen(owner_color);
  painter.drawRect(bounds);
  if (gui_options.draw_city_names && should_draw_lower_bar) {
    painter.drawLine(bounds.topLeft() + QPointF(0, first.height() + 1),
                     bounds.topRight() + QPointF(0, first.height() + 1));
  }

  // Draw text and icons on top
  first.paint(painter, QPointF(x + 1, y + 1)); // +1 for the frame
  second.paint(painter, QPointF(x + 1, y + 1 + num_lines + first.height()));

  return QRect(x, y, x + width + 2, bounds.height());
}

/**
 * Draws the "polished" city bar. It uses a single line and semitransparent
 * colored background.
 */
QRect polished_citybar_painter::paint(QPainter &painter,
                                      const QPointF &position,
                                      const city *pcity) const
{
  /**
   * Most of the info is on a single line. The only exception is trade.
   */

  // Decide what to do
  const bool can_see_inside =
      (client_is_global_observer() || city_owner(pcity) == client_player());
  const bool should_draw_productions =
      can_see_inside && gui_options.draw_city_productions;
  const bool should_draw_growth =
      can_see_inside && gui_options.draw_city_growth;
  const bool should_draw_trade_routes =
      can_see_inside && gui_options.draw_city_trade_routes;

  // Select the tileset to grab stuff from
  const auto t = (gui_options.zoom_scale_fonts || !unscaled_tileset)
                     ? tileset
                     : unscaled_tileset;

  // Get some city properties
  const citybar_sprites *citybar = get_citybar_sprites(t);

  char name[512], growth[32];
  color_std growth_color, production_color;
  get_city_mapview_name_and_growth(pcity, name, sizeof(name), growth,
                                   sizeof(growth), &growth_color,
                                   &production_color);

  // Decide colors
  QColor owner_color = *get_player_color(t, city_owner(pcity));
  QColor *textcolors[2] = {get_color(t, COLOR_MAPVIEW_CITYTEXT),
                           get_color(t, COLOR_MAPVIEW_CITYTEXT_DARK)};
  QColor text_color = pcity->owner == client_player()
                          ? *get_color(t, COLOR_MAPVIEW_CITYTEXT)
                          : *color_best_contrast(&owner_color, textcolors,
                                                 ARRAY_SIZE(textcolors));

  // Decide on the target height. It's the max of the font sizes and the
  // occupied indicator (we assume all indicators have the same size).
  // It's used to scale the flag, progress bars and production.
  double target_height = citybar->occupancy.p[0]->height();
  target_height = std::max(target_height,
                           QFontMetricsF(get_font(FONT_CITY_NAME)).height());
  target_height = std::max(target_height,
                           QFontMetricsF(get_font(FONT_CITY_PROD)).height());

  // Build the contents
  line_of_text line;

  QMargins text_margins(3, 0, 3, 0);
  QTextCharFormat format;

  // Size
  format.setFont(get_font(FONT_CITY_NAME));
  format.setForeground(text_color);
  line.add_text(QString::number(pcity->size), format, false, text_margins);

  // Growth
  std::unique_ptr<QPixmap> growth_progress;
  if (should_draw_growth) {
    // Progress bar
    growth_progress = std::make_unique<QPixmap>(
        QSize(6, target_height) * painter.device()->devicePixelRatio());
    growth_progress->setDevicePixelRatio(
        painter.device()->devicePixelRatio());
    growth_progress->fill(Qt::black);

    QPainter p(growth_progress.get());

    // Avoid div by 0
    int granary_max = std::max(1, city_granary_size(city_size_get(pcity)));
    double current = double(pcity->food_stock) / granary_max;
    double next_turn = qBound(
        0.0, current + double(pcity->surplus[O_FOOD]) / granary_max, 1.0);

    current *= target_height;
    next_turn *= target_height;

    // Surplus (yellow)
    if (next_turn > current) {
      p.fillRect(QRectF(0, target_height, 6, -next_turn),
                 QColor(200, 200, 60));
    }

    // Stock (green)
    p.fillRect(QRectF(0, target_height, 6, -current), QColor(70, 120, 50));

    // Negative surplus (red)
    if (next_turn < current) {
      p.fillRect(QRectF(0, target_height - current, 6, current - next_turn),
                 Qt::red);
    }

    // Add it
    line.add_icon(growth_progress.get());

    // Text
    format.setFont(get_font(FONT_CITY_PROD));
    format.setFontPointSize(format.fontPointSize() / 1.5);
    format.setForeground(*get_color(t, growth_color));
    line.add_text(growth, format, false, text_margins);
  }

  // Flag
  std::unique_ptr<QPixmap> scaled_flag;
  if (city_owner(pcity) != client_player()) {
    scaled_flag = std::make_unique<QPixmap>(
        get_city_flag_sprite(t, pcity)->scaledToHeight(
            target_height, Qt::SmoothTransformation));
    line.add_icon(scaled_flag.get());
  }

  // Occupied indicator
  if (can_player_see_units_in_city(client.conn.playing, pcity)) {
    unsigned long count = unit_list_size(pcity->tile->units);
    count = qBound(0UL, count,
                   static_cast<unsigned long>(citybar->occupancy.size - 1));
    line.add_icon(citybar->occupancy.p[count]);
  } else {
    if (pcity->client.occupied) {
      line.add_icon(citybar->occupied);
    } else {
      line.add_icon(citybar->occupancy.p[0]);
    }
  }

  // Name
  if (gui_options.draw_city_names) {
    format.setFont(get_font(FONT_CITY_NAME));
    format.setForeground(text_color);
    line.add_text(name, format, false, text_margins);
  }

  // Production
  std::unique_ptr<QPixmap> production_pix;
  std::unique_ptr<QPixmap> production_progress;
  if (should_draw_productions) {
    // Format
    format.setFont(get_font(FONT_CITY_PROD));
    format.setFontPointSize(format.fontPointSize() / 1.5);
    format.setForeground(*get_color(t, production_color));

    QMargins prod_margins = text_margins;
    prod_margins.setLeft(0); // Already have the city name on the left

    // Text
    int turns = city_production_turns_to_build(pcity, true);
    if (turns < 1000) {
      line.add_text(QString::number(turns), format, false, text_margins);
    } else {
      line.add_text(QStringLiteral("∞"), format, false, text_margins);
    }

    // Progress bar
    production_progress = std::make_unique<QPixmap>(
        QSize(6, target_height) * painter.device()->devicePixelRatio());
    production_progress->setDevicePixelRatio(
        painter.device()->devicePixelRatio());

    QPainter p(production_progress.get());

    // Draw
    if (pcity->surplus[O_SHIELD] < 0) {
      production_progress->fill(Qt::red);
    } else {
      production_progress->fill(Qt::black);
    }
    int total = universal_build_shield_cost(pcity, &pcity->production);
    double current = double(pcity->shield_stock) / total;
    double next_turn =
        qBound(0.0, current + double(pcity->surplus[O_SHIELD]) / total, 1.0);

    current *= target_height;
    next_turn *= target_height;

    // Surplus (yellow)
    if (next_turn > current) {
      p.fillRect(QRectF(0, target_height, 6, -next_turn),
                 QColor(200, 200, 60));
    }

    // Stock (blue)
    p.fillRect(QRectF(0, target_height, 6, -current), Qt::blue);

    // Negative surplus (red)
    if (next_turn < current) {
      p.fillRect(QRectF(0, target_height - current, 6, current - next_turn),
                 Qt::red);
    }

    // Add it
    line.add_icon(production_progress.get());

    // Icon
    const QPixmap *xsprite = nullptr;
    const auto &target = pcity->production;
    if (can_see_inside && (VUT_UTYPE == target.kind)) {
      xsprite =
          get_unittype_sprite(t, target.value.utype, direction8_invalid());
    } else if (can_see_inside && (target.kind == VUT_IMPROVEMENT)) {
      xsprite = get_building_sprite(t, target.value.building);
    }
    if (xsprite) {
      production_pix = std::make_unique<QPixmap>(
          xsprite->scaledToHeight(target_height, Qt::SmoothTransformation));
      line.add_icon(production_pix.get());
    }
  }

  // Lay out and draw the first line
  line.do_layout();

  double width = std::ceil(line.size().width());
  double height = line.height() + 2;
  double x = std::floor(position.x() - width / 2 - 1);
  double y = std::floor(position.y());

  // Draw the background
  if (city_owner(pcity) == client_player()) {
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QColor(0, 0, 0, 100));
  } else {
    owner_color.setAlpha(90);
    painter.setPen(QPen(owner_color, 1));
    painter.setBrush(QBrush(owner_color));
  }
  painter.drawRoundedRect(QRectF(x, y, width + 2, height - 1), 7, 7);

  // Draw the text
  line.paint(painter, QPointF(x + 1, y + 1));

  // Trade line
  if (should_draw_trade_routes) {
    line_of_text trade_line;

    // Icon
    trade_line.add_icon(citybar->trade);

    // Text
    char trade_routes[32];
    color_std trade_routes_color;
    get_city_mapview_trade_routes(pcity, trade_routes, sizeof(trade_routes),
                                  &trade_routes_color);

    format.setFont(get_font(FONT_CITY_PROD));
    format.setForeground(*get_color(t, trade_routes_color));
    trade_line.add_text(trade_routes, format, false, text_margins);

    // Lay it out
    trade_line.do_layout();

    double trade_width = std::ceil(trade_line.size().width());
    double trade_x = std::floor(position.x() - trade_width / 2 - 1);
    double trade_y = y + height;

    // Draw the background
    if (city_owner(pcity) == client_player()) {
      painter.setPen(QPen(Qt::black, 1));
      painter.setBrush(QColor(0, 0, 0, 100));
    } else {
      owner_color.setAlpha(90);
      painter.setPen(QPen(owner_color, 1));
      painter.setBrush(QBrush(owner_color));
    }
    painter.drawRoundedRect(QRectF(trade_x, y + line.height() + 2,
                                   trade_width + 2, trade_line.height() + 1),
                            7, 7);

    // Draw the text
    trade_line.paint(painter, QPointF(trade_x + 1, trade_y + 1));

    x = std::min(x, trade_x);
    width = std::max(width, trade_width);
    height += trade_line.height() + 2;
  }

  return QRect(x, y, std::ceil(width + 2), std::ceil(height));
}
