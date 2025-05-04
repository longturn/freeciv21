// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv and Freeciv21 Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

// self
#include "path.h"

// utility
#include "log.h"

// common
#include "path_finder.h"
#include "unit.h"

// Qt
#include <QtPreprocessorSupport> // Q_UNUSED

// std
#include <algorithm> // std::sort, std::unique
#include <vector>    // std::vector

namespace freeciv {

/**
 * Finds the first step in the path that is *unsafe* (if any).
 *
 * Unsafe steps are the ones from which it is impossible to come back because
 * the unit will not have enough fuel or HP left to make it to safety (unless
 * something else changes).
 */
std::vector<path::step>::const_iterator
path::first_unsafe_step(unit *unit) const
{
  fc_assert_ret_val(unit != nullptr, m_steps.end());
  // Bisection search
  auto it = std::lower_bound(
      m_steps.begin(), m_steps.end(), 1, [unit](auto step, auto dummy) {
        Q_UNUSED(dummy);
        // Always let the search begin. Note that this makes the result not
        // fully reliable for ORDER_ACTION_MOVE at the end of a path (eg when
        // attacking a city) [FIXME]. It is, however, better to underestimate
        // the cost of attack in some cases than to always think that
        // ORDER_ACTION_MOVE is fuel-unsafe (it's almost always not the
        // case).
        step.is_final = false;
        path_finder finder(unit, step);
        return bool(finder.find_path(refuel_destination(*unit)));
      });
  return it;
}

} // namespace freeciv
