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
    tile *location = nullptr;
    int turns, moves_left;
    unit_order order;
    bool is_waypoint;

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
  int turns() const { return m_steps.back().turns; }

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
