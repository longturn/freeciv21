// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "unit.h"

#include <map>
#include <memory>
#include <queue>
#include <set>

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
  int health;     ///< How many HP the unit has left
  int fuel_left;  ///< How much fuel the unit has left

  bool comparable(const cost &other) const;

  bool operator==(const cost &other) const;
  bool operator<(const cost &other) const;
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
  unit *loaded;   ///< The unit we are loaded in
  bool moved;     ///< Whether we moved this turn (for HP recovery)

  // Cost of the path to come here, needed for path finding
  detail::cost cost; ///< How many turns it takes to get here

  // Ancestor information, needed to build a path. Invalid for the first
  // tile.
  vertex *parent;   ///< The previous vertex, if any
  unit_order order; ///< The order to come here

  vertex child_for_action(action_id action, const unit &probe,
                          const tile *target);

  bool comparable(const vertex &other) const;
  void fill_probe(unit &probe) const;

  bool operator==(const vertex &other) const;
  bool operator>(const vertex &other) const;
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

    void insert_initial_vertex();
    void maybe_insert_vertex(const detail::vertex &v);

    void attempt_move(detail::vertex &source);
    void attempt_full_mp(detail::vertex &source);
    void attempt_load(detail::vertex &source);
    void attempt_unload(detail::vertex &source);

    bool run_search(const tile *stopping_condition);
  };

public:
  explicit path_finder(const unit *unit);
  virtual ~path_finder();

  inline path_finder &operator=(path_finder &&other);

  void unit_changed(const unit &unit);

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
