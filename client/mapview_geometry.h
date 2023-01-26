/*
 * SPDX-FileCopyrightText: 2022-2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QRect>

#include "tileset/layer.h"

struct edge;
struct tile;
struct tileset;

namespace freeciv {

class gui_rect_iterator {
public:
  gui_rect_iterator(const struct tileset *t, const QRect &rect);

  /// Checks whether the current iteration point has a corner.
  bool has_corner() const { return m_has_corner; }

  /// Checks whether the current iteration point has an edge.
  bool has_edge() const { return m_has_edge; }

  /// Checks whether the current iteration point has a non-null tile.
  bool has_tile() const { return m_has_tile; }

  /// Retrieves the current corner. Only valid if @ref has_corner is @c true.
  const tile_corner &corner() const { return m_corner; }

  /// Retrieves the current edge. Only valid if @ref has_edge is @c true.
  const tile_edge &edge() const { return m_edge; }

  /// Retrieves the current tile. Only valid if @ref has_tile is @c true.
  const ::tile *tile() const { return m_tile; }

  /// Retrieves the x position of the current item.
  int x() const { return m_xi * m_w / m_r2 - m_w / 2; }

  /// Retrieves the y position of the current item.
  int y() const { return m_yi * m_h / m_r2 - m_h / 2; }

  bool next();

private:
  bool m_isometric;
  int m_index, m_count;
  int m_r1, m_r2, m_w, m_h;
  int m_x0, m_y0, m_x1, m_y1;
  int m_xi, m_yi;

  tile_corner m_corner;
  tile_edge m_edge;
  ::tile *m_tile = nullptr;
  bool m_has_corner = false, m_has_edge = false, m_has_tile = false;
};

} // namespace freeciv
