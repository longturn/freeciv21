/*
 * SPDX-FileCopyrightText: 1996-2020 Freeciv and Freeciv21 contributors
 * SPDX-FileCopyrightText: 2022-2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

/*
 * This file contains tileset geometry functions to manipulate the GUI for
 * the the main map view.
 */

#include "views/view_map_geometry.h"

#include "game.h"
#include "map.h"
#include "tileset/tilespec.h"

namespace freeciv {

/**
 * @class gui_rect_iterator
 * @brief Iterates over all map tiles that intersect with a rectangle in GUI
 *        coordinates.
 *
 * The order of iteration is guaranteed to satisfy the painter's algorithm.
 * The iteration covers not only tiles but tile edges and corners.
 *
 * Iteration is performed by repeatedly calling @ref next and querying the
 * current available items using @ref has_corner, @ref has_edge, or
 * @ref has_tile. If the item type matches your needs, you can retrieve the
 * @ref corner, @ref edge, or @ref tile using the corresponding functions.
 *
 * Typical usage is as follows:
 *
 * ```{.cpp}
 * for (auto it = gui_rect_iterator(tileset, rect); it.next(); ) {
 *   if (it.has_tile()) {
 *     auto tile = it.tile();
 *     // Do something
 *   }
 * }
 * ```
 *
 * One can also retrieve the canvas position of the current item using the
 * @ref x and @ref y functions.
 *
 * The order of iteration depends on the map topology. The basic idea is to
 * operate on a rectangular grid, which works pretty nicely when the topology
 * is neither isometric nor hexagonal:
 * ~~~
 *    |     |     |
 * t  e  t  e  t  e  t
 *    |     |     |
 * e--c--e--c--e--c--e--
 *    |     |     |
 * t  e  t  e  t  e  t
 *    |     |     |
 * e--c--e--c--e--c--e--
 *    |     |     |
 * t  e  t  e  t  e  t
 *    |     |     |
 * ~~~
 *
 * In the diagram above, "t" stands for @ref tile, "e" for @ref edge, and "c"
 * for @ref corner. Iteration takes place from left to right and from top to
 * bottom, i.e. in reading order.
 *
 * Things get more complicated for isometric grids, as some points on the
 * grid don't correspond to anything and need to be skipped. They are
 * represented with dots in the diagram below:
 * ~~~
 * t . c . t . c . t
 *    / \     / \
 * . e . e . e . e .
 *  /     \ /     \
 * c . t . c . t . c
 *  \     / \     /
 * . e . e . e . e .
 *    \ /     \ /
 * t . c . t . c . t
 *    / \     / \
 * . e . e . e . e .
 *  /     \ /     \
 * c . t . c . t . c
 *  \     / \     /
 * . e . e . e . e .
 *    \ /     \ /
 * t . c . t . c . t
 * ~~~
 *
 * For hexagonal grids, some points are corners and edges at the same time:
 * ~~~
 *   . --ce- .   t   . --ce- .
 *    /     \         /     \
 *   e   .   e   .   e   .   e
 *  /         \     /         \
 * - .   t   . --ce- .   t   . -
 *  \         /     \         /
 *   e   .   e   .   e   .   e
 *    \     /         \     /
 *   . --ce- .   t   . --ce- .
 *    /     \         /     \
 *   e   .   e   .   e   .   e
 *  /         \     /         \
 * - .   t   . --ce- .   t   . -
 *  \         /     \         /
 *   e   .   e   .   e   .   e
 *    \     /         \     /
 *   . --ce-  .  t   . --ce- .
 * ~~~
 *
 *
 * ~~~
 * t . c . t . c . t
 *    / \     / \
 * . e . e . e . e .
 *  /     \ /     \
 * |       |       |
 * ce. t . ce. t . ce
 * |       |       |
 *  \     / \     /
 * . e . e . e . e .
 *    \ /     \ /
 *     |       |
 * t . ce. t . ce. t
 *     |       |
 *    / \     / \
 * . e . e . e . e .
 *  /     \ /     \
 * |       |       |
 * ce. t . ce. t . ce
 * |       |       |
 *  \     / \     /
 * . e . e . e . e .
 *    \ /     \ /
 * t . c . t . c . t
 * ~~~
 *
 * During iteration, it may happen that some tiles are null because they are
 * not visible to the current player. In this case, @ref has_tile will return
 * @c false.
 */

/**
 * Constructor.
 */
gui_rect_iterator::gui_rect_iterator(const struct tileset *t,
                                     const QRect &rect)
{
  const auto normalized = rect.normalized();
  if (!normalized.isValid()) {
    m_index = 0;
    m_count = 0; // Don't iterate
    return;
  }

  m_isometric = tileset_is_isometric(t);

  m_r1 = m_isometric ? 2 : 1;
  m_r2 = m_r1 * 2; // double the ratio
  m_w = tileset_tile_width(t);
  m_h = tileset_tile_height(t);

  // Don't divide by r2 yet, to avoid integer rounding errors.
  m_x0 = DIVIDE(normalized.left() * m_r2, m_w) - m_r1 / 2;
  m_y0 = DIVIDE(normalized.top() * m_r2, m_h) - m_r1 / 2;
  m_x1 = DIVIDE(normalized.right() * m_r2 + m_w - 1, m_w) + m_r1;
  m_y1 = DIVIDE(normalized.bottom() * m_r2 + m_h - 1, m_h) + m_r1;

  // Start counting
  m_index = 0;
  m_count = (m_x1 - m_x0) * (m_y1 - m_y0);

  log_debug("Iterating over %d-%d x %d-%d rectangle.", m_x1, m_x0, m_y1,
            m_y0);
}

/**
 * Iterates to the next item. Returns `false` once the end of the iteration
 * has been reached.
 */
bool gui_rect_iterator::next()
{
  if (m_index >= m_count) {
    return false;
  }

  m_tile = nullptr;
  m_has_corner = false;
  m_has_edge = false;
  m_has_tile = false;

  m_xi = m_x0 + (m_index % (m_x1 - m_x0));
  m_yi = m_y0 + (m_index / (m_x1 - m_x0));

  const auto si = m_xi + m_yi;
  const auto di = m_yi - m_xi;

  if (m_isometric) {
    if ((m_xi + m_yi) % 2 != 0) {
      m_index++;
      return next();
    }

    if (m_xi % 2 == 0 && m_yi % 2 == 0) {
      if ((m_xi + m_yi) % 4 == 0) {
        // Tile
        m_tile = map_pos_to_tile(&(wld.map), si / 4 - 1, di / 4);
        m_has_tile = (m_tile != nullptr);
      } else {
        // Corner
        m_has_corner = true;
        m_corner.tile[0] =
            map_pos_to_tile(&(wld.map), (si - 6) / 4, (di - 2) / 4);
        m_corner.tile[1] =
            map_pos_to_tile(&(wld.map), (si - 2) / 4, (di - 2) / 4);
        m_corner.tile[2] =
            map_pos_to_tile(&(wld.map), (si - 2) / 4, (di + 2) / 4);
        m_corner.tile[3] =
            map_pos_to_tile(&(wld.map), (si - 6) / 4, (di + 2) / 4);
        if (tileset_hex_width(tileset) > 0) {
          m_has_edge = true;
          m_edge.type = EDGE_UD;
          m_edge.tile[0] = m_corner.tile[0];
          m_edge.tile[1] = m_corner.tile[2];
        } else if (tileset_hex_height(tileset) > 0) {
          m_has_edge = true;
          m_edge.type = EDGE_LR;
          m_edge.tile[0] = m_corner.tile[1];
          m_edge.tile[1] = m_corner.tile[3];
        }
      }
    } else {
      // Edge.
      m_has_edge = true;
      if (si % 4 == 0) {
        m_edge.type = EDGE_NS;
        m_edge.tile[0] = map_pos_to_tile(&(wld.map), (si - 4) / 4,
                                         (di - 2) / 4); // N
        m_edge.tile[1] = map_pos_to_tile(&(wld.map), (si - 4) / 4,
                                         (di + 2) / 4); // S
      } else {
        m_edge.type = EDGE_WE;
        m_edge.tile[0] = map_pos_to_tile(&(wld.map), (si - 6) / 4,
                                         di / 4); // W
        m_edge.tile[1] = map_pos_to_tile(&(wld.map), (si - 2) / 4,
                                         di / 4); // E
      }
    }
  } else {
    if (si % 2 == 0) {
      if (m_xi % 2 == 0) {
        // Corner.
        m_has_corner = true;
        m_corner.tile[0] = map_pos_to_tile(&(wld.map), m_xi / 2 - 1,
                                           m_yi / 2 - 1); // NW
        m_corner.tile[1] = map_pos_to_tile(&(wld.map), m_xi / 2,
                                           m_yi / 2 - 1); // NE
        m_corner.tile[2] = map_pos_to_tile(&(wld.map), m_xi / 2,
                                           m_yi / 2); // SE
        m_corner.tile[3] = map_pos_to_tile(&(wld.map), m_xi / 2 - 1,
                                           m_yi / 2); // SW
      } else {
        // Tile.
        m_tile = map_pos_to_tile(&(wld.map), (m_xi - 1) / 2, (m_yi - 1) / 2);
        m_has_tile = (m_tile != nullptr);
      }
    } else {
      // Edge.
      m_has_edge = true;
      if (m_yi % 2 == 0) {
        m_edge.type = EDGE_NS;
        m_edge.tile[0] = map_pos_to_tile(&(wld.map), (m_xi - 1) / 2,
                                         m_yi / 2 - 1); // N
        m_edge.tile[1] = map_pos_to_tile(&(wld.map), (m_xi - 1) / 2,
                                         m_yi / 2); // S
      } else {
        m_edge.type = EDGE_WE;
        m_edge.tile[0] = map_pos_to_tile(&(wld.map), m_xi / 2 - 1,
                                         (m_yi - 1) / 2); // W
        m_edge.tile[1] = map_pos_to_tile(&(wld.map), m_xi / 2,
                                         (m_yi - 1) / 2); // E
      }
    }
  }

  m_index++;
  return true;
}

} // namespace freeciv
