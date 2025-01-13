/*
 * SPDX-FileCopyrightText: Freeciv21 contributors
 * SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QPoint>

#include <vector>

class QPixmap;
class QRect;

struct tileset;

struct drawn_sprite {
  explicit drawn_sprite(const struct tileset *ts, const QPixmap *sprite,
                        bool foggable = true, int offset_x = 0,
                        int offset_y = 0);
  explicit drawn_sprite(const struct tileset *ts, const QPixmap *sprite,
                        bool foggable, const QPoint &offset);
  drawn_sprite(const drawn_sprite &other) = default;
  drawn_sprite(drawn_sprite &&other) = default;
  drawn_sprite &operator=(const drawn_sprite &other) = default;
  drawn_sprite &operator=(drawn_sprite &&other) = default;

  const QPixmap *sprite;
  bool foggable; ///< Set to FALSE for sprites that are never fogged.
  QPoint offset; ///< offset from tile origin
};

QRect sprite_array_bounds(const std::vector<drawn_sprite> &sprs);
