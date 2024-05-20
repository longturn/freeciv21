// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
// SPDX-License-Identifier: GPLv3-or-later

#include "unit_utils.h"

#include "player.h"
#include "unit.h"
#include "unitlist.h"

#include <algorithm>

namespace /* anonymous */ {
/**
 * Finds how deeply the unit is nested in transports.
 */
int transport_depth(const unit *unit)
{
  int depth = 0;
  for (auto parent = unit->transporter; parent != nullptr;
       parent = parent->transporter) {
    depth++;
  }
  return depth;
}

/**
 * Comparison function to sort units. Used in the city dialog and the unit
 * selector.
 */
bool units_sort(const unit *lhs, const unit *rhs)
{
  if (lhs == rhs) {
    return 0;
  }

  // Transports are shown before the units they transport.
  if (lhs == rhs->transporter) {
    return true;
  } else if (lhs->transporter == rhs) {
    return false;
  }

  // When one unit is deeper or the two transporters are different, compare
  // the parents instead.
  int lhs_depth = transport_depth(lhs);
  int rhs_depth = transport_depth(rhs);
  if (lhs_depth > rhs_depth) {
    return units_sort(lhs->transporter, rhs);
  } else if (lhs_depth < rhs_depth) {
    return units_sort(lhs, rhs->transporter);
  } else if (lhs->transporter != rhs->transporter) {
    return units_sort(lhs->transporter, rhs->transporter);
  }

  // Put defensive units on the left - these are least likely to move, having
  // them first makes the list more stable
  const auto lhs_def = lhs->utype->defense_strength * lhs->utype->hp;
  const auto rhs_def = rhs->utype->defense_strength * rhs->utype->hp;
  if (lhs_def != rhs_def) {
    return lhs_def > rhs_def;
  }

  // More veteran units further left
  if (lhs->veteran != rhs->veteran) {
    return lhs->veteran > rhs->veteran;
  }

  // Reverse order by unit type - in most ruleset this means old units last
  if (lhs->utype != rhs->utype) {
    return lhs->utype->item_number > rhs->utype->item_number;
  }

  // By nationality so units from the same player are grouped together
  if (player_index(lhs->nationality) != player_index(rhs->nationality)) {
    return player_index(lhs->nationality) < player_index(rhs->nationality);
  }

  // Then unit id
  return lhs->id < rhs->id;
}
} // anonymous namespace

/**
 * Returns a version of \c units sorted in the way the user would like to see
 * them.
 */
std::vector<unit *> sorted(const unit_list *units)
{
  std::vector<unit *> vec;
  unit_list_iterate(units, unit) { vec.push_back(unit); }
  unit_list_iterate_end;

  std::sort(vec.begin(), vec.end(), units_sort);

  return vec;
}
