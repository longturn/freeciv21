// SPDX-FileCopyrightText: 2022 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "path_finder.h"

#include "actions.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "path.h"
#include "tile.h"
#include "unit.h"
#include "unit_utils.h"
#include "world_object.h"

#include <map>
#include <queue>
#include <set>

namespace freeciv {

namespace detail {

/**
 * Creates a vertex representing the state of a unit.
 */
vertex vertex::from_unit(const unit &unit)
{
  return detail::vertex{{unit.tile, unit.transporter, unit.moved,
                         unit.paradropped,
                         false, // Final
                         0,     // Waypoints
                         0,     // Turns
                         unit.moves_left, unit.hp, unit.fuel},
                        nullptr};
}

/**
 * Creates a vertex starting from this one and executing an action. The
 * vertex state is initialized as a copy of this one except that the action
 * move cost is subtracted.
 */
vertex vertex::child_for_action(action_id action, const unit &probe,
                                tile *target)
{
  auto ret = *this;
  ret.parent = this;
  ret.order.order = ORDER_PERFORM_ACTION;
  ret.order.action = action;
  ret.order.target = target->index;
  ret.order.dir = DIR8_ORIGIN;
  ret.moves_left = probe.moves_left;
  ret.location = target;

  const auto action_ptr = action_by_number(action);
  if (utype_is_moved_to_tgt_by_action(action_ptr, probe.utype)) {
    // We need a second probe because EFT_ACTION_SUCCESS_MOVE_COST is
    // evaluated after the unit has moved
    auto moved_probe = probe;
    moved_probe.tile = target;
    ret.moves_left -= unit_pays_mp_for_action(action_ptr, &moved_probe);
  } else {
    ret.moves_left -= unit_pays_mp_for_action(action_ptr, &probe);
  }

  return ret;
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
  // All "state" variables must be equal
  if (std::tie(location, loaded, moved, paradropped, is_final, waypoints)
      != std::tie(other.location, other.loaded, other.moved,
                  other.paradropped, other.is_final, other.waypoints)) {
    return false;
  }

  // When the results of the expressions below are positive, this node does
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
 * Ensures that `probe` reflects the properties of this vertex. Any property
 * of `probe` not used in path finding is left unchanged.
 */
void vertex::fill_probe(unit &probe) const
{
  probe.tile = location;
  probe.transporter = loaded;
  probe.client.transported_by = loaded ? loaded->id : -1;
  probe.moved = moved;
  probe.paradropped = paradropped;
  probe.moves_left = moves_left;
  probe.hp = health;
  probe.fuel = fuel_left;
}

/**
 * Equality comparator.
 */
bool vertex::operator==(const vertex &other) const
{
  return std::tie(location, loaded, moved, paradropped, is_final, waypoints,
                  turns, moves_left, health, fuel_left)
         == std::tie(other.location, other.loaded, other.moved,
                     other.paradropped, other.is_final, other.waypoints,
                     other.turns, other.moves_left, other.health,
                     other.fuel_left);
}

/**
 * Defines an ordering for the priority queue.
 */
bool vertex::operator>(const vertex &other) const
{
  return std::tie(turns, other.moves_left, other.health, paradropped,
                  other.fuel_left)
         > std::tie(other.turns, moves_left, health, other.paradropped,
                    fuel_left);
}

} // namespace detail

/**
 * Constructor.
 */
path_finder::path_finder_private::path_finder_private(
    const ::unit *unit, const detail::vertex &init)
    : unit(*unit), initial_vertex(init)
{
  insert_initial_vertex();
}

/**
 * Inserts the initial vertex, from which the search will be started.
 */
void path_finder::path_finder_private::insert_initial_vertex()
{
  // Insert the starting vertex
  queue.push(initial_vertex);
  best_vertices.emplace(initial_vertex.location,
                        std::make_unique<detail::vertex>(initial_vertex));
}

/**
 * Saves a new vertex for further processing if it is better than the current
 * vertex at the same location.
 */
void path_finder::path_finder_private::maybe_insert_vertex(
    const detail::vertex &v)
{
  // Handle any constraint.
  if (constraint && !constraint->is_allowed(v)) {
    return;
  }

  // Make a copy because it may change before insertion
  auto insert = v;

  // Handle turn change
  if (v.moves_left <= 0) {
    // Make a probe
    auto probe = unit;
    v.fill_probe(probe);

    // FIXME The order could be important here: fuel before HP or HP before
    // fuel?
    insert.turns++;
    insert.moves_left = unit_move_rate(&probe);

    // Fuel
    if (utype_fuel(probe.utype)) {
      if (is_unit_being_refueled(&probe)) {
        // Refuel
        probe.fuel = utype_fuel(probe.utype);
        insert.fuel_left = probe.fuel;
      } else if (probe.fuel <= 1) {
        // The unit dies, don't generate a new vertex
        return;
      } else {
        // Consume fuel
        probe.fuel--;
        insert.fuel_left--;
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
    insert.health = probe.hp;

    // "Start of turn" part
    insert.moved = false;       // Didn't move yet
    insert.paradropped = false; // Didn't paradrop yet
  }

  // Did we just reach a waypoint?
  if (insert.waypoints < waypoints.size()
      && insert.location == waypoints[insert.waypoints]) {
    insert.waypoints++;
  }

  // The remaining logic checks whether we should insert the new vertex. This
  // is complicated because the new candidate may be better than one or
  // several of the previous paths to the same tile. Also do some bookkeeping
  // so we only insert the new cost if it isn't already there.
  const auto [begin, end] = best_vertices.equal_range(v.location);
  bool do_insert = true;
  for (auto it = begin; it != end; /* in loop body */) {
    const bool comparable = it->second->comparable(insert);
    if (comparable && *it->second > insert) {
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
 * Checks if a vertex is at the destination, taking waypoints into account.
 */
bool path_finder::path_finder_private::is_reached(
    const destination &destination, const detail::vertex &v) const
{
  // Check that we went through every waypoint
  if (v.waypoints != waypoints.size()) {
    return false;
  }

  // We've just arrived.
  if (destination.reached(v)) {
    return true;
  }

  return false;
}

/**
 * Opens vertices corresponding to attempts to do ORDER_MOVE from the source
 * vertex.
 */
void path_finder::path_finder_private::attempt_move(detail::vertex &source)
{
  // Don't attempt to move loaded units
  if (source.loaded) {
    return;
  }

  // Make a probe
  auto probe = unit;
  source.fill_probe(probe);

  // Try moving to adjacent tiles
  adjc_dir_iterate(&(wld.map), source.location, target, dir)
  {
    bool can_move;
    int move_cost;
    if (tile_get_known(target, unit_owner(&probe)) == TILE_UNKNOWN) {
      // Try to move into the unknown
      can_move = true;
      move_cost = probe.utype->unknown_move_cost;
    } else {
      can_move =
          unit_can_move_to_tile(&wld.map, &probe, target, false, false);
      move_cost = map_move_cost_unit(&wld.map, &probe, target);
    }
    if (can_move) {
      move_cost = std::min(move_cost, probe.moves_left);

      // Construct the next vertex
      auto next = source;
      next.location = target;
      next.moved = true;
      next.moves_left -= move_cost;
      next.parent = &source;
      next.order.order = ORDER_MOVE;
      next.order.dir = dir;
      maybe_insert_vertex(next);
    }
  }
  adjc_dir_iterate_end;
}

/**
 * Opens vertices corresponding to attempts to do ORDER_FULL_MP from the
 * source vertex. This is a last resort vertex that may give the unit more HP
 * or fuel that will be useful to continue its journey.
 */
void path_finder::path_finder_private::attempt_full_mp(
    detail::vertex &source)
{
  auto next = source;
  next.moves_left = 0; // Trigger end-of-turn logic
  next.parent = &source;
  next.order.order = ORDER_FULL_MP;
  maybe_insert_vertex(next);
}

/**
 * Opens vertices corresponding to attempts to load into a transport from the
 * source vertex.
 */
void path_finder::path_finder_private::attempt_load(detail::vertex &source)
{
  // Make a probe
  auto probe = unit;
  source.fill_probe(probe);

  // Try to load into a transport -- even if we're already in a transport
  // Same tile (maybe we can recover HP)
  if (auto transport = transporter_for_unit(&probe);
      transport != nullptr
      && is_action_enabled_unit_on_unit(ACTION_TRANSPORT_BOARD, &probe,
                                        transport)) {
    auto next =
        source.child_for_action(ACTION_TRANSPORT_BOARD, probe, probe.tile);
    next.loaded = transport;
    maybe_insert_vertex(next);
  }

  // Nearby tiles
  adjc_iterate(&(wld.map), probe.tile, target)
  {
    probe.tile = target;
    auto transport = transporter_for_unit(&probe);
    // Reset the probe -- needed now because is_action_enabled_unit_on_unit
    // checks the range
    probe.tile = source.location;
    if (transport != nullptr
        && is_action_enabled_unit_on_unit(ACTION_TRANSPORT_EMBARK, &probe,
                                          transport)) {
      auto next =
          source.child_for_action(ACTION_TRANSPORT_EMBARK, probe, target);
      // See unithand.cpp:do_unit_embark
      next.moves_left -= map_move_cost_unit(&(wld.map), &probe, target);
      next.moved = true;
      next.loaded = transport;
      maybe_insert_vertex(next);
    }
  }
  adjc_iterate_end;
}

/**
 * Opens vertices corresponding to attempts to unload from a transport at the
 * source vertex.
 */
void path_finder::path_finder_private::attempt_unload(detail::vertex &source)
{
  // Make a probe
  auto probe = unit;
  source.fill_probe(probe);

  // Try to unload from a transport -- but only if we're already loaded
  if (probe.transporter != nullptr) {
    // Same tile
    if (is_action_enabled_unit_on_unit(ACTION_TRANSPORT_ALIGHT, &probe,
                                       probe.transporter)) {
      auto next = source.child_for_action(ACTION_TRANSPORT_ALIGHT, probe,
                                          probe.tile);
      next.loaded = nullptr;
      maybe_insert_vertex(next);
    }

    // Nearby tiles
    adjc_iterate(&(wld.map), probe.tile, target)
    {
      if (is_action_enabled_unit_on_tile(ACTION_TRANSPORT_DISEMBARK1, &probe,
                                         target, nullptr)) {
        auto next = source.child_for_action(ACTION_TRANSPORT_DISEMBARK1,
                                            probe, target);
        next.moved = true;
        next.loaded = nullptr;
        // See unithand.cpp:do_disembark
        next.moves_left -= map_move_cost_unit(&(wld.map), &probe, target);
        maybe_insert_vertex(next);
      }
      // Thanks sveinung
      if (is_action_enabled_unit_on_tile(ACTION_TRANSPORT_DISEMBARK2, &probe,
                                         target, nullptr)) {
        auto next = source.child_for_action(ACTION_TRANSPORT_DISEMBARK2,
                                            probe, target);
        next.moved = true;
        next.loaded = nullptr;
        // See unithand.cpp:do_disembark
        next.moves_left -= map_move_cost_unit(&(wld.map), &probe, target);
        maybe_insert_vertex(next);
      }
    }
    adjc_iterate_end;
  }
}

/**
 * Opens vertices corresponding to attempts to unload from a transport at the
 * source vertex.
 */
void path_finder::path_finder_private::attempt_paradrop(
    detail::vertex &source)
{
  // Make a probe
  auto probe = unit;
  source.fill_probe(probe);

  // Try to paradrop -- if we can at all
  if (!probe.paradropped
      && utype_can_do_action(probe.utype, ACTION_PARADROP)) {
    // Get action details
    auto action = action_by_number(ACTION_PARADROP);

    // circle_dxyr_iterate will take the square root of this
    auto sq_radius =
        std::min(action->max_distance, probe.utype->paratroopers_range);
    sq_radius *= sq_radius;

    // circle_dxyr_iterate does some magic with the name of the center tile
    // variable, make sure it will works
    auto tile = probe.tile;

    // Iterate over reachable tiles
    circle_dxyr_iterate(&(wld.map), tile, sq_radius, target, _dx, _dy,
                        distance)
    {
      if (distance >= action->min_distance * action->min_distance) {
        // Paradrop has many hard requirements, see unittools.cpp:do_paradrop
        // The target tile must be seen or be a native tile
        if (TILE_KNOWN_SEEN != tile_get_known(target, probe.owner)
            && !is_native_tile(probe.utype, target)) {
          continue;
        }

        // Don't drown in the ocean (but allow dropping into ships)
        if (!can_unit_exist_at_tile(&(wld.map), &probe, target)
            && (!game.info.paradrop_to_transport
                || !unit_could_load_at(&probe, target))) {
          continue;
        }

        // We must be allowed to go to the target tile (not at peace with
        // potential owner)
        if (target->owner
            && pplayers_non_attack(probe.owner, target->owner)) {
          continue;
        }

        // Don't paradrop on top of other units or cities
        if (is_non_allied_unit_tile(target, probe.owner)) {
          continue;
        }

        // Don't paradrop to non allied cities
        if (tile_city(target) && !is_allied_city_tile(target, probe.owner)) {
          continue;
        }

        // Soft requirements
        if (!is_action_enabled_unit_on_tile(ACTION_PARADROP, &probe, target,
                                            nullptr)) {
          continue;
        }

        auto next = source.child_for_action(ACTION_PARADROP, probe, target);
        next.location = target;
        next.paradropped = true;
        next.moved = true;
        next.loaded = nullptr;
        maybe_insert_vertex(next);
      }
    }
    circle_dxyr_iterate_end;
  }
}

/**
 * Opens vertices corresponding to attempts to do ORDER_ACTION_MOVE from the
 * source vertex. The generated vertices are always `final` because
 * ORDER_ACTION_MOVE cannot be used in the middle of a path.
 */
void path_finder::path_finder_private::attempt_action_move(
    detail::vertex &source)
{
  // Make a probe
  auto probe = unit;
  source.fill_probe(probe);

  // Try action-moving to adjacent tiles
  adjc_dir_iterate(&(wld.map), source.location, target, dir)
  {
    if (target->terrain == nullptr) {
      // Can't see this tile
      continue;
    }

    // Only search for actions on tiles with cities or visible stacks. This
    // matches the old client behavior and allows us to skip the (extremely
    // slow) search for actions most of the times.
    if (!tile_city(target) && unit_list_size(target->units) == 0) {
      continue;
    }

    // See unithand.cpp:unit_move_handling
    const bool can_not_move =
        !unit_can_move_to_tile(&(wld.map), &probe, target, false, false);

    const bool one_action_may_be_legal =
        action_tgt_unit(&probe, target, can_not_move)
        || action_tgt_city(&probe, target, can_not_move)
        // A legal action with an extra sub target is a legal action
        || action_tgt_tile_extra(&probe, target, can_not_move)
        // Tile target actions with extra sub targets are handled above
        // Includes actions vs unit stacks
        || action_tgt_tile(&probe, target, nullptr, can_not_move);

    if (one_action_may_be_legal) {
      // Construct the vertex
      auto next = source;
      next.location = target;
      next.moved = true;
      next.is_final = true;
      next.parent = &source;
      next.order.order = ORDER_ACTION_MOVE;
      next.order.dir = dir;
      maybe_insert_vertex(next);
    }
  }
  adjc_dir_iterate_end;
}

/**
 * Runs the path finding seach until the stopping condition is met (the
 * destination tile is reached). Checks if the tile has already been reached
 * before proceeding.
 *
 * If full is true, the algorithm is only stopped once all possibilities have
 * been found.
 *
 * \returns true if a path was found.
 */
bool path_finder::path_finder_private::run_search(
    const destination &destination, bool full)
{
  // Check if we've already found a path (but keep searching if the tip of
  // the queue is cheaper: we haven't checked every possibility).
  if (auto it = destination.find_best(best_vertices, waypoints.size());
      it != best_vertices.end()
      && !(!queue.empty() && *it->second > queue.top())) {
    return true;
  }

  // What follows is an implementation of Dijkstra's path finding algorithm.
  while (!queue.empty()) {
    // Get the "best" vertex
    const auto v = queue.top();

    // Check if we just arrived
    // Keep the node in the queue so adjacent nodes are generated if the
    // search needs to be expanded later.
    if (!full && is_reached(destination, v)) {
      return true;
    }

    queue.pop(); // Remove it from the queue

    // An equivalent (or better) vertex may already have been processed.
    // Check that we have one of the "current best" vertices for that tile.
    const auto [begin, end] = best_vertices.equal_range(v.location);
    const auto it = std::find_if(
        begin, end, [&v](const auto &pair) { return *pair.second == v; });
    if (it == end) {
      // Not found, we processed something else in the meantime. Since we
      // processed it earlier, that path was at least as short.
      continue;
    }

    if (!v.is_final) {
      // Fetch the pointer version of v for use as a parent
      auto parent = it->second.get();

      // Generate vertices starting from this one
      attempt_move(*parent);
      attempt_full_mp(*parent);
      attempt_load(*parent);
      attempt_unload(*parent);
      attempt_paradrop(*parent);
      attempt_action_move(*parent);
    }
  }

  return false;
}

/**
 * Resets the state of the path finder. The search will be resumed from the
 * beginning.
 */
void path_finder::path_finder_private::reset()
{
  best_vertices.clear();
  while (!queue.empty()) {
    queue.pop();
  }
  insert_initial_vertex();
}

/**
 * Constructs a @c path_finder for the given unit. Doesn't start the path
 * finding yet.
 *
 * The path finder becomes useless if the unit state changes.
 */
path_finder::path_finder(const unit *unit)
    : path_finder(unit, detail::vertex::from_unit(*unit))
{
}

/**
 * Constructs a @c path_finder for the given unit. Initializes the search
 * from a step in the unit's planned path. Doesn't start the path finding
 * yet.
 *
 * The path finder becomes useless if the unit state changes.
 */
path_finder::path_finder(const unit *unit, const path::step &init)
    : m_d(std::make_unique<path_finder_private>(
        unit, detail::vertex{init, nullptr}))
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
 * Sets a constraint to limit the number of steps considered when trying to
 * find a path. A path_finder can only have one constraint at a time.
 *
 * Takes ownership of the constraint.
 */
void path_finder::set_constraint(
    std::unique_ptr<step_constraint> &&constraint)
{
  m_d->constraint = std::move(constraint);
  m_d->reset();
}

/**
 * Adds a waypoint to the path finding. Waypoints are tiles that the path
 * must go through (in order) before reaching the destination.
 *
 * \see pop_waypoint
 */
void path_finder::push_waypoint(const tile *location)
{
  if (m_d->waypoints.empty() || location != m_d->waypoints.back()) {
    m_d->waypoints.push_back(location);

    // We can try to be smarter later. For now, just invalidate everything.
    m_d->reset();
  }
}

/**
 * Removes the last added waypoint.
 *
 * \see push_waypoint
 */
bool path_finder::pop_waypoint()
{
  // Nothing to to.
  if (m_d->waypoints.empty()) {
    return false;
  }

  m_d->waypoints.pop_back();

  // We can try to be smarter later. For now, just invalidate everything.
  m_d->reset();

  return true;
}
/**
 * Notifies the path finder that some unit died or changed state. In many
 * cases, this will trigger a recalculation of the path.
 */
void path_finder::unit_changed(const ::unit &unit)
{
  Q_UNUSED(unit);

  // We can try to be smarter later. For now, just invalidate everything.
  m_d->best_vertices.clear();
  while (!m_d->queue.empty()) {
    m_d->queue.pop();
  }
  m_d->insert_initial_vertex();
}

/**
 * Runs the path finding algorithm, searching for all paths leading to the
 * destination.
 * Make sure to set a harsh step constraint before calling this function, or
 * it will become very slow very quickly.
 *
 * \warning Potentially very expensive.
 */
std::vector<path> path_finder::find_all(const destination &destination)
{
  // Unit frozen by scenario
  if (m_d->unit.stay) {
    return {};
  }

  m_d->run_search(destination, true);

  // Collect results.
  auto ret = std::vector<path>();
  ret.reserve(m_d->best_vertices.size());

  for (const auto &[_, end] : m_d->best_vertices) {
    // Only use vertices at the destination
    if (!m_d->is_reached(destination, *end)) {
      continue;
    }

    // Build a path
    auto steps = std::vector<path::step>();
    for (auto vertex = end.get(); vertex->parent != nullptr;
         vertex = vertex->parent) {
      steps.push_back(*vertex);
    }

    ret.emplace_back(std::vector<path::step>(steps.rbegin(), steps.rend()));
  }

  return ret;
}

/**
 * Runs the path finding algorithm.
 */
std::optional<path> path_finder::find_path(const destination &destination)
{
  // Unit frozen by scenario
  if (m_d->unit.stay) {
    return std::nullopt;
  }

  // Check if we're already at the destination
  if (m_d->waypoints.empty() && destination.reached(m_d->initial_vertex)) {
    return path();
  }

  if (m_d->run_search(destination)) {
    // Find the best path. We may have several vertices, so select the one
    // with the lowest cost.
    const auto it =
        destination.find_best(m_d->best_vertices, m_d->waypoints.size());

    // If run_search returned true, we should always have something. But
    // better check anyway.
    fc_assert_ret_val(it != m_d->best_vertices.end(), std::nullopt);

    // Build a path
    auto steps = std::vector<path::step>();
    for (auto vertex = it->second.get(); vertex->parent != nullptr;
         vertex = vertex->parent) {
      steps.push_back(*vertex);
    }

    return path(std::vector<path::step>(steps.rbegin(), steps.rend()));
  } else {
    return std::nullopt;
  }
}

/**
 * Returns an iterator to the best vertex that is a destination vertex. The
 * default implementation calls \ref reached for every vertex.
 */
path_finder::storage_type::const_iterator
destination::find_best(const path_finder::storage_type &map,
                       std::size_t num_waypoints) const
{
  auto best = map.end();
  for (auto it = map.begin(); it != map.end(); ++it) {
    // Is this vertex a destination?
    if (it->second->waypoints == num_waypoints && reached(*it->second)) {
      // Is it better than the current `best'?
      if (best == map.end() || *best->second > *it->second) {
        best = it;
      }
    }
  }
  return best;
}

/**
 * \copydoc destination::reached
 */
bool tile_destination::reached(const detail::vertex &vertex) const
{
  return vertex.location == m_destination;
}

/**
 * \copydoc destination::find_best
 *
 * This implementation only checks relevant nodes.
 */
path_finder::storage_type::const_iterator
tile_destination::find_best(const path_finder::storage_type &map,
                            std::size_t num_waypoints) const
{
  auto best = map.end();
  const auto [begin, end] = map.equal_range(m_destination);
  for (auto it = begin; it != end; ++it) {
    // Is this vertex a destination?
    if (it->second->waypoints == num_waypoints && reached(*it->second)) {
      // Is it better than the current `best'?
      if (best == map.end() || *best->second > *it->second) {
        best = it;
      }
    }
  }
  return best;
}

/**
 * \copydoc destination::reached
 */
bool allied_city_destination::reached(const detail::vertex &vertex) const
{
  return is_allied_city_tile(vertex.location, m_player);
}

/**
 * \copydoc destination::reached
 */
bool refuel_destination::reached(const detail::vertex &vertex) const
{
  // "final" vertices currently only include ORDER_ACTION_MOVE. Try to find
  // why this was generated: reject refueling if it wasn't because of an
  // allied city or unit.
  if (vertex.is_final && !is_allied_unit_tile(vertex.location, m_unit.owner)
      && !is_allied_city_tile(vertex.location, m_unit.owner)) {
    return false;
  }

  // Get a probe
  auto probe = m_unit;
  vertex.fill_probe(probe);

  return can_unit_survive_at_tile(&(wld.map), &probe, vertex.location);
}

/**
 * \copydoc step_constraint::is_allowed
 */
bool tile_known_constraint::is_allowed(const path::step &step) const
{
  return tile_get_known(step.location, m_player) != TILE_UNKNOWN;
}

} // namespace freeciv
