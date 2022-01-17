// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "path_finder.h"

#include "game.h"
#include "map.h"
#include "movement.h"
#include "path.h"
#include "tile.h"
#include "unit.h"
#include "unit_utils.h"
#include "world_object.h"

#include <iostream>
#include <map>
#include <queue>
#include <set>

namespace freeciv {

namespace detail {

/**
 * Returns `true` if the comparison with `other` would be unambiguous.
 */
bool cost::comparable(const cost &other) const
{
  // When the results of the expressions below are positive, this cost does
  // better than the other for this criteria. When it's negative, it's the
  // opposite.
  auto a = other.turns - turns;
  auto b = moves_left - other.moves_left;
  auto c = health - other.health;
  auto d = fuel_left - other.fuel_left;
  // For the comparison to be meaningful, all criteria must go in the same
  // direction.
  return (a <= 0 && b <= 0 && c <= 0 && d <= 0)
         || (a >= 0 && b >= 0 && c >= 0 && d >= 0);
}

/**
 * Compares for equality.
 */
bool cost::operator==(const cost &other) const
{
  return std::tie(turns, other.moves_left, other.health, other.fuel_left)
         == std::tie(other.turns, moves_left, health, fuel_left);
}

/**
 * Defines a strict ordering among costs.
 */
bool cost::operator<(const cost &other) const
{
  // To break ties, we prefer the unit with the most moves, then the
  // healthiest unit, then the unit with the most fuel. This is an arbitrary
  // choice.
  return std::tie(turns, other.moves_left, other.health, other.fuel_left)
         < std::tie(other.turns, moves_left, health, fuel_left);
}

/**
 * Checks whether two vertices are comparable, which is the case when one of
 * them is unambiguously "better" than the other. Vertices that are not
 * comparable should be considered distinct: this is the case, for instance,
 * of vertices at different locations. Comparability is not a transitive
 * property.
 */
bool vertex::comparable(const vertex &other) const
{
  return std::tie(location, moved) == std::tie(other.location, other.moved)
         && cost.comparable(other.cost);
}

/**
 * Equality comparator.
 */
bool vertex::operator==(const vertex &other) const
{
  return std::tie(location, cost) == std::tie(other.location, other.cost);
}

/**
 * Defines an ordering for the priority queue.
 */
bool vertex::operator>(const vertex &other) const
{
  return other.cost < cost;
}

} // namespace detail

/**
 * Constructor.
 */
path_finder::path_finder_private::path_finder_private(const ::unit *unit)
    : unit(*unit)
{
  // Insert the starting vertex
  auto v = detail::vertex{unit->tile,
                          unit->moved,
                          {0, unit->moves_left, unit->hp, unit->fuel},
                          nullptr};
  queue.push(v);
  best_vertices.emplace(v.location, std::make_unique<detail::vertex>(v));
}

/**
 * Saves a new vertex for further processing if it is better than the current
 * vertex at the same location.
 */
void path_finder::path_finder_private::maybe_insert_vertex(
    const detail::vertex &v)
{
  // Make a copy because it may change before insertion
  auto insert = v;

  // Handle turn change
  if (v.cost.moves_left <= 0) {
    auto probe = unit;
    probe.tile = v.location;
    probe.moved = v.moved;
    probe.fuel = v.cost.fuel_left;
    probe.hp = v.cost.health;
    probe.moves_left = 0;

    // FIXME The order could be important here: fuel before HP or HP before
    // fuel?
    insert.cost.turns++;
    insert.cost.moves_left = unit_move_rate(&probe);

    // Fuel
    if (utype_fuel(probe.utype)) {
      if (is_unit_being_refueled(&probe)) {
        // Refuel
        probe.fuel = utype_fuel(probe.utype);
        insert.cost.fuel_left = probe.fuel;
      } else if (probe.fuel <= 1) {
        // The unit dies, don't generate a new vertex
        return;
      } else {
        // Consume fuel
        probe.fuel--;
        insert.cost.fuel_left--;
      }
    }

    // HP loss and recovery
    // Userful for helis, killunhomed, slow damaged units with fuel. A path
    // can require that the unit heals first. Of course, it will choose
    // barracks if you have them, because healing is faster there.
    unit_restore_hitpoints(&probe);
    if (probe.hp <= 0) {
      // Unit dies, don't let the user send it there.
      return;
    }
    insert.cost.health = probe.hp;

    // "Start of turn" part
    insert.moved = false; // Didn't move yet
  }

  // The remaining logic checks whether we should insert the new vertex. This
  // is complicated because the new candidate may be better than one or
  // several of the previous paths to the same tile. Also do some bookkeeping
  // so we only insert the new cost if it isn't already there.
  const auto [begin, end] = best_vertices.equal_range(v.location);
  bool do_insert = true;
  for (auto it = begin; it != end; /* in loop body */) {
    const bool comparable = it->second->comparable(insert);
    if (comparable && insert.cost < it->second->cost) {
      // The new candidate is strictly better. Remove the old one
      it = best_vertices.erase(it);
      continue; // ++it is done inside erase()
    } else if (comparable) {
      // We already have it (or something equivalent, or even something
      // better), no need to add it.
      do_insert = false;
      break;
    }
    ++it;
  }

  // Insert the new cost if needed
  if (do_insert) {
    queue.push(insert);
    best_vertices.emplace(v.location,
                          std::make_unique<detail::vertex>(insert));
  }
}

/**
 * Constructs a @c path_finder for the given unit and destination. Doesn't
 * start the path finding yet.
 *
 * The path finder becomes useless if the unit moves.
 */
path_finder::path_finder(const ::unit *unit)
    : m_d(std::make_unique<path_finder_private>(unit))
{
}

/**
 * Destructor.
 */
path_finder::~path_finder()
{
  // Make sure that the destructor of path_finder_private is known.
}

/**
 * Runs the path finding algorithm.
 */
path path_finder::find_path(const tile *destination)
{
  // Unit frozen by scenario
  if (m_d->unit.stay) {
    return path();
  }

  // Already at the destination
  if (m_d->unit.tile == destination) {
    return path();
  }

  // What follows is an implementation of Dijkstra path finding algorithm.
  while (!m_d->queue.empty()) {
    // Get the "best" vertex
    const auto v = m_d->queue.top();
    m_d->queue.pop(); // Remove it from the queue

    // Check if we just arrived
    if (v.location == destination) {
      break;
    }

    // An equivalent (or better) vertex may already have been processed.
    // Check that we have one of the "current best" vertices for that tile.
    const auto [begin, end] = m_d->best_vertices.equal_range(v.location);
    const auto it = std::find_if(
        begin, end, [&v](const auto &pair) { return *pair.second == v; });
    if (it == m_d->best_vertices.end()) {
      // Not found, we processed something else in the meantime. Since we
      // processed it earlier, that path was at least as short.
      continue;
    }

    // Fetch the pointer version of v for use as a parent
    const auto parent = it->second.get();

    // Update the probe
    auto probe = m_d->unit;
    probe.tile = v.location;
    probe.moved = v.moved;
    probe.fuel = v.cost.fuel_left;
    probe.hp = v.cost.health;
    probe.moves_left = v.cost.moves_left;

    // Try moving to adjacent tiles
    adjc_dir_iterate(&(wld.map), v.location, target, dir)
    {
      if (target->terrain == nullptr) {
        // Can't see this tile
        continue;
      }
      if (unit_can_move_to_tile(&wld.map, &probe, target, false, false)) {
        auto move_cost = std::min(
            map_move_cost_unit(&wld.map, &probe, target), probe.moves_left);

        // Construct the next vertex
        auto next = detail::vertex();
        next.location = target;
        next.moved = true;

        next.cost = v.cost;
        next.cost.moves_left -= move_cost;

        next.parent = parent;
        next.order.order = ORDER_MOVE;
        next.order.dir = dir;

        m_d->maybe_insert_vertex(next);
      }
    }
    adjc_dir_iterate_end;

    // As a last resort, we can always stay where we are. Maybe this
    // gives us enough MP or fuel to do something interesting next turn.
    auto next = v;
    next.cost.moves_left = 0; // Trigger end-of-turn logic
    next.parent = parent;
    next.order.order = ORDER_FULL_MP;
    m_d->maybe_insert_vertex(next);
  }

  if (m_d->best_vertices.count(destination) > 0) {
    // Find the best path. We may have several vertices, so select the one
    // with the lowest cost.
    const auto [begin, end] = m_d->best_vertices.equal_range(destination);
    const detail::vertex *best = nullptr;
    for (auto it = begin; it != end; ++it) {
      if (best == nullptr || it->second->cost < best->cost) {
        best = it->second.get();
      }
    }

    // Build a path
    auto steps = std::vector<path::step>();
    for (auto vertex = best; vertex->parent != nullptr;
         vertex = vertex->parent) {
      steps.push_back({vertex->location, vertex->cost.turns,
                       vertex->cost.turns, vertex->order});
    }

    return path(std::vector<path::step>(steps.rbegin(), steps.rend()));
  } else {
    return path();
  }
}

} // namespace freeciv
