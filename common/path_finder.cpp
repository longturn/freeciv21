// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "path_finder.h"

#include "game.h"
#include "map.h"
#include "movement.h"
#include "path.h"
#include "tile.h"
#include "unit.h"
#include "world_object.h"

#include <map>
#include <queue>

namespace freeciv {

/**
 * Constructor.
 */
path_finder::path_finder_private::path_finder_private(const ::unit *unit)
    : unit(*unit)
{
  // Insert the starting vertex
  auto v = detail::vertex{unit->tile, {0, unit->moves_left}, nullptr};
  queue.push(v);
  best_vertices[v.location] = std::make_unique<detail::vertex>(v);
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
    probe.moves_left = 0;

    insert.cost.turns++;
    insert.cost.moves_left = unit_move_rate(&probe);
  }

  // Do we already have a vertex for this tile?
  auto it = best_vertices.find(insert.location);
  if (it == best_vertices.end() || *it->second > insert) {
    // The new vertex is better than anything we currently have
    queue.push(insert);
    best_vertices[insert.location] =
        std::make_unique<detail::vertex>(insert);
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

    // Update the probe
    auto probe = m_d->unit;
    probe.tile = v.location;
    probe.moves_left = v.cost.moves_left;

    // Fetch the pointer version of v for use as a parent
    const auto parent = m_d->best_vertices[v.location].get();

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

        next.cost = v.cost;
        next.cost.moves_left -= move_cost;

        next.parent = parent;
        next.order.order = ORDER_MOVE;
        next.order.dir = dir;

        m_d->maybe_insert_vertex(next);
      }
    }
    adjc_dir_iterate_end
  }

  if (m_d->best_vertices.count(destination) > 0) {
    // Build a path
    auto steps = std::vector<path::step>();
    for (auto vertex = m_d->best_vertices.at(destination).get();
         vertex->parent != nullptr; vertex = vertex->parent) {
      steps.push_back({vertex->location, vertex->cost.turns,
                       vertex->cost.turns, vertex->order});
    }

    return path(std::vector<path::step>(steps.rbegin(), steps.rend()));
  } else {
    return path();
  }
}

} // namespace freeciv
