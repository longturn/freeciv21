/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "colorizer.h"

#include <QBrush>
#include <QPainter>

namespace freeciv {

/**
 * @class colorizer Swaps colors in QPixmap
 *
 * Starting from a base pixmap, this class generates new pixmaps with one
 * color replaced (for instance, all pink pixels changed to green).
 */

/**
 * Creates a colorizer that will replace every pixel of the given hue.
 * Passing a negative hue disables colorization.
 */
colorizer::colorizer(const QPixmap &base, int hue_to_replace,
                     QObject *parent)
    : QObject(parent), m_base(base), m_base_image(base.toImage()),
      m_hue_to_replace(hue_to_replace)
{
}

/**
 * Returns a pixmap with some pixels changed to the target color. The pixmap
 * is cached for later use.
 */
const QPixmap *colorizer::pixmap(const QColor &color) const
{
  // Easy cases with nothing to do
  if (m_hue_to_replace < 0 || !color.isValid()) {
    return &m_base;
  }

  // Draw it if we don't have it yet
  if (m_cache.count(color.rgba()) == 0) {
    auto new_hue = color.hslHue();
    auto image = m_base_image.copy();

    // Iterate through pixels and replace the hue
    for (int x = 0; x < image.width(); ++x) {
      for (int y = 0; y < image.height(); ++y) {
        auto pixel = image.pixelColor(x, y);
        if (pixel.hslHue() == m_hue_to_replace) {
          image.setPixelColor(x, y,
                              QColor::fromHsl(new_hue, pixel.hslSaturation(),
                                              pixel.lightness()));
        }
      }
    }

    m_cache[color.rgba()] = QPixmap::fromImage(image);
  }

  return &m_cache[color.rgba()];
}

} // namespace freeciv
