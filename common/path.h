// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <unit.h>

#include <vector>

struct tile;

namespace freeciv {

/**
 * A path is a succession of moves and actions to go from one location to
 * another.
 */
class path {
public:
  struct step {
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

    unit_order order; ///< The order to come here

    operator bool() const { return location != nullptr; }
  };

  /**
   * Constructor.
   */
  explicit path() {}

  /**
   * Constructor.
   */
  explicit path(const std::vector<step> &steps) : m_steps(steps) {}

  /**
   * Finds how many turns (rounded down) it takes to reach the end of the
   * path.
   */
  int turns() const { return empty() ? 0 : m_steps.back().turns; }

  /**
   * Returns true if the path is empty, usually meaning that a path to the
   * destination was not found.
   */
  bool empty() const { return m_steps.empty(); }

  /**
   * Returns the steps making up this path.
   */
  std::vector<step> steps() const { return m_steps; }

private:
  std::vector<step> m_steps;
};

} // namespace freeciv
