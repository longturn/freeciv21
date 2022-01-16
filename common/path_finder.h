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
  int moves_left; ///< How many move fragments the unit has left
  int fuel_left;  ///< How much fuel the unit has left

  /**
   * Returns `true` if the comparison with `other` would be unambiguous.
   */
  bool comparable(const cost &other) const
  {
    // When the results of the expressions below are positive, this cost does
    // better than the other for this criteria. When it's negative, it's the
    // opposite.
    auto a = other.turns - turns;
    auto b = moves_left - other.moves_left;
    auto c = fuel_left - other.fuel_left;
    // For the comparison to be meaningful, all criteria must go in the same
    // direction.
    return (a <= 0 && b <= 0 && c <= 0) || (a >= 0 && b >= 0 && c >= 0);
  }

  /**
   * Compares for equality.
   */
  bool operator==(const cost &other) const
  {
    return std::tie(turns, other.moves_left, other.fuel_left)
           == std::tie(other.turns, moves_left, fuel_left);
  }

  /**
   * Defines a strict ordering among costs.
   */
  bool operator<(const cost &other) const
  {
    return std::tie(turns, other.moves_left, other.fuel_left)
           < std::tie(other.turns, moves_left, fuel_left);
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
   * Is this vertex unambigously worse than `other` and should be dropped?
   * This function only makes sense if the `position` and `loaded` fields are
   * identical.
   */
  bool worse(const vertex &other) const
  {
    // Can't compare if not at the same location => can't say it's worse
    if (location != other.location)
      return false;

    // Identical isn't worse
    if (*this == other)
      return false;

    // Taking longer to arrive is almost always bad...
    if (cost.turns > other.cost.turns) {
      // ...except if we have more fuel as a result
      return cost.fuel_left <= other.cost.fuel_left;
    }

    // Having fewer MP left is almost always bad...
    if (cost.moves_left < other.cost.moves_left) {
      // ...except if we have more fuel as a result (but how would this
      // happen?)
      return cost.fuel_left <= other.cost.fuel_left;
    }

    // If we come here, we're no worse than `other` in number of turns or
    // moves left, so the only thing that remains is fuel. To be worse in
    // fuel means having less fuel left (note how the comparison is inverted
    // wrt above).
    return cost.fuel_left < other.cost.fuel_left;
  }

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
    // In most cases, a single vertex will be stored for a given tile. There
    // are situations, however, where more vertices are needed. This is for
    // instance the case for fueled units: a path in 5 turns leading to a
    // tile with 3 fuel left cannot be compared to a path in 1 turn leading
    // to the same tile with only one fuel left (since we don't know how much
    // fuel will be needed to reach the target). In such a case, the tile is
    // mapped to several vertices.
    std::multimap<const tile *, std::unique_ptr<detail::vertex>>
        best_vertices;
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
