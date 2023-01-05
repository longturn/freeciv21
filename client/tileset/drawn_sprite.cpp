/*
 * SPDX-FileCopyrightText: Copyright (c) 2021 Freeciv21 contributors
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "drawn_sprite.h"

#include "tilespec.h"

#include <QPixmap>
#include <QRect>

/**
 * Instantiates a drawn sprite, ensuring that it's never null.
 */
drawn_sprite::drawn_sprite(const struct tileset *ts, const QPixmap *sprite,
                           bool foggable, int offset_x, int offset_y)
    : sprite(sprite), foggable(foggable && tileset_use_hard_coded_fog(ts)),
      offset_x(offset_x), offset_y(offset_y)
{
  fc_assert(sprite);
}

/**
 * Calculates the bounding rectangle of the given sprite array.
 */
QRect sprite_array_bounds(const std::vector<drawn_sprite> &sprs)
{
  QRect bounds;
  for (const auto &sprite : sprs) {
    bounds |= QRect(QPoint(sprite.offset_x, sprite.offset_y),
                    sprite.sprite->size());
  }
  return bounds;
}
