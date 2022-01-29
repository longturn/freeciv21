// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "unit.h"

#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>

struct tile;

namespace freeciv {

class path;

namespace detail {

/**
 * A vertex in the path-finding graph. It primarily corresponds to a tile,
 * but sometimes a tile has several vertices (for instance, a unit may want
 * to wait in a city to avoid ending its turn in the open).
 *
 * Additional information about the path finding is also stored.
 */
struct vertex {
  // Vertex location in the search graph
  tile *location;   ///< Where we are
  unit *loaded;     ///< The unit we are loaded in
  bool moved;       ///< Whether we moved this turn (for HP recovery)
  bool paradropped; ///< Whether we paradropped this turn
  bool is_final;    ///< Whether this vertex can have children
  int waypoints;    ///< How many waypoints we have visited so far

  // Cost of the path to come here
  int turns;      ///< How many turns it takes to get there
  int moves_left; ///< How many move fragments the unit has left
  int health;     ///< How many HP the unit has left
  int fuel_left;  ///< How much fuel the unit has left

  // Ancestor information, needed to build a path. Invalid for the first
  // tile.
  vertex *parent;   ///< The previous vertex, if any
  unit_order order; ///< The order to come here

  static vertex from_unit(const unit &unit);

  vertex child_for_action(action_id action, const unit &probe,
                          const tile *target);

  bool comparable(const vertex &other) const;
  void fill_probe(unit &probe) const;

  bool operator==(const vertex &other) const;
  bool operator>(const vertex &other) const;
};
} // namespace detail

class destination;

/**
 * A path is a succession of moves and actions to go from one location to
 * another.
 */
class path_finder {
  friend class destination;

public:
  /**
   * The type of the underlying storage, exposed through \ref destination.
   */
  using storage_type =
      std::multimap<const tile *, std::unique_ptr<detail::vertex>>;

private:
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
    storage_type best_vertices;
    std::priority_queue<detail::vertex, std::vector<detail::vertex>,
                        std::greater<>>
        queue;

    // Waypoints are tiles we must use in our path
    std::vector<const tile *> waypoints;

    void insert_initial_vertex();
    void maybe_insert_vertex(const detail::vertex &v);

    bool is_reached(const destination &destination,
                    const detail::vertex &v) const;

    void attempt_move(detail::vertex &source);
    void attempt_full_mp(detail::vertex &source);
    void attempt_load(detail::vertex &source);
    void attempt_unload(detail::vertex &source);
    void attempt_paradrop(detail::vertex &source);
    void attempt_action_move(detail::vertex &source);

    bool run_search(const destination &destination);
  };

public:
  explicit path_finder(const unit *unit);
  virtual ~path_finder();

  inline path_finder &operator=(path_finder &&other);

  void push_waypoint(const tile *location);
  void pop_waypoint();

  void unit_changed(const unit &unit);

  std::optional<path> find_path(const destination &destination);

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

/**
 * \brief Abstraction for path finding destinations.
 *
 * The path finding algorithm can find paths not only to single tiles, but to
 * any set of vertices. This class is used to specify which vertices are
 * valid destinations.
 */
class destination {
public:
  friend class path_finder;
  friend class path_finder::path_finder_private;

  /**
   * Destructor.
   */
  virtual ~destination() {}

protected:
  /**
   * Checks if a vertex should be considered as being at the destination.
   */
  virtual bool reached(const detail::vertex &vertex) const = 0;

  virtual path_finder::storage_type::const_iterator
  find_best(const path_finder::storage_type &map,
            std::size_t num_waypoints) const;
};

/**
 * A path finding destination that accepts any path leading to a specific
 * tile.
 */
class tile_destination : public destination {
public:
  /**
   * Constructor.
   */
  explicit tile_destination(const tile *destination)
      : m_destination(destination)
  {
    fc_assert(destination != nullptr);
  }

  /**
   * Destructor.
   */
  ~tile_destination() {}

protected:
  bool reached(const detail::vertex &vertex) const override;
  path_finder::storage_type::const_iterator
  find_best(const path_finder::storage_type &map,
            std::size_t num_waypoints) const override;

private:
  const tile *m_destination;
};

/**
 * A path finding destination that accepts any allied city.
 */
class allied_city_destination : public destination {
public:
  /**
   * Constructor.
   */
  explicit allied_city_destination(const player *allied_with)
      : m_player(allied_with)
  {
    fc_assert(m_player != nullptr);
  }

  /**
   * Destructor.
   */
  ~allied_city_destination() {}

protected:
  bool reached(const detail::vertex &vertex) const override;

private:
  const player *m_player;
};

} // namespace freeciv
