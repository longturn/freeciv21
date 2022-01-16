// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "unit.h"

#include <map>
#include <memory>
#include <queue>

struct tile;

namespace freeciv {

class path;

namespace detail {
/**
 * The "cost" of a path is the amount of effort needed to reach it. Two costs
 * can be compared to find out which is the best.
 */
struct cost {
  int turns;      ///< How many turns it takes to get there
  int moves_left; ///< How many move fragments the units has left

  /**
   * Compares for equality.
   */
  bool operator==(const cost &other) const
  {
    return std::tie(turns, other.moves_left)
           == std::tie(other.turns, moves_left);
  }

  /**
   * Defines a strict ordering among costs.
   */
  bool operator<(const cost &other) const
  {
    return std::tie(turns, other.moves_left)
           < std::tie(other.turns, moves_left);
  }

  /**
   * Defines a weak ordering among costs.
   */
  bool operator<=(const cost &other) const
  {
    return std::tie(turns, other.moves_left)
           <= std::tie(other.turns, moves_left);
  }
};

/**
 * A vertex in the path-finding graph. It primarily corresponds to a tile,
 * but sometimes a tile has several vertices (for instance, a unit may want
 * to wait in a city to avoid ending its turn in the open).
 *
 * Additional information about the path finding is also stored.
 */
struct vertex {
  // Vertex location on the map
  tile *location; ///< Where we are

  // Cost of the path to come here, needed for path finding
  detail::cost cost; ///< How many turns it takes to get here

  // Ancestor information, needed to build a path. Invalid for the first
  // tile.
  vertex *parent;   ///< The previous vertex, if any
  unit_order order; ///< The order to come here

  /**
   * Equality comparator.
   */
  bool operator==(const vertex &other) const
  {
    return std::tie(location, cost) == std::tie(other.location, other.cost);
  }

  /**
   * Defines an ordering for the priority queue.
   */
  bool operator>(const vertex &other) const { return other.cost < cost; }
};
} // namespace detail

/**
 * A path is a succession of moves and actions to go from one location to
 * another.
 */
class path_finder {
  class path_finder_private {
  public:
    explicit path_finder_private(const ::unit *unit);
    ~path_finder_private() = default;

    const ::unit unit; ///< A unit used to probe whether moves are valid.

    // Storage for Dijkstra's algorithm.
    std::map<const tile *, std::unique_ptr<detail::vertex>> best_vertices;
    std::priority_queue<detail::vertex, std::vector<detail::vertex>,
                        std::greater<>>
        queue;

    void maybe_insert_vertex(const detail::vertex &v);
  };

public:
  explicit path_finder(const unit *unit);
  virtual ~path_finder();

  inline path_finder &operator=(path_finder &&other);

  path find_path(const tile *destination);

private:
  std::unique_ptr<path_finder_private> m_d;
};

/**
 * Move-assignment operator.
 */
path_finder &path_finder::operator=(path_finder &&other)
{
  m_d = std::move(other.m_d);
  return *this;
}

} // namespace freeciv
