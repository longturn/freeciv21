/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "city_icon_widget.h"

// client
#include "game.h"
#include "tileset/layer_city.h"
#include "tileset/tilespec.h"

#include <QPainter>

namespace freeciv {

/**
 * \class city_icon_widget
 *
 * Displays an icon representing a city. No more functionality.
 */

/**
 * Constructor
 */
city_icon_widget::city_icon_widget(QWidget *parent)
{
  setAutoFillBackground(true);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

/**
 * Changes the city displayed by this widget
 */
void city_icon_widget::set_city(int city_id)
{
  if (city_id != m_city) {
    m_city = city_id;
    update();
  }
}

/**
 * Reimplemented to allow for tiny tileset.
 */
QSize city_icon_widget::minimumSizeHint() const { return QSize(0, 0); }

/**
 * Reimplemented to pick the size from the tileset.
 */
QSize city_icon_widget::sizeHint() const
{
  auto size =
      std::max(tileset_tile_width(tileset), tileset_tile_height(tileset));

  if (auto city = game_city_by_number(m_city); city) {
    auto sprs =
        tileset_layer_city(tileset)->fill_sprite_array_no_flag(city, false);
    const auto bounds = sprite_array_bounds(sprs);
    size = std::max(bounds.width(), bounds.height());
  }

  return QSize(size, size); // Square
}

/**
 * Reimplemented to draw the widget.
 */
void city_icon_widget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  auto city = game_city_by_number(m_city);
  if (!city) {
    return;
  }

  // We don't show the occupied flag as it looks bad with amplio
  auto sprs =
      tileset_layer_city(tileset)->fill_sprite_array_no_flag(city, false);

  // Center the sprites
  auto bounds = sprite_array_bounds(sprs);
  const auto origin = bounds.topLeft();
  bounds.moveCenter(QPoint(width() / 2, height() / 2));

  QPainter p(this);
  p.translate(bounds.topLeft() - origin);

  for (const auto sprite : sprs) {
    // Floating-point API for DPI scaling.
    p.drawPixmap(QPointF(sprite.offset), *sprite.sprite);
  }
}

/**
 * Reimplemented to handle tileset changes.
 */
bool city_icon_widget::event(QEvent *event)
{
  if (event->type() == TilesetChanged) {
    updateGeometry();
    update();
    return true;
  }
  return QWidget::event(event);
}

} // namespace freeciv
