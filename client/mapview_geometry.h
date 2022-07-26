/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QRect>

#include "layer.h"

struct edge;
struct tile;
struct tileset;

namespace freeciv {

class gui_rect_iterator {
public:
  /// The type of item currently being iterated over.
  enum class item_type {
    corner, ///< The meeting point of up to four tiles.
    edge,   ///< The edge between two tiles.
    tile,   ///< The center of a tile.
  };

  gui_rect_iterator(const struct tileset *t, const QRect &rect);

  /**
   * Retrieves the current corner. Only valid if @ref current_item is
   * @ref item_type::corner "corner".
   */
  const tile_corner &corner() const { return m_corner; }

  /**
   * Retrieves the current edge. Only valid if @ref current_item is
   * @ref item_type::edge "edge".
   */
  const tile_edge &edge() const { return m_edge; }

  /**
   * Retrieves the current tile. Only valid if @ref current_item is
   * @ref item_type::tile "tile".
   */
  const ::tile *tile() const { return m_tile; }

  /// Retrieves the x position of the current item.
  int x() const { return m_xi * m_w / m_r2 - m_w / 2; }

  /// Retrieves the y position of the current item.
  int y() const { return m_yi * m_h / m_r2 - m_h / 2; }

  bool next();

  /// Retrieves the type of the current item.
  item_type current_item() { return m_type; }

private:
  bool m_isometric;
  int m_index, m_count;
  int m_r1, m_r2, m_w, m_h;
  int m_x0, m_y0, m_x1, m_y1;
  int m_xi, m_yi;

  item_type m_type = item_type::tile; // Just set it to *something* initially
  tile_corner m_corner;
  tile_edge m_edge;
  ::tile *m_tile = nullptr;
};

} // namespace freeciv
