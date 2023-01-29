/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include <cstring>

// Qt
#include <QLoggingCategory>

// common
#include "actions.h"
#include "fc_types.h"
#include "game.h"
#include "map.h"
#include "packets.h"
#include "path_finder.h"
#include "pf_tools.h"
#include "road.h"
#include "unit.h"
#include "unitlist.h"

/* client/include */
#include "client_main.h"
#include "control.h"
#include "mapview_g.h"

// client
#include "goto.h"
#include "mapctrl_common.h"

// Logging category for goto
Q_LOGGING_CATEGORY(goto_category, "freeciv.goto")

class PFPath;

// Indexed by unit id
static auto goto_finders = std::map<int, freeciv::path_finder>();

/**
  Various stuff for the goto routes
 */
static struct tile *goto_destination = nullptr;

/**
   Returns if unit can move now
 */
bool can_unit_move_now(const struct unit *punit)
{
  time_t dt;

  if (!punit) {
    return false;
  }

  if (punit->action_turn != game.info.turn - 1) {
    return true;
  }

  dt = time(nullptr) - punit->action_timestamp;
  if (dt < 0) {
    return false;
  }

  return true;
}

/**
   Called only by handle_map_info() in client/packhand.c.
 */
void init_client_goto()
{
  // Nothing
}

/**
   Called above, and by control_done() in client/control.c.
 */
void free_client_goto()
{
  goto_finders.clear();
  goto_destination = nullptr;
}

/**
   Determines if a goto to the destination tile is allowed.
 */
bool is_valid_goto_destination(const struct tile *ptile)
{
  return (nullptr != goto_destination && ptile == goto_destination);
}

/**
   Inserts a waypoint at the end of the current goto line.
 */
void goto_add_waypoint()
{
  for (auto &[_, finder] : goto_finders) {
    // Patrol always uses a waypoint
    if (hover_state == HOVER_PATROL) {
      finder.pop_waypoint();
    }

    finder.push_waypoint(goto_destination);

    // Patrol always uses a waypoint
    if (hover_state == HOVER_PATROL) {
      finder.push_waypoint(goto_destination);
    }
  }
}

/**
   Returns whether there were any waypoint popped (we don't remove the
   initial position)
 */
bool goto_pop_waypoint()
{
  bool popped = false;
  for (auto &[_, finder] : goto_finders) {
    // Patrol always uses a waypoint
    if (hover_state == HOVER_PATROL) {
      finder.pop_waypoint();
    }

    popped |= finder.pop_waypoint();

    // Patrol always uses a waypoint
    if (hover_state == HOVER_PATROL) {
      finder.push_waypoint(goto_destination);
    }
  }
  return popped;
}

/**
   PF callback to get the path with the minimal number of steps (out of
   all shortest paths).
 */
static int get_EC(const struct tile *ptile, enum known_type known,
                  const struct pf_parameter *param)
{
  return 1;
}

/**
   PF callback to prohibit going into the unknown.  Also makes sure we
   don't plan our route through enemy city/tile.
 */
static enum tile_behavior get_TB_aggr(const struct tile *ptile,
                                      enum known_type known,
                                      const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN) {
    if (!gui_options->goto_into_unknown) {
      return TB_IGNORE;
    }
  } else if (is_non_allied_unit_tile(ptile, param->owner)
             || is_non_allied_city_tile(ptile, param->owner)) {
    // Can attack but can't count on going through
    return TB_DONT_LEAVE;
  }
  return TB_NORMAL;
}

/**
   PF callback for caravans. Caravans doesn't go into the unknown and
   don't attack enemy units but enter enemy cities.
 */
static enum tile_behavior get_TB_caravan(const struct tile *ptile,
                                         enum known_type known,
                                         const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN) {
    if (!gui_options->goto_into_unknown) {
      return TB_IGNORE;
    }
  } else if (is_non_allied_city_tile(ptile, param->owner)) {
    /* Units that can establish a trade route, enter a market place or
     * establish an embassy can travel to, but not through, enemy cities.
     * FIXME: ACTION_HELP_WONDER units cannot.  */
    return TB_DONT_LEAVE;
  } else if (is_non_allied_unit_tile(ptile, param->owner)) {
    // Note this must be below the city check.
    return TB_IGNORE;
  }

  // Includes empty, allied, or allied-city tiles.
  return TB_NORMAL;
}

/**
   PF callback to prohibit going into the unknown (conditionally).  Also
   makes sure we don't plan to attack anyone.
 */
static enum tile_behavior
no_fights_or_unknown_goto(const struct tile *ptile, enum known_type known,
                          const struct pf_parameter *p)
{
  if (known == TILE_UNKNOWN && gui_options->goto_into_unknown) {
    // Special case allowing goto into the unknown.
    return TB_NORMAL;
  }

  return no_fights_or_unknown(ptile, known, p);
}

/**
   Fill the PF parameter with the correct client-goto values.
   See also goto_fill_parameter_full().
 */
static void goto_fill_parameter_base(struct pf_parameter *parameter,
                                     const struct unit *punit)
{
  pft_fill_unit_parameter(parameter, punit);

  fc_assert(parameter->get_EC == nullptr);
  fc_assert(parameter->get_TB == nullptr);
  fc_assert(parameter->get_MC != nullptr);
  fc_assert(parameter->start_tile == unit_tile(punit));
  fc_assert(parameter->omniscience == false);

  parameter->get_EC = get_EC;
  if (utype_acts_hostile(unit_type_get(punit))) {
    parameter->get_TB = get_TB_aggr;
  } else if (utype_may_act_at_all(unit_type_get(punit))
             && !utype_acts_hostile(unit_type_get(punit))) {
    parameter->get_TB = get_TB_caravan;
  } else {
    parameter->get_TB = no_fights_or_unknown_goto;
  }
}

/**
   Enter the goto state: activate, prepare PF-template and add the
   initial part.
 */
void enter_goto_state(const std::vector<unit *> &units)
{
  fc_assert_ret(!goto_is_active());

  // Can't have selection rectangle and goto going on at the same time.
  cancel_selection_rectangle();

  // Initialize path finders
  for (const auto punit : units) {
    // This calls path_finder::path_finder(punit) behind the scenes
    // Need to do this because path_finder isn't copy-assignable
    auto [it, _] = goto_finders.emplace(punit->id, punit);
    if (!gui_options->goto_into_unknown) {
      it->second.set_constraint(
          std::make_unique<freeciv::tile_known_constraint>(client_player()));
    }
  }
}

/**
   Tidy up and deactivate goto state.
 */
void exit_goto_state()
{
  if (!goto_is_active()) {
    return;
  }

  mapdeco_clear_gotoroutes();

  goto_finders.clear();
  goto_destination = nullptr;
}

/**
   Called from control_unit_killed() in client/control.c
 */
void goto_unit_killed(struct unit *punit)
{
  if (!goto_is_active()) {
    return;
  }

  // Drop the finder for the killed unit, if any.
  goto_finders.erase(punit->id);

  // Notify our finders that something has changed.
  for (auto &[_, finder] : goto_finders) {
    finder.unit_changed(*punit);
  }
}

/**
   Is goto state active?
 */
bool goto_is_active() { return !goto_finders.empty(); }

/**
   Returns the state of 'ptile': turn number to print, and whether 'ptile'
   is a waypoint.
 */
bool goto_tile_state(const struct tile *ptile, enum goto_tile_state *state,
                     int *turns, bool *waypoint)
{
  fc_assert_ret_val(ptile != nullptr, false);
  fc_assert_ret_val(turns != nullptr, false);
  fc_assert_ret_val(waypoint != nullptr, false);

  if (!goto_is_active() || goto_destination == nullptr) {
    return false;
  }

  *state = GTS_INVALID;
  *turns = GTS_INVALID;
  *waypoint = false;

  if (hover_state == HOVER_CONNECT) {
    // FIXME unsupported
  } else {
    *waypoint = false;
    *turns = -1;

    for (auto &[unit_id, finder] : goto_finders) {
      // Initial tile
      const auto unit = game_unit_by_number(unit_id);
      int last_turns = 0;
      if (unit->moves_left == 0) {
        last_turns = 1;
        if (ptile == unit->tile) {
          *state = GTS_TURN_STEP;
          *turns = 1;
        }
      }

      // Get a path
      const auto destination =
          hover_state == HOVER_PATROL ? unit->tile : goto_destination;
      const auto path =
          finder.find_path(freeciv::tile_destination(destination));
      if (path && !path->empty()) {
        const auto steps = path->steps();
        int last_waypoints = 0;
        // Find tiles on the path where we end turns
        for (const auto step : steps) {
          if (ptile == step.location) {
            *waypoint |= (step.waypoints > last_waypoints);
            if (step.turns > last_turns) {
              // Number of turns increased at this step
              *state = GTS_TURN_STEP;
              *turns = std::max(*turns, step.turns);
            }
          }
          last_turns = step.turns;
          last_waypoints = step.waypoints;
        }
        // Show end-of-path sprites (only when moving)
        if (ptile == steps.back().location) {
          if (*state == GTS_INVALID) { // Not set above => not a turn step
            *state = GTS_MP_LEFT;
          } else {
            *state = GTS_EXHAUSTED_MP;
          }
          *turns = std::max(*turns, steps.back().turns);
        }
      }
    }
  }

  return (*turns != -1 || *waypoint);
}

/**
   Puts a line to dest_tile on the map according to the current
   goto_map.
   If there is no route to the dest then don't draw anything.
 */
bool is_valid_goto_draw_line(struct tile *dest_tile)
{
  fc_assert_ret_val(goto_is_active(), false);
  if (nullptr == dest_tile) {
    return false;
  }

  mapdeco_clear_gotoroutes(); // We could be smarter here, but it's fast
                              // enough

  // assume valid destination
  goto_destination = dest_tile;
  for (auto &[unit_id, finder] : goto_finders) {
    auto destination = dest_tile;

    // Patrol is implemented by automatically adding a waypoint under the
    // cursor
    if (hover_state == HOVER_PATROL) {
      finder.pop_waypoint(); // Remove the last waypoint
      finder.push_waypoint(dest_tile);
      destination = game_unit_by_number(unit_id)->tile;
    }

    const auto path =
        finder.find_path(freeciv::tile_destination(destination));
    if (!path) {
      // This is our way of signalling that we can't go to a tile
      goto_destination = nullptr;
      continue;
    }

    // Show the path on the map
    auto unit = game_unit_by_number(unit_id);
    tile *previous_tile = unit_tile(unit);
    const auto first_unsafe =
        path->first_unsafe_step(game_unit_by_number(unit_id));
    const auto &steps = path->steps();
    for (auto it = steps.begin(); it != steps.end(); ++it) {
      const auto &step = *it;
      if (auto dir = direction8_invalid();
          previous_tile != nullptr
          && base_get_direction_for_step(&(wld.map), previous_tile,
                                         step.location, &dir)) {
        mapdeco_add_gotoline(step.location, opposite_direction(dir),
                             it < first_unsafe);
      }
      previous_tile = step.location;
    }
  }

  // Update goto data in info label.
  update_unit_info_label(get_units_in_focus());
  return (nullptr != goto_destination);
}

/**
   Send a packet to the server to request that the current orders be
   cleared.
 */
void request_orders_cleared(struct unit *punit)
{
  struct packet_unit_orders p;

  if (!can_client_issue_orders()) {
    return;
  }

  // Clear the orders by sending an empty orders path.
  qCDebug(goto_category, "Clearing orders for unit %d.", punit->id);
  p.unit_id = punit->id;
  p.src_tile = tile_index(unit_tile(punit));
  p.repeat = false;
  p.vigilant = false;
  p.length = 0;
  p.dest_tile = tile_index(unit_tile(punit));
  request_unit_ssa_set(punit, SSA_NONE);
  send_packet_unit_orders(&client.conn, &p);
}

/**
   Creates orders for a path as a goto or patrol route.
 */
static void make_path_orders(struct unit *punit, const PFPath &path,
                             enum unit_orders orders,
                             struct unit_order *final_order,
                             struct unit_order *order_list, int *length,
                             int *dest_tile)
{
  int i;
  struct tile *old_tile;

  fc_assert_ret(!path.empty());
  fc_assert_ret_msg(unit_tile(punit) == path[0].tile,
                    "Unit %d has moved without goto cancelation.",
                    punit->id);
  fc_assert_ret(length != nullptr);

  // We skip the start position.
  *length = path.length() - 1;
  fc_assert(*length < MAX_LEN_ROUTE);
  old_tile = path[0].tile;

  // If the path has n positions it takes n-1 steps.
  for (i = 0; i < path.length() - 1; i++) {
    struct tile *new_tile = path[i + 1].tile;

    if (same_pos(new_tile, old_tile)) {
      order_list[i].order = ORDER_FULL_MP;
      order_list[i].dir = DIR8_ORIGIN;
      order_list[i].activity = ACTIVITY_LAST;
      order_list[i].target = NO_TARGET;
      order_list[i].sub_target = NO_TARGET;
      order_list[i].action = ACTION_NONE;
      qCDebug(goto_category, "  packet[%d] = wait: %d,%d", i,
              TILE_XY(old_tile));
    } else {
      order_list[i].order = orders;
      order_list[i].dir = static_cast<direction8>(
          get_direction_for_step(&(wld.map), old_tile, new_tile));
      order_list[i].activity = ACTIVITY_LAST;
      order_list[i].target = NO_TARGET;
      order_list[i].sub_target = NO_TARGET;
      order_list[i].action = ACTION_NONE;
      qCDebug(goto_category, "  packet[%d] = move %s: %d,%d => %d,%d", i,
              dir_get_name(order_list[i].dir), TILE_XY(old_tile),
              TILE_XY(new_tile));
    }
    old_tile = new_tile;
  }

  if (i > 0 && order_list[i - 1].order == ORDER_MOVE
      && (is_non_allied_city_tile(old_tile, client_player()) != nullptr
          || is_non_allied_unit_tile(old_tile, client_player())
                 != nullptr)) {
    // Won't be able to perform a regular move to the target tile...
    if (!final_order) {
      /* ...and no final order exists. Choose what to do when the unit gets
       * there. */
      order_list[i - 1].order = ORDER_ACTION_MOVE;
    } else {
      /* ...and a final order exist. Can't assume an action move. Did the
       * caller hope that the situation would change before the unit got
       * there? */

      /* It's currently illegal to walk into tiles with non allied units or
       * cities. Some actions causes the actor to enter the target tile but
       * that is a part of the action it self, not a regular pre action
       * move. */
      qCDebug(goto_category, "unit or city blocks the path of your %s",
              unit_rule_name(punit));
    }
  }

  if (final_order) {
    // Append the final order after moving to the target tile.
    order_list[i].order = final_order->order;
    order_list[i].dir = final_order->dir;
    order_list[i].activity = (final_order->order == ORDER_ACTIVITY)
                                 ? final_order->activity
                                 : ACTIVITY_LAST;
    order_list[i].target = final_order->target;
    order_list[i].sub_target = final_order->sub_target;
    order_list[i].action = final_order->action;
    (*length)++;
  }

  if (dest_tile) {
    *dest_tile = tile_index(old_tile);
  }
}

/**
   Send a path as a goto or patrol route to the server.
 */
static void send_path_orders(struct unit *punit, const PFPath &path,
                             bool repeat, bool vigilant,
                             enum unit_orders orders,
                             struct unit_order *final_order)
{
  struct packet_unit_orders p;

  if (path.length() == 1 && final_order == nullptr) {
    return; // No path at all, no need to spam the server.
  }

  memset(&p, 0, sizeof(p));
  p.unit_id = punit->id;
  p.src_tile = tile_index(unit_tile(punit));
  p.repeat = repeat;
  p.vigilant = vigilant;

  qCDebug(goto_category, "Orders for unit %d:", punit->id);
  qCDebug(goto_category, "  Repeat: %d. Vigilant: %d.", p.repeat,
          p.vigilant);

  make_path_orders(punit, path, orders, final_order, p.orders, &p.length,
                   &p.dest_tile);

  request_unit_ssa_set(punit, SSA_NONE);
  send_packet_unit_orders(&client.conn, &p);
}

/**
   Send a path as a goto or patrol rally orders to the server.
 */
static void send_rally_path_orders(struct city *pcity, struct unit *punit,
                                   const PFPath &path, bool vigilant,
                                   enum unit_orders orders,
                                   struct unit_order *final_order)
{
  struct packet_city_rally_point p;

  memset(&p, 0, sizeof(p));
  p.city_id = pcity->id;
  p.vigilant = vigilant;

  qCDebug(goto_category, "Rally orders for city %d:", pcity->id);
  qCDebug(goto_category, "  Vigilant: %d.", p.vigilant);

  make_path_orders(punit, path, orders, final_order, p.orders, &p.length,
                   nullptr);

  send_packet_city_rally_point(&client.conn, &p);
}

/**
   Send an arbitrary goto path for the unit to the server.
 */
void send_goto_path(struct unit *punit, const PFPath &path,
                    struct unit_order *final_order)
{
  send_path_orders(punit, path, false, false, ORDER_MOVE, final_order);
}

/**
   Send an arbitrary rally path for the city to the server.
 */
void send_rally_path(struct city *pcity, struct unit *punit,
                     const PFPath &path, struct unit_order *final_order)
{
  send_rally_path_orders(pcity, punit, path, false, ORDER_MOVE, final_order);
}

/**
   Send orders for the unit to move it to the arbitrary tile.  Returns
   FALSE if no path is found.
 */
bool send_goto_tile(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;

  goto_fill_parameter_base(&parameter, punit);
  pfm = pf_map_new(&parameter);
  auto path = pf_map_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (!path.empty()) {
    send_goto_path(punit, path, nullptr);
    return true;
  } else {
    return false;
  }
}

/**
   Send rally orders for the city to move new units to the arbitrary tile.
   Returns FALSE if no path is found for the currently produced unit type.
 */
bool send_rally_tile(struct city *pcity, struct tile *ptile)
{
  const struct unit_type *putype;
  struct unit *punit;

  struct pf_parameter parameter;
  struct pf_map *pfm;

  fc_assert_ret_val(pcity != nullptr, false);
  fc_assert_ret_val(ptile != nullptr, false);

  // Create a virtual unit of the type being produced by the city.
  if (pcity->production.kind != VUT_UTYPE) {
    // Can only give orders to units.
    return false;
  }
  putype = pcity->production.value.utype;
  punit =
      unit_virtual_create(client_player(), pcity, putype,
                          city_production_unit_veteran_level(pcity, putype));

  // Use the unit to find a path to the destination tile.
  goto_fill_parameter_base(&parameter, punit);
  pfm = pf_map_new(&parameter);
  auto path = pf_map_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (!path.empty()) {
    // Send orders to server.
    send_rally_path(pcity, punit, path, nullptr);
    unit_virtual_destroy(punit);
    return true;
  } else {
    unit_virtual_destroy(punit);
    return false;
  }
}

/**
   Send orders for the unit to move it to the arbitrary tile and attack
   everything it approaches. Returns FALSE if no path is found.
 */
bool send_attack_tile(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;

  goto_fill_parameter_base(&parameter, punit);
  parameter.move_rate = 0;
  parameter.is_pos_dangerous = nullptr;
  parameter.get_moves_left_req = nullptr;
  pfm = pf_map_new(&parameter);
  auto path = pf_map_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (!path.empty()) {
    send_path_orders(punit, path, false, false, ORDER_ACTION_MOVE, nullptr);
    return true;
  }
  return false;
}

/**
   Send the current patrol route (i.e., the one generated via HOVER_STATE)
   to the server.
 */
void send_patrol_route() { send_goto_route(); }

/**
   Send the current connect route (i.e., the one generated via HOVER_STATE)
   to the server.
 */
void send_connect_route(enum unit_activity activity, struct extra_type *tgt)
{
  fc_assert_ret(goto_is_active());
  // FIXME unsupported
}

namespace /* anonymous */ {

/**
 * Returns true if the order must always be performed from an adjacent tile.
 */
bool order_requires_adjacent(unit_orders order, action_id action)
{
  switch (order) {
  case ORDER_MOVE:
  case ORDER_ACTION_MOVE:
    // A move is always done in a direction.
    return true;
  case ORDER_PERFORM_ACTION:
    // Always illegal to do to a target on the actor's own tile.
    if (!action_id_distance_accepted(action, 0)) {
      return true;
    }

    return false;
  default:
    return false;
  }
}

/**
 * Returns true if the order could be performed from an adjacent
 * tile.
 */
bool order_allows_adjacent(unit_orders order, action_id action)
{
  // Only ORDER_PERFORM_ACTION has complicated rules
  if (order != ORDER_PERFORM_ACTION) {
    return order_requires_adjacent(order, action);
  }

  // Can never be done from adjacent
  if (!action_id_distance_accepted(action, 1)) {
    return false;
  }

  return true;
}

/**
 * Modifies the last order(s) in the packet to account for goto_last_order.
 */
bool edit_last_order(packet_unit_orders &packet, const tile *tgt_tile)
{
  // The main complication here is that some actions must be performed from
  // the target tile and some actions must be performed from an adjacent tile
  // (or more). We always prefer performing the action from an adjacent tile,
  // and edit the path accordingly.

  // Is the last step in the path a move? Otherwise we can't assume it is
  // from an adjacent tile, so we won't generate
  const bool have_move =
      (packet.length > 0
       && (packet.orders[packet.length - 1].order == ORDER_MOVE
           || packet.orders[packet.length - 1].order == ORDER_ACTION_MOVE));

  // Do we want to start from an adjacent tile?
  bool adjacent =
      (have_move
       && order_allows_adjacent(goto_last_order, goto_last_action));

  // But, do we *need* to start from an adjacent tile?
  if (order_requires_adjacent(goto_last_order, goto_last_action)) {
    if (have_move) {
      // Must be performed from an adjacent tile
      adjacent = true;
    } else {
      // FIXME We could generate a longer path instead
      return false;
    }
  }

  if (!adjacent) {
    // If the last order was ORDER_ACTION_MOVE, convert it to ORDER_MOVE
    // Would happen e.g. for a caravan moving to an allied city
    if (packet.orders[packet.length - 1].order == ORDER_ACTION_MOVE) {
      packet.orders[packet.length - 1].order = ORDER_MOVE;
    }

    // Append an order after we reach the destination
    packet.length++;
  }

  // Edit the last order (the ORDER_[ACTION_]MOVE if adjacent, the one we
  // inserted if not)
  packet.orders[packet.length - 1] = unit_order{
      goto_last_order,
      ACTIVITY_LAST,
      (goto_last_tgt == NO_TARGET ? tgt_tile->index : goto_last_tgt),
      goto_last_sub_tgt,
      goto_last_action,
      DIR8_ORIGIN};

  return true;
}
} // anonymous namespace

/**
   Send the current goto route (i.e., the one generated via
   HOVER_STATE) to the server.  The route might involve more than one
   part if waypoints were used.
 */
void send_goto_route()
{
  fc_assert_ret(goto_is_active());
  fc_assert_ret(goto_destination != nullptr);

  for (auto &[unit_id, finder] : goto_finders) {
    auto destination = hover_state == HOVER_PATROL
                           ? game_unit_by_number(unit_id)->tile
                           : goto_destination;
    const auto path =
        finder.find_path(freeciv::tile_destination(destination));
    // No path to destination. Still try the other units...
    if (!path) {
      continue;
    }

    const auto unit = game_unit_by_number(unit_id);
    fc_assert_ret(unit != nullptr);

    auto packet = packet_unit_orders{};
    packet.unit_id = unit_id;
    packet.dest_tile = goto_destination->index;
    packet.src_tile = unit->tile->index;
    packet.repeat = hover_state == HOVER_PATROL;
    packet.vigilant = hover_state == HOVER_PATROL;

    const auto steps = path->steps();
    fc_assert_ret(steps.size() < MAX_LEN_ROUTE);

    packet.length = steps.size();
    for (std::size_t i = 0; i < steps.size(); ++i) {
      packet.orders[i] = steps[i].order;
    }

    if (goto_last_order != ORDER_LAST) {
      if (!edit_last_order(packet, destination)) {
        continue;
      }
    }

    // Send
    request_unit_ssa_set(unit, SSA_NONE);
    send_packet_unit_orders(&client.conn, &packet);
  }

  clear_hover_state();
  exit_goto_state();
}

/**
   Finds penultimate tile on path for given unit going to ptile
 */
struct tile *tile_before_end_path(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct tile *dtile;

  goto_fill_parameter_base(&parameter, punit);
  parameter.move_rate = 0;
  parameter.is_pos_dangerous = nullptr;
  parameter.get_moves_left_req = nullptr;
  pfm = pf_map_new(&parameter);
  auto path = pf_map_path(pfm, ptile);
  if (path.empty()) {
    return nullptr;
  }
  if (path.length() < 2) {
    dtile = nullptr;
  } else {
    dtile = path[-2].tile;
  }
  pf_map_destroy(pfm);

  return dtile;
}
