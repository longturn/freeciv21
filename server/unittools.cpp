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

#include <QDateTime>

#include <cstdlib>
#include <cstring>

// utility
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

// common
#include "base.h"
#include "city.h"
#include "combat.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "terrain.h"
#include "unit.h"
#include "unit_utils.h"
#include "unitlist.h"
#include "unittype.h"

// aicore
#include "path_finding.h"
#include "pf_tools.h"

/* server/scripting */
#include "script_server.h"

// server
#include "actiontools.h"
#include "aiiface.h"
#include "citytools.h"
#include "cityturn.h"
#include "gamehand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "sernet.h"
#include "srv_main.h"
#include "techtools.h"
#include "unithand.h"

/* server/advisors */
#include "advgoto.h"
#include "autoexplorer.h"
#include "autosettlers.h"

// ai
#include "handicaps.h"

#include "unittools.h"

/* Tools for controlling the client vision of every unit when a unit
 * moves + script effects. See unit_move(). You can access this data with
 * punit->server.moving; it may be nullptr if the unit is not moving). */
struct unit_move_data {
  int ref_count;
  struct unit *punit; // nullptr for invalidating.
  struct player *powner;
  bv_player can_see_unit;
  bv_player can_see_move;
  struct vision *old_vision;
};

#define SPECLIST_TAG unit_move_data
#include "speclist.h"
#define unit_move_data_list_iterate(_plist, _pdata)                         \
  TYPED_LIST_ITERATE(struct unit_move_data, _plist, _pdata)
#define unit_move_data_list_iterate_end LIST_ITERATE_END
#define unit_move_data_list_iterate_rev(_plist, _pdata)                     \
  TYPED_LIST_ITERATE_REV(struct unit_move_data, _plist, _pdata)
#define unit_move_data_list_iterate_rev_end LIST_ITERATE_REV_END

/* This data structure lets the auto attack code cache each potential
 * attacker unit's probability of success against the target unit during
 * the checks if the unit can do autoattack. It is then reused when the
 * list of potential attackers is sorted by probability of success. */
struct autoattack_prob {
  int unit_id;
  struct act_prob prob;
};

#define SPECLIST_TAG autoattack_prob
#define SPECLIST_TYPE struct autoattack_prob
#include "speclist.h"

#define autoattack_prob_list_iterate_safe(autoattack_prob_list, _aap_,      \
                                          _unit_)                           \
  TYPED_LIST_ITERATE(struct autoattack_prob, autoattack_prob_list, _aap_)   \
  struct unit *_unit_ = game_unit_by_number(_aap_->unit_id);                \
                                                                            \
  if (_unit_ == nullptr) {                                                  \
    continue;                                                               \
  }

#define autoattack_prob_list_iterate_safe_end LIST_ITERATE_END

static void unit_restore_movepoints(struct player *pplayer,
                                    struct unit *punit);
static void update_unit_activity(struct unit *punit);
static bool try_to_save_unit(struct unit *punit,
                             const struct unit_type *pttype, bool helpless,
                             bool teleporting, const struct city *pexclcity);
static void wakeup_neighbor_sentries(struct unit *punit);
static void do_upgrade_effects(struct player *pplayer);

static bool maybe_cancel_patrol_due_to_enemy(struct unit *punit);
static bool maybe_become_veteran_real(struct unit *punit, bool settler);

static void unit_transport_load_tp_status(struct unit *punit,
                                          struct unit *ptrans, bool force);

static void wipe_unit_full(struct unit *punit, bool transported,
                           enum unit_loss_reason reason,
                           struct player *killer);

/**
   Returns a unit type that matches the role_tech or role roles.

   If role_tech is given, then we look at all units with this role
   whose requirements are met by any player, and return a random one.  This
   can be used to give a unit to barbarians taken from the set of most
   advanced units researched by the 'real' players.

   If role_tech is not give (-1) or if there are no matching unit types,
   then we look at 'role' value and return a random matching unit type.

   It is an error if there are no available units.  This function will
   always return a valid unit.
 */
struct unit_type *find_a_unit_type(enum unit_role_id role,
                                   enum unit_role_id role_tech)
{
  struct unit_type *which[U_LAST];
  int i, num = 0;

  if (role_tech != -1) {
    for (i = 0; i < num_role_units(role_tech); i++) {
      struct unit_type *iunit = get_role_unit(role_tech, i);
      const int minplayers = 2;
      int players = 0;

      /* Note, if there's only one player in the game this check will always
       * fail. */
      players_iterate(pplayer)
      {
        if (!is_barbarian(pplayer)
            && can_player_build_unit_direct(pplayer, iunit)) {
          players++;
        }
      }
      players_iterate_end;
      if (players > minplayers) {
        which[num++] = iunit;
      }
    }
  }
  if (num == 0) {
    for (i = 0; i < num_role_units(role); i++) {
      which[num++] = get_role_unit(role, i);
    }
  }

  /* Ruleset code should ensure there is at least one unit for each
   * possibly-required role, or check before calling this function. */
  fc_assert_exit_msg(0 < num, "No unit types in find_a_unit_type(%d, %d)!",
                     role, role_tech);

  return which[fc_rand(num)];
}

/**
   Unit has a chance to become veteran. This should not be used for settlers
   for the work they do.
 */
bool maybe_make_veteran(struct unit *punit)
{
  return maybe_become_veteran_real(punit, false);
}

/**
   After a battle, after diplomatic aggression and after surviving trireme
   loss chance, this routine is called to decide whether or not the unit
   should become more experienced.

   There is a specified chance for it to happen, (+50% if player got SUNTZU)
   the chances are specified in the units.ruleset file.

   If 'settler' is TRUE the veteran level is increased due to work done by
   the unit.
 */
static bool maybe_become_veteran_real(struct unit *punit, bool settler)
{
  const struct veteran_system *vsystem;
  const struct veteran_level *vlevel;
  int chance;

  fc_assert_ret_val(punit != nullptr, false);

  vsystem = utype_veteran_system(unit_type_get(punit));
  fc_assert_ret_val(vsystem != nullptr, false);
  fc_assert_ret_val(vsystem->levels > punit->veteran, false);

  vlevel = utype_veteran_level(unit_type_get(punit), punit->veteran);
  fc_assert_ret_val(vlevel != nullptr, false);

  if (punit->veteran + 1 >= vsystem->levels
      || unit_has_type_flag(punit, UTYF_NO_VETERAN)) {
    return false;
  } else if (!settler) {
    int mod = 100 + get_unit_bonus(punit, EFT_VETERAN_COMBAT);

    /* The modification is tacked on as a multiplier to the base chance.
     * For example with a base chance of 50% for green units and a modifier
     * of +50% the end chance is 75%. */
    chance = vlevel->base_raise_chance * mod / 100;
  } else if (settler && unit_has_type_flag(punit, UTYF_SETTLERS)) {
    chance = vlevel->work_raise_chance;
  } else {
    // No battle and no work done.
    return false;
  }

  if (fc_rand(100) < chance) {
    punit->veteran++;
    return true;
  }

  return false;
}

/**
   This is the basic unit versus unit combat routine.
   1) ALOT of modifiers bonuses etc is added to the 2 units rates.
   2) the combat loop, which continues until one of the units are dead or
      EFT_COMBAT_ROUNDS rounds have been fought.
   3) the aftermath, the loser (and potentially the stack which is below it)
      is wiped, and the winner gets a chance of gaining veteran status
 */
void unit_versus_unit(struct unit *attacker, struct unit *defender,
                      int *att_hp, int *def_hp)
{
  int attackpower = get_total_attack_power(attacker, defender);
  int defensepower = get_total_defense_power(attacker, defender);
  int attack_firepower, defense_firepower;
  struct player *plr1 = unit_owner(attacker);
  struct player *plr2 = unit_owner(defender);
  int max_rounds;
  int rounds;

  *att_hp = attacker->hp;
  *def_hp = defender->hp;
  get_modified_firepower(attacker, defender, &attack_firepower,
                         &defense_firepower);

  qDebug("attack:%d, defense:%d, attack firepower:%d, "
         "defense firepower:%d",
         attackpower, defensepower, attack_firepower, defense_firepower);

  player_update_last_war_action(plr1);
  player_update_last_war_action(plr2);

  max_rounds = get_unit_bonus(attacker, EFT_COMBAT_ROUNDS);
  if (max_rounds <= 0) {
    if (attackpower == 0 || attack_firepower == 0) {
      *att_hp = 0;
    } else if (defensepower == 0 || defense_firepower == 0) {
      *def_hp = 0;
    }
  }
  for (rounds = 0; *att_hp > 0 && *def_hp > 0
                   && (max_rounds <= 0 || max_rounds > rounds);
       rounds++) {
    if (fc_rand(attackpower + defensepower) >= defensepower) {
      *def_hp -= attack_firepower;
    } else {
      *att_hp -= defense_firepower;
    }
  }
  if (*att_hp < 0) {
    *att_hp = 0;
  }
  if (*def_hp < 0) {
    *def_hp = 0;
  }
}

/**
   This is the basic unit versus unit classic bombardment routine.
   1) ALOT of modifiers bonuses etc is added to the 2 units rates.
   2) Do rate attacks and don't kill the defender, then return.
 */
void unit_bombs_unit(struct unit *attacker, struct unit *defender,
                     int *att_hp, int *def_hp)
{
  int i;
  int old_def_hp, bomb_limit_hp, bomb_limit_pct;
  int rate = unit_type_get(attacker)->bombard_rate;

  int attackpower = get_total_attack_power(attacker, defender);
  int defensepower = get_total_defense_power(attacker, defender);
  int attack_firepower, defense_firepower;
  struct player *plr1 = unit_owner(attacker);
  struct player *plr2 = unit_owner(defender);

  *att_hp = attacker->hp;
  old_def_hp = *def_hp = defender->hp;
  // don't reduce below maxHP * Bombard_Limit_Pct%, rounded up
  bomb_limit_pct = get_unit_bonus(defender, EFT_BOMBARD_LIMIT_PCT);
  if (bomb_limit_pct > 0)
    bomb_limit_hp =
        (unit_type_get(defender)->hp - 1) * bomb_limit_pct / 100 + 1;
  else
    bomb_limit_hp = 0;
  // if unit was already below limit, don't heal it back up to the limit
  bomb_limit_hp = MIN(bomb_limit_hp, old_def_hp);
  get_modified_firepower(attacker, defender, &attack_firepower,
                         &defense_firepower);

  qDebug("attack:%d, defense:%d, attack firepower:%d, "
         "defense firepower:%d, bomb limit:%d",
         attackpower, defensepower, attack_firepower, defense_firepower,
         bomb_limit_hp);

  player_update_last_war_action(plr1);
  player_update_last_war_action(plr2);

  for (i = 0; i < rate; i++) {
    if (fc_rand(attackpower + defensepower) >= defensepower) {
      *def_hp -= attack_firepower;
    }
  }

  // Don't kill the target.
  if (*def_hp < bomb_limit_hp) {
    *def_hp = bomb_limit_hp;
  }
}

/**
   Maybe make either side of combat veteran
 */
void combat_veterans(struct unit *attacker, struct unit *defender)
{
  if (attacker->hp <= 0 || defender->hp <= 0
      || !game.info.only_killing_makes_veteran) {
    if (attacker->hp > 0) {
      maybe_make_veteran(attacker);
    }
    if (defender->hp > 0) {
      maybe_make_veteran(defender);
    }
  }
}

/**
   Do unit auto-upgrades to players with the EFT_UNIT_UPGRADE effect
   (traditionally from Leonardo's Workshop).
 */
static void do_upgrade_effects(struct player *pplayer)
{
  int upgrades = get_player_bonus(pplayer, EFT_UPGRADE_UNIT);
  struct unit_list *candidates;

  if (upgrades <= 0) {
    return;
  }
  candidates = unit_list_new();

  unit_list_iterate(pplayer->units, punit)
  {
    /* We have to be careful not to strand units at sea, for example by
     * upgrading a frigate to an ironclad while it was carrying a unit. */
    if (UU_OK == unit_upgrade_test(punit, true)) {
      unit_list_prepend(candidates, punit); // Potential candidate :)
    }
  }
  unit_list_iterate_end;

  while (upgrades > 0 && unit_list_size(candidates) > 0) {
    /* Upgrade one unit.  The unit is chosen at random from the list of
     * available candidates. */
    int candidate_to_upgrade = fc_rand(unit_list_size(candidates));
    struct unit *punit = unit_list_get(candidates, candidate_to_upgrade);
    const struct unit_type *type_from = unit_type_get(punit);
    const struct unit_type *type_to =
        can_upgrade_unittype(pplayer, type_from);

    transform_unit(punit, type_to, true);
    notify_player(pplayer, unit_tile(punit), E_UNIT_UPGRADED, ftc_server,
                  _("%s was upgraded for free to %s."),
                  utype_name_translation(type_from), unit_link(punit));
    unit_list_remove(candidates, punit);
    upgrades--;
  }

  unit_list_destroy(candidates);
}

/**
   Return true if unit is about to finish converting to a unittype
   with more/no fuel, rescuing it from imminent death
 */
static bool converting_fuel_rescue(struct unit *punit)
{
  const struct unit_type *from_type, *to_type;

  if (punit->activity != ACTIVITY_CONVERT)
    /* not converting */
    return false;
  if (punit->activity_count + get_activity_rate_this_turn(punit)
      < action_id_get_act_time(ACTION_CONVERT, punit, unit_tile(punit),
                               punit->activity_target))
    /* won't be finished in time */
    return false;
  if (!unit_can_convert(punit))
    /* can't actually convert */
    return false;
  from_type = unit_type_get(punit);
  to_type = from_type->converted_to;
  if (utype_fuel(to_type) && utype_fuel(to_type) <= utype_fuel(from_type))
    /* to_type doesn't have more fuel, so conversion doesn't help */
    return false;
  /* we're saved! */
  return true;
}

/**
   1. Do Leonardo's Workshop upgrade if applicable.

   2. Restore/decrease unit hitpoints.

   3. Kill dead units.

   4. Rescue airplanes by returning them to base automatically.

   5. Decrease fuel of planes in the air.

   6. Refuel planes that are in bases.

   7. Kill planes that are out of fuel.
 */
void player_restore_units(struct player *pplayer)
{
  // 1) get Leonardo out of the way first:
  do_upgrade_effects(pplayer);

  unit_list_iterate_safe(pplayer->units, punit)
  {
    // 2) Modify unit hitpoints. Helicopters can even lose them.
    unit_restore_hitpoints(punit);

    // 3) Check that unit has hitpoints
    if (punit->hp <= 0) {
      /* This should usually only happen for heli units, but if any other
       * units get 0 hp somehow, catch them too.  --dwp  */
      /* if 'game.server.killunhomed' is activated unhomed units are slowly
       * killed; notify player here */
      if (!punit->homecity && 0 < game.info.killunhomed) {
        notify_player(pplayer, unit_tile(punit), E_UNIT_LOST_MISC,
                      ftc_server,
                      _("Your %s has run out of hit points "
                        "because it was not supported by a city."),
                      unit_tile_link(punit));
      } else {
        notify_player(pplayer, unit_tile(punit), E_UNIT_LOST_MISC,
                      ftc_server, _("Your %s has run out of hit points."),
                      unit_tile_link(punit));
      }

      wipe_unit(punit, ULR_HP_LOSS, nullptr);
      continue; // Continue iterating...
    }

    // 4) Rescue planes if needed
    if (utype_fuel(unit_type_get(punit))) {
      // Shall we emergency return home on the last vapors?

      /* I think this is strongly against the spirit of client goto.
       * The problem is (again) that here we know too much. -- Zamar */

      if (punit->fuel <= 1 && !is_unit_being_refueled(punit)
          && !converting_fuel_rescue(punit)) {
        struct unit *carrier;

        carrier = transporter_for_unit(punit);
        if (carrier) {
          unit_transport_load_tp_status(punit, carrier, false);
        } else {
          bool alive = true;

          struct pf_map *pfm;
          struct pf_parameter parameter;

          pft_fill_unit_parameter(&parameter, punit);
          parameter.omniscience = !has_handicap(pplayer, H_MAP);
          pfm = pf_map_new(&parameter);

          pf_map_move_costs_iterate(pfm, ptile, move_cost, true)
          {
            if (move_cost > punit->moves_left) {
              // Too far
              break;
            }

            if (is_airunit_refuel_point(ptile, pplayer, punit)) {
              PFPath path;
              int id = punit->id;

              /* Client orders may be running for this unit - if so
               * we free them before engaging goto. */
              free_unit_orders(punit);

              path = pf_map_path(pfm, ptile);

              alive = adv_follow_path(punit, path, ptile);

              if (!alive) {
                qCritical("rescue plane: unit %d died enroute!", id);
              } else if (!same_pos(unit_tile(punit), ptile)) {
                /* Enemy units probably blocked our route
                 * FIXME: We should try find alternative route around
                 * the enemy unit instead of just giving up and crashing. */
                log_debug("rescue plane: unit %d could not move to "
                          "refuel point!",
                          punit->id);
              }

              if (alive) {
                /* Clear activity. Unit info will be sent in the end of
                 * the function. */
                unit_activity_handling(punit, ACTIVITY_IDLE);
                adv_unit_new_task(punit, AUT_NONE, nullptr);
                punit->goto_tile = nullptr;

                if (!is_unit_being_refueled(punit)) {
                  carrier = transporter_for_unit(punit);
                  if (carrier) {
                    unit_transport_load_tp_status(punit, carrier, false);
                  }
                }

                notify_player(
                    pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                    _("Your %s has returned to refuel."), unit_link(punit));
              }
              break;
            }
          }
          pf_map_move_costs_iterate_end;
          pf_map_destroy(pfm);

          if (!alive) {
            // Unit died trying to move to refuel point.
            return;
          }
        }
      }

      // 5) Update fuel
      punit->fuel--;

      /* 6) Automatically refuel air units in cities, airbases, and
       *    transporters (carriers). */
      if (is_unit_being_refueled(punit)) {
        punit->fuel = utype_fuel(unit_type_get(punit));
      }
    }
  }
  unit_list_iterate_safe_end;

  // 7) Check if there are air units without fuel
  unit_list_iterate_safe(pplayer->units, punit)
  {
    if (punit->fuel <= 0 && utype_fuel(unit_type_get(punit))
        && !converting_fuel_rescue(punit)) {
      notify_player(pplayer, unit_tile(punit), E_UNIT_LOST_MISC, ftc_server,
                    _("Your %s has run out of fuel."),
                    unit_tile_link(punit));
      wipe_unit(punit, ULR_FUEL, nullptr);
    }
  }
  unit_list_iterate_safe_end;

  // Send all updates.
  unit_list_iterate(pplayer->units, punit)
  {
    send_unit_info(nullptr, punit);
  }
  unit_list_iterate_end;
}

/**
   Move points are trivial, only modifiers to the base value is if it's
   sea units and the player has certain wonders/techs. Then add veteran
   bonus, if any.
 */
static void unit_restore_movepoints(struct player *pplayer,
                                    struct unit *punit)
{
  punit->moves_left = unit_move_rate(punit);
  punit->done_moving = false;
}

/**
   Iterate through all units and update them.
 */
void update_unit_activities(struct player *pplayer)
{
  unit_list_iterate_safe(pplayer->units, punit)
  {
    update_unit_activity(punit);
  }
  unit_list_iterate_safe_end;
}

/**
   Iterate through all units and execute their orders.
 */
void execute_unit_orders(struct player *pplayer)
{
  unit_list_iterate_safe(pplayer->units, punit)
  {
    if (unit_has_orders(punit)) {
      execute_orders(punit, false);
    }
  }
  unit_list_iterate_safe_end;
}

/**
   Iterate through all units and remember their current activities.
 */
void finalize_unit_phase_beginning(struct player *pplayer)
{
  /* Remember activities only after all knock-on effects of unit activities
   * on other units have been resolved */
  unit_list_iterate(pplayer->units, punit)
  {
    punit->changed_from = punit->activity;
    punit->changed_from_target = punit->activity_target;
    punit->changed_from_count = punit->activity_count;
    send_unit_info(nullptr, punit);
  }
  unit_list_iterate_end;
}

/**
   Calculate the total amount of activity performed by all units on a tile
   for a given task and target.
 */
static int total_activity(struct tile *ptile, enum unit_activity act,
                          struct extra_type *tgt)
{
  int total = 0;
  bool tgt_matters = activity_requires_target(act);

  unit_list_iterate(ptile->units, punit)
  {
    if (punit->activity == act
        && (!tgt_matters || punit->activity_target == tgt)) {
      total += punit->activity_count;
    }
  }
  unit_list_iterate_end;

  return total;
}

/**
   Check the total amount of activity performed by all units on a tile
   for a given task.
 */
static bool total_activity_done(struct tile *ptile, enum unit_activity act,
                                struct extra_type *tgt)
{
  return total_activity(ptile, act, tgt)
         >= tile_activity_time(act, ptile, tgt);
}

/**
   Common notification for all experience levels.
 */
void notify_unit_experience(struct unit *punit)
{
  const struct veteran_system *vsystem;
  const struct veteran_level *vlevel;

  if (!punit) {
    return;
  }

  vsystem = utype_veteran_system(unit_type_get(punit));
  fc_assert_ret(vsystem != nullptr);
  fc_assert_ret(vsystem->levels > punit->veteran);

  vlevel = utype_veteran_level(unit_type_get(punit), punit->veteran);
  fc_assert_ret(vlevel != nullptr);

  notify_player(unit_owner(punit), unit_tile(punit), E_UNIT_BECAME_VET,
                ftc_server,
                // TRANS: Your <unit> became ... rank of <veteran level>.
                _("Your %s became more experienced and achieved the rank "
                  "of %s."),
                unit_link(punit), name_translation_get(&vlevel->name));
}

/**
   Convert a single unit to another type.
 */
static void unit_convert(struct unit *punit)
{
  const struct unit_type *to_type;
  const struct unit_type *from_type;

  from_type = unit_type_get(punit);
  to_type = from_type->converted_to;

  if (unit_can_convert(punit)) {
    transform_unit(punit, to_type, true);
    notify_player(unit_owner(punit), unit_tile(punit), E_UNIT_UPGRADED,
                  ftc_server, _("%s converted to %s."),
                  utype_name_translation(from_type),
                  utype_name_translation(to_type));
  } else {
    notify_player(unit_owner(punit), unit_tile(punit), E_UNIT_UPGRADED,
                  ftc_server, _("%s cannot be converted."),
                  utype_name_translation(from_type));
  }
}

/**
   Cancel all illegal activities done by units at the specified tile.
 */
void unit_activities_cancel_all_illegal(const struct tile *ptile)
{
  unit_list_iterate(ptile->units, punit2)
  {
    if (!can_unit_continue_current_activity(punit2)) {
      if (unit_has_orders(punit2)) {
        notify_player(unit_owner(punit2), unit_tile(punit2), E_UNIT_ORDERS,
                      ftc_server,
                      _("Orders for %s aborted because activity "
                        "is no longer available."),
                      unit_link(punit2));
        free_unit_orders(punit2);
      }

      set_unit_activity(punit2, ACTIVITY_IDLE);
      send_unit_info(nullptr, punit2);
    }
  }
  unit_list_iterate_end;
}

/**
   Progress settlers in their current tasks,
   and units that is pillaging.
   also move units that is on a goto.
   Restore unit move points (information needed for settler tasks)
 */
static void update_unit_activity(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  bool unit_activity_done = false;
  enum unit_activity activity = punit->activity;
  struct tile *ptile = unit_tile(punit);

  switch (activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_SENTRY:
  case ACTIVITY_GOTO:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_LAST:
    //  We don't need the activity_count for the above
    break;

  case ACTIVITY_FORTIFYING:
  case ACTIVITY_CONVERT:
    punit->activity_count += get_activity_rate_this_turn(punit);
    break;

  case ACTIVITY_POLLUTION:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_PILLAGE:
  case ACTIVITY_CULTIVATE:
  case ACTIVITY_PLANT:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_BASE:
  case ACTIVITY_GEN_ROAD:
    punit->activity_count += get_activity_rate_this_turn(punit);

    // settler may become veteran when doing something useful
    if (maybe_become_veteran_real(punit, true)) {
      notify_unit_experience(punit);
    }
    break;
  case ACTIVITY_OLD_ROAD:
  case ACTIVITY_OLD_RAILROAD:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_AIRBASE:
    fc_assert(false);
    break;
  };

  unit_restore_movepoints(pplayer, punit);

  switch (activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_SENTRY:
  case ACTIVITY_GOTO:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_CONVERT:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
    // no default, ensure all handled
    break;

  case ACTIVITY_EXPLORE:
    do_explore(punit);
    return;

  case ACTIVITY_PILLAGE:
    if (total_activity_done(ptile, ACTIVITY_PILLAGE,
                            punit->activity_target)) {
      destroy_extra(ptile, punit->activity_target);
      unit_activity_done = true;

      bounce_units_on_terrain_change(ptile);

      // Change vision if effects have changed.
      unit_list_refresh_vision(ptile->units);
    }
    break;

  case ACTIVITY_POLLUTION:
    /* TODO: Remove this fallback target setting when target always correctly
     *       set */
    if (punit->activity_target == nullptr) {
      punit->activity_target =
          prev_extra_in_tile(ptile, ERM_CLEANPOLLUTION, nullptr, punit);
    }
    if (total_activity_done(ptile, ACTIVITY_POLLUTION,
                            punit->activity_target)) {
      destroy_extra(ptile, punit->activity_target);
      unit_activity_done = true;
    }
    break;

  case ACTIVITY_FALLOUT:
    /* TODO: Remove this fallback target setting when target always correctly
     *       set */
    if (punit->activity_target == nullptr) {
      punit->activity_target =
          prev_extra_in_tile(ptile, ERM_CLEANFALLOUT, nullptr, punit);
    }
    if (total_activity_done(ptile, ACTIVITY_FALLOUT,
                            punit->activity_target)) {
      destroy_extra(ptile, punit->activity_target);
      unit_activity_done = true;
    }
    break;

  case ACTIVITY_BASE: {
    if (total_activity(ptile, ACTIVITY_BASE, punit->activity_target)
        >= tile_activity_time(ACTIVITY_BASE, ptile,
                              punit->activity_target)) {
      create_extra(ptile, punit->activity_target, unit_owner(punit));
      unit_activity_done = true;
    }
  } break;

  case ACTIVITY_GEN_ROAD: {
    if (total_activity(ptile, ACTIVITY_GEN_ROAD, punit->activity_target)
        >= tile_activity_time(ACTIVITY_GEN_ROAD, ptile,
                              punit->activity_target)) {
      create_extra(ptile, punit->activity_target, unit_owner(punit));
      unit_activity_done = true;
    }
  } break;

  case ACTIVITY_IRRIGATE:
  case ACTIVITY_MINE:
  case ACTIVITY_CULTIVATE:
  case ACTIVITY_PLANT:
  case ACTIVITY_TRANSFORM:
    if (total_activity_done(ptile, activity, punit->activity_target)) {
      struct terrain *old = tile_terrain(ptile);

      /* The function below could change the terrain. Therefore, we have to
       * check the terrain (which will also do a sanity check for the tile).
       */
      tile_apply_activity(ptile, activity, punit->activity_target);
      check_terrain_change(ptile, old);
      unit_activity_done = true;
    }
    break;

  case ACTIVITY_OLD_ROAD:
  case ACTIVITY_OLD_RAILROAD:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_AIRBASE:
    fc_assert(false);
    break;
  }

  if (unit_activity_done) {
    update_tile_knowledge(ptile);
    if (ACTIVITY_IRRIGATE == activity || ACTIVITY_MINE == activity
        || ACTIVITY_CULTIVATE == activity || ACTIVITY_PLANT == activity
        || ACTIVITY_TRANSFORM == activity) {
      /* FIXME: As we might probably do the activity again, because of the
       * terrain change cycles, we need to treat these cases separatly.
       * Probably ACTIVITY_TRANSFORM should be associated to its terrain
       * target, whereas ACTIVITY_IRRIGATE and ACTIVITY_MINE should only
       * used for extras. */
      unit_list_iterate(ptile->units, punit2)
      {
        if (punit2->activity == activity) {
          set_unit_activity(punit2, ACTIVITY_IDLE);
          send_unit_info(nullptr, punit2);
        }
      }
      unit_list_iterate_end;
    } else {
      unit_list_iterate(ptile->units, punit2)
      {
        if (!can_unit_continue_current_activity(punit2)) {
          set_unit_activity(punit2, ACTIVITY_IDLE);
          send_unit_info(nullptr, punit2);
        }
      }
      unit_list_iterate_end;
    }

    tile_changing_activities_iterate(act)
    {
      if (act == activity) {
        /* Some units nearby may not be able to continue their action,
         * such as building irrigation if we removed the only source
         * of water from them. */
        adjc_iterate(&(wld.map), ptile, ptile2)
        {
          unit_activities_cancel_all_illegal(ptile2);
        }
        adjc_iterate_end;
        break;
      }
    }
    tile_changing_activities_iterate_end;
  }

  if (activity == ACTIVITY_FORTIFYING) {
    if (punit->activity_count >= action_id_get_act_time(
            ACTION_FORTIFY, punit, ptile, punit->activity_target)) {
      set_unit_activity(punit, ACTIVITY_FORTIFIED);
      unit_activity_done = true;
    }
  }

  if (activity == ACTIVITY_CONVERT) {
    if (punit->activity_count >= action_id_get_act_time(
            ACTION_CONVERT, punit, ptile, punit->activity_target)) {
      unit_convert(punit);
      set_unit_activity(punit, ACTIVITY_IDLE);
      unit_activity_done = true;
    }
  }

  if (unit_activity_done) {
    if (activity == ACTIVITY_PILLAGE) {
      // Casus Belli for when the action is completed.
      /* TODO: is it more logical to put Casus_Belli_Success here, change
       * Casus_Belli_Complete to Casus_Belli_Successful_Beginning and
       * trigger it when an activity successfully has began? */
      action_consequence_complete(
          action_by_number(ACTION_PILLAGE), unit_owner(punit),
          tile_owner(unit_tile(punit)), unit_tile(punit),
          tile_link(unit_tile(punit)));
    }
  }
}

/**
   Forget the unit's last activity so that it can't be resumed. This is
   used for example when the unit moves or attacks.
 */
void unit_forget_last_activity(struct unit *punit)
{
  punit->changed_from = ACTIVITY_IDLE;
}

/**
   Return TRUE iff activity requires some sort of target to be specified by
   the client.
 */
bool unit_activity_needs_target_from_client(enum unit_activity activity)
{
  switch (activity) {
  case ACTIVITY_PILLAGE:
    // Can be set server side.
    return false;
  default:
    return activity_requires_target(activity);
  }
}

/**
   For some activities (currently only pillaging), the precise target can
   be assigned by the server rather than explicitly requested by the client.
   This function assigns a specific activity+target if the current
   settings are open-ended (otherwise leaves them unchanged).

   Please update unit_activity_needs_target_from_client() if you add server
   side unit activity target setting to more activities.
 */
void unit_assign_specific_activity_target(struct unit *punit,
                                          enum unit_activity *activity,
                                          struct extra_type **target)
{
  if (*activity == ACTIVITY_PILLAGE && *target == nullptr) {
    struct tile *ptile = unit_tile(punit);
    struct extra_type *tgt;

    bv_extras extras = *tile_extras(ptile);

    while ((tgt = get_preferred_pillage(extras))) {
      BV_CLR(extras, extra_index(tgt));

      if (can_unit_do_activity_targeted(punit, *activity, tgt)) {
        *target = tgt;
        return;
      }
    }
    // Nothing we can pillage here.
    *activity = ACTIVITY_IDLE;
  }
}

/**
   Find place to place partisans. Returns whether such spot was found, and
   if it has been found, dst_tile contains that tile.
 */
static bool find_a_good_partisan_spot(struct tile *pcenter,
                                      struct player *powner,
                                      struct unit_type *u_type,
                                      int sq_radius, struct tile **dst_tile)
{
  int bestvalue = 0;

  // coords of best tile in arg pointers
  circle_iterate(&(wld.map), pcenter, sq_radius, ptile)
  {
    int value;

    if (!is_native_tile(u_type, ptile)) {
      continue;
    }

    if (nullptr != tile_city(ptile)) {
      continue;
    }

    if (0 < unit_list_size(ptile->units)) {
      continue;
    }

    // City may not have changed hands yet; see place_partisans().
    value =
        get_virtual_defense_power(nullptr, u_type, powner, ptile, false, 0);
    value *= 10;

    if (tile_continent(ptile) != tile_continent(pcenter)) {
      value /= 2;
    }

    value -= fc_rand(value / 3);

    if (value > bestvalue) {
      *dst_tile = ptile;
      bestvalue = value;
    }
  }
  circle_iterate_end;

  return bestvalue > 0;
}

/**
   Place partisans for powner around pcenter (normally around a city).
 */
void place_partisans(struct tile *pcenter, struct player *powner, int count,
                     int sq_radius)
{
  struct tile *ptile = nullptr;
  struct unit_type *u_type = get_role_unit(L_PARTISAN, 0);

  while (count-- > 0
         && find_a_good_partisan_spot(pcenter, powner, u_type, sq_radius,
                                      &ptile)) {
    struct unit *punit;

    punit = create_unit(powner, ptile, u_type, 0, 0, -1);
    if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      punit->activity = ACTIVITY_FORTIFIED; // yes; directly fortified
      send_unit_info(nullptr, punit);
    }
  }
}

/**
   Teleport punit to city at cost specified. Returns success. Note that unit
   may die if it succesfully moves, i.e., even when return value is TRUE.
   (If specified cost is -1, then teleportation costs all movement.)
 */
bool teleport_unit_to_city(struct unit *punit, struct city *pcity,
                           int move_cost, bool verbose)
{
  struct tile *src_tile = unit_tile(punit), *dst_tile = pcity->tile;

  if (city_owner(pcity) == unit_owner(punit)) {
    qDebug("Teleported %s %s from (%d,%d) to %s",
           nation_rule_name(nation_of_unit(punit)), unit_rule_name(punit),
           TILE_XY(src_tile), city_name_get(pcity));
    if (verbose) {
      notify_player(unit_owner(punit), city_tile(pcity), E_UNIT_RELOCATED,
                    ftc_server, _("Teleported your %s to %s."),
                    unit_link(punit), city_link(pcity));
    }

    // Silently free orders since they won't be applicable anymore.
    free_unit_orders(punit);

    if (move_cost == -1) {
      move_cost = punit->moves_left;
    }
    unit_move(punit, dst_tile, move_cost, nullptr, false, false);

    return true;
  }
  return false;
}

/**
   Move or remove a unit due to stack conflicts. This function will try to
   find a random safe tile within a two tile distance of the unit's current
   tile and move the unit there. If no tiles are found, the unit is
   disbanded. If 'verbose' is TRUE, a message is sent to the unit owner
   regarding what happened.
 */
void bounce_unit(struct unit *punit, bool verbose)
{
  struct player *pplayer;
  struct tile *punit_tile;
  struct unit_list *pcargo_units;
  int count = 0;

  /* I assume that there are no topologies that have more than
   * (2d + 1)^2 tiles in the "square" of "radius" d. */
  const int DIST = 2;
  struct tile *tiles[(2 * DIST + 1) * (2 * DIST + 1)];

  if (!punit) {
    return;
  }

  pplayer = unit_owner(punit);
  punit_tile = unit_tile(punit);

  square_iterate(&(wld.map), punit_tile, DIST, ptile)
  {
    if (count >= ARRAY_SIZE(tiles)) {
      break;
    }

    if (ptile == punit_tile) {
      continue;
    }

    if (can_unit_survive_at_tile(&(wld.map), punit, ptile)
        && !is_non_allied_city_tile(ptile, pplayer)
        && !is_non_allied_unit_tile(ptile, pplayer)) {
      tiles[count++] = ptile;
    }
  }
  square_iterate_end;

  if (count == 0) {
    /* If no place unit can survive try the same with tiles the unit can just
     * exist inspite of losing health or fuel*/
    square_iterate(&(wld.map), punit_tile, DIST, ptile)
    {
      if (count >= ARRAY_SIZE(tiles)) {
        break;
      }

      if (ptile == punit_tile) {
        continue;
      }

      if (can_unit_exist_at_tile(&(wld.map), punit, ptile)
          && !is_non_allied_city_tile(ptile, pplayer)
          && !is_non_allied_unit_tile(ptile, pplayer)) {
        tiles[count++] = ptile;
      }
    }
    square_iterate_end;
  }

  if (count > 0) {
    struct tile *ptile = tiles[fc_rand(count)];

    if (verbose) {
      notify_player(pplayer, ptile, E_UNIT_RELOCATED, ftc_server,
                    // TRANS: A unit is moved to resolve stack conflicts.
                    _("Moved your %s."), unit_link(punit));
    }
    /* TODO: should a unit be able to bounce to a transport like is done
     * below? What if the unit can't legally enter the transport, say
     * because the transport is Unreachable and the unit doesn't have it in
     * its embarks field or because "Transport Embark" isn't enabled? Kept
     * like it was to preserve the old rules for now. -- Sveinung */
    unit_move(punit, ptile, 0, nullptr, true, false);
    return;
  }

  /* Didn't find a place to bounce the unit, going to disband it.
   * Try to bounce transported units. */
  if (0 < get_transporter_occupancy(punit)) {
    pcargo_units = unit_transport_cargo(punit);
    unit_list_iterate(pcargo_units, pcargo) { bounce_unit(pcargo, verbose); }
    unit_list_iterate_end;
  }

  if (verbose) {
    notify_player(pplayer, punit_tile, E_UNIT_LOST_MISC, ftc_server,
                  // TRANS: A unit is disbanded to resolve stack conflicts.
                  _("Disbanded your %s."), unit_tile_link(punit));
  }
  wipe_unit(punit, ULR_STACK_CONFLICT, nullptr);
}

/**
   Throw pplayer's units from non allied cities

   If verbose is true, pplayer gets messages about where each units goes.
 */
static void throw_units_from_illegal_cities(struct player *pplayer,
                                            bool verbose)
{
  struct tile *ptile;
  struct city *pcity;
  struct unit *ptrans;
  struct unit_list *pcargo_units;

  // Unload undesired units from transports, if possible.
  unit_list_iterate(pplayer->units, punit)
  {
    ptile = unit_tile(punit);
    pcity = tile_city(ptile);
    if (nullptr != pcity && !pplayers_allied(city_owner(pcity), pplayer)
        && 0 < get_transporter_occupancy(punit)) {
      pcargo_units = unit_transport_cargo(punit);
      unit_list_iterate(pcargo_units, pcargo)
      {
        if (!pplayers_allied(unit_owner(pcargo), pplayer)) {
          if (can_unit_exist_at_tile(&(wld.map), pcargo, ptile)) {
            unit_transport_unload_send(pcargo);
          }
        }
      }
      unit_list_iterate_end;
    }
  }
  unit_list_iterate_end;

  /* Bounce units except transported ones which will be bounced with their
   * transport. */
  unit_list_iterate_safe(pplayer->units, punit)
  {
    ptile = unit_tile(punit);
    pcity = tile_city(ptile);
    if (nullptr != pcity && !pplayers_allied(city_owner(pcity), pplayer)) {
      ptrans = unit_transport_get(punit);
      if (nullptr == ptrans || pplayer != unit_owner(ptrans)) {
        bounce_unit(punit, verbose);
      }
    }
  }
  unit_list_iterate_safe_end;

#ifdef FREECIV_DEBUG
  // Sanity check.
  unit_list_iterate(pplayer->units, punit)
  {
    ptile = unit_tile(punit);
    pcity = tile_city(ptile);
    fc_assert_msg(
        nullptr == pcity || pplayers_allied(city_owner(pcity), pplayer),
        "Failed to throw %s %d from %s %d (%d, %d)", unit_rule_name(punit),
        punit->id, city_name_get(pcity), pcity->id, TILE_XY(ptile));
  }
  unit_list_iterate_end;
#endif // FREECIV_DEBUG
}

/**
   For each pplayer's unit, check if we stack illegally, if so,
   bounce both players' units. If on ocean tile, bounce everyone but ships
   to avoid drowning. This function assumes that cities are clean.

   If verbose is true, the unit owner gets messages about where each
   units goes.
 */
static void resolve_stack_conflicts(struct player *pplayer,
                                    struct player *aplayer, bool verbose)
{
  unit_list_iterate_safe(pplayer->units, punit)
  {
    struct tile *ptile = unit_tile(punit);

    if (is_non_allied_unit_tile(ptile, pplayer)) {
      unit_list_iterate_safe(ptile->units, aunit)
      {
        if (unit_owner(aunit) == pplayer || unit_owner(aunit) == aplayer
            || !can_unit_survive_at_tile(&(wld.map), aunit, ptile)) {
          bounce_unit(aunit, verbose);
        }
      }
      unit_list_iterate_safe_end;
    }
  }
  unit_list_iterate_safe_end;
}

/**
   When in civil war or an alliance breaks there will potentially be units
   from both sides coexisting on the same squares.  This routine resolves
   this by first bouncing off non-allied units from their cities, then by
   bouncing both players' units in now illegal multiowner stacks.  To avoid
   drowning due to removal of transports, we bounce everyone (including
   third parties' units) from ocean tiles.

   If verbose is true, the unit owner gets messages about where each
   units goes.
 */
void resolve_unit_stacks(struct player *pplayer, struct player *aplayer,
                         bool verbose)
{
  throw_units_from_illegal_cities(pplayer, verbose);
  throw_units_from_illegal_cities(aplayer, verbose);

  resolve_stack_conflicts(pplayer, aplayer, verbose);
  resolve_stack_conflicts(aplayer, pplayer, verbose);
}

/**
   Returns the list of the units seen by 'pplayer' potentially seen only
   thanks to an alliance with 'aplayer'. The returned pointer is newly
   allocated and should be freed by the caller, using unit_list_destroy().
 */
struct unit_list *get_units_seen_via_ally(const struct player *pplayer,
                                          const struct player *aplayer)
{
  struct unit_list *seen_units = unit_list_new();

  // Anybody's units inside ally's cities
  city_list_iterate(aplayer->cities, pcity)
  {
    unit_list_iterate(city_tile(pcity)->units, punit)
    {
      if (can_player_see_unit(pplayer, punit)) {
        unit_list_append(seen_units, punit);
      }
    }
    unit_list_iterate_end;
  }
  city_list_iterate_end;

  // Ally's own units inside transports
  unit_list_iterate(aplayer->units, punit)
  {
    if (unit_transported(punit) && can_player_see_unit(pplayer, punit)) {
      unit_list_append(seen_units, punit);
    }
  }
  unit_list_iterate_end;

  /* Make sure the same unit is not added in multiple phases
   * (unit within transport in a city) */
  unit_list_unique(seen_units);

  return seen_units;
}

/**
   When two players cancel an alliance, a lot of units that were visible may
   no longer be visible (this includes units in transporters and cities).
   Call this function to inform the clients that these units are no longer
   visible. Pass the list of seen units returned by get_units_seen_via_ally()
   before alliance was broken up.
 */
void remove_allied_visibility(struct player *pplayer, struct player *aplayer,
                              const struct unit_list *seen_units)
{
  unit_list_iterate(seen_units, punit)
  {
    // We need to hide units previously seen by the client.
    if (!can_player_see_unit(pplayer, punit)) {
      unit_goes_out_of_sight(pplayer, punit);
    }
  }
  unit_list_iterate_end;

  city_list_iterate(aplayer->cities, pcity)
  {
    /* The player used to know what units were in these cities.  Now that he
     * doesn't, he needs to get a new short city packet updating the
     * occupied status. */
    if (map_is_known_and_seen(pcity->tile, pplayer, V_MAIN)) {
      send_city_info(pplayer, pcity);
    }
  }
  city_list_iterate_end;
}

/**
   Refresh units visibility of 'aplayer' for 'pplayer' after alliance have
   been contracted.
 */
void give_allied_visibility(struct player *pplayer, struct player *aplayer)
{
  unit_list_iterate(aplayer->units, punit)
  {
    if (can_player_see_unit(pplayer, punit)) {
      send_unit_info(pplayer->connections, punit);
    }
  }
  unit_list_iterate_end;
}

/**
   Can unit refuel on tile. Considers also carrier capacity on tile.
 */
bool is_airunit_refuel_point(const struct tile *ptile,
                             const struct player *pplayer,
                             const struct unit *punit)
{
  const struct unit_class *pclass;

  if (nullptr != is_non_allied_unit_tile(ptile, pplayer)) {
    return false;
  }

  if (nullptr != is_allied_city_tile(ptile, pplayer)) {
    return true;
  }

  pclass = unit_class_get(punit);
  if (nullptr != pclass->cache.refuel_bases) {
    const struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

    extra_type_list_iterate(pclass->cache.refuel_bases, pextra)
    {
      if (BV_ISSET(plrtile->extras, extra_index(pextra))) {
        return true;
      }
    }
    extra_type_list_iterate_end;
  }

  return unit_could_load_at(punit, ptile);
}

/**
   Really transforms a single unit to another type.

   This function performs no checks. You should perform the appropriate
   test first to check that the transformation is legal (test_unit_upgrade()
   or test_unit_convert()).

   is_free: Does unit owner need to pay upgrade price.

   Note that this function is strongly tied to unit.c:test_unit_upgrade().
 */
void transform_unit(struct unit *punit, const struct unit_type *to_unit,
                    bool is_free)
{
  struct player *pplayer = unit_owner(punit);
  const struct unit_type *old_type = punit->utype;
  int old_mr = unit_move_rate(punit);
  int old_hp = unit_type_get(punit)->hp;
  bv_player can_see_unit;

  BV_CLR_ALL(can_see_unit);
  players_iterate(oplayer)
  {
    if (can_player_see_unit(oplayer, punit)) {
      BV_SET(can_see_unit, player_index(oplayer));
    }
  }
  players_iterate_end;

  if (!is_free) {
    pplayer->economic.gold -=
        unit_upgrade_price(pplayer, unit_type_get(punit), to_unit);
  }

  punit->utype = to_unit;

  /* New type may not have the same veteran system, and we may want to
   * knock some levels off. */
  punit->veteran =
      MIN(punit->veteran, utype_veteran_system(to_unit)->levels - 1);
  if (is_free) {
    punit->veteran =
        MAX(punit->veteran - game.server.autoupgrade_veteran_loss, 0);
  } else {
    punit->veteran =
        MAX(punit->veteran - game.server.upgrade_veteran_loss, 0);
  }

  /* Scale HP and MP, rounding down. Be careful with integer arithmetic,
   * and don't kill the unit. unit_move_rate() is used to take into account
   * global effects like Magellan's Expedition. */
  punit->hp = MAX(punit->hp * unit_type_get(punit)->hp / old_hp, 1);
  if (old_mr == 0) {
    punit->moves_left = unit_move_rate(punit);
  } else {
    punit->moves_left = punit->moves_left * unit_move_rate(punit) / old_mr;
  }

  /* Update unit fuel state */
  if (utype_fuel(to_unit) && utype_fuel(old_type)) {
    /* Keep the amount of fuel used constant, except don't kill the unit */
    int delta = utype_fuel(to_unit) - utype_fuel(old_type);
    punit->fuel = MAX(punit->fuel + delta, 1);
  } else {
    /* When converting from a non-fuel unit, fully refuel */
    punit->fuel = utype_fuel(to_unit);
  }

  unit_forget_last_activity(punit);

  // update unit upkeep
  city_units_upkeep(game_city_by_number(punit->homecity));

  conn_list_do_buffer(pplayer->connections);

  unit_refresh_vision(punit);

  // unit may disappear for some players if vlayer changed
  players_iterate(oplayer)
  {
    if (BV_ISSET(can_see_unit, player_index(oplayer))
        && !can_player_see_unit(oplayer, punit)) {
      unit_goes_out_of_sight(oplayer, punit);
    }
  }
  players_iterate_end;

  CALL_PLR_AI_FUNC(unit_transformed, pplayer, punit, old_type);
  CALL_FUNC_EACH_AI(unit_info, punit);

  send_unit_info(nullptr, punit);
  conn_list_do_unbuffer(pplayer->connections);
}

/**
   Wrapper of the below
 */
struct unit *create_unit(struct player *pplayer, struct tile *ptile,
                         const struct unit_type *type, int veteran_level,
                         int homecity_id, int moves_left)
{
  return create_unit_full(pplayer, ptile, type, veteran_level, homecity_id,
                          moves_left, -1, nullptr);
}

/**
   Set carried goods for unit.
 */
void unit_get_goods(struct unit *punit)
{
  if (punit->homecity != 0) {
    struct city *home = game_city_by_number(punit->homecity);

    if (home != nullptr && game.info.goods_selection == GSM_LEAVING) {
      punit->carrying = goods_from_city_to_unit(home, punit);
    }
  }
}

/**
   Creates a unit, and set it's initial values, and put it into the right
   lists.
   If moves_left is less than zero, unit will get max moves.
 */
struct unit *create_unit_full(struct player *pplayer, struct tile *ptile,
                              const struct unit_type *type,
                              int veteran_level, int homecity_id,
                              int moves_left, int hp_left,
                              struct unit *ptrans)
{
  struct unit *punit =
      unit_virtual_create(pplayer, nullptr, type, veteran_level);
  struct city *pcity;

  // Register unit
  punit->id = identity_number();
  idex_register_unit(&wld, punit);

  fc_assert_ret_val(ptile != nullptr, nullptr);
  unit_tile_set(punit, ptile);

  pcity = game_city_by_number(homecity_id);
  if (utype_has_flag(type, UTYF_NOHOME)) {
    punit->homecity = 0; // none
  } else {
    punit->homecity = homecity_id;
  }

  if (hp_left >= 0) {
    // Override default full HP
    punit->hp = hp_left;
  }

  if (moves_left >= 0) {
    // Override default full MP
    punit->moves_left = MIN(moves_left, unit_move_rate(punit));
  }

  if (ptrans) {
    // Set transporter for unit.
    unit_transport_load_tp_status(punit, ptrans, false);
  } else {
    fc_assert_ret_val(
        !ptile || can_unit_exist_at_tile(&(wld.map), punit, ptile), nullptr);
  }

  /* Assume that if moves_left < 0 then the unit is "fresh",
   * and not moved; else the unit has had something happen
   * to it (eg, bribed) which we treat as equivalent to moved.
   * (Otherwise could pass moved arg too...)  --dwp */
  punit->moved = (moves_left >= 0);

  unit_list_prepend(pplayer->units, punit);
  unit_list_prepend(ptile->units, punit);
  if (pcity && !utype_has_flag(type, UTYF_NOHOME)) {
    fc_assert(city_owner(pcity) == pplayer);
    unit_list_prepend(pcity->units_supported, punit);
    // Refresh the unit's homecity.
    city_refresh(pcity);
    send_city_info(pplayer, pcity);
  }

  punit->server.vision = vision_new(pplayer, ptile);
  unit_refresh_vision(punit);

  send_unit_info(nullptr, punit);
  maybe_make_contact(ptile, unit_owner(punit));
  wakeup_neighbor_sentries(punit);

  // update unit upkeep
  city_units_upkeep(game_city_by_number(homecity_id));

  // The unit may have changed the available tiles in nearby cities.
  city_map_update_tile_now(ptile);
  sync_cities();

  unit_get_goods(punit);

  CALL_FUNC_EACH_AI(unit_created, punit);
  CALL_PLR_AI_FUNC(unit_got, pplayer, punit);

  return punit;
}

/**
   Set the call back to run when the server removes the unit.
 */
void unit_set_removal_callback(struct unit *punit,
                               void (*callback)(struct unit *punit))
{
  /* Tried to overwrite another call back. If this assertion is triggered
   * in a case where two call back are needed it may be time to support
   * more than one unit removal call back at a time. */
  fc_assert_ret(punit->server.removal_callback == nullptr);

  punit->server.removal_callback = callback;
}

/**
   Remove the call back so nothing runs when the server removes the unit.
 */
void unit_unset_removal_callback(struct unit *punit)
{
  punit->server.removal_callback = nullptr;
}

/**
   We remove the unit and see if it's disappearance has affected the homecity
   and the city it was in.
 */
static void server_remove_unit_full(struct unit *punit, bool transported,
                                    enum unit_loss_reason reason)
{
  struct packet_unit_remove packet;
  struct tile *ptile = unit_tile(punit);
  struct city *pcity = tile_city(ptile);
  struct city *phomecity = game_city_by_number(punit->homecity);
  struct unit *ptrans;
  struct player *pplayer = unit_owner(punit);

  // The unit is doomed.
  punit->server.dying = true;

#ifdef FREECIV_DEBUG
  unit_list_iterate(ptile->units, pcargo)
  {
    fc_assert(unit_transport_get(pcargo) != punit);
  }
  unit_list_iterate_end;
#endif // FREECIV_DEBUG

  CALL_PLR_AI_FUNC(unit_lost, pplayer, punit);
  CALL_FUNC_EACH_AI(unit_destroyed, punit);

  // Save transporter for updating below.
  ptrans = unit_transport_get(punit);
  // Unload unit.
  unit_transport_unload(punit);

  /* Since settlers plot in new cities in the minimap before they
     are built, so that no two settlers head towards the same city
     spot, we need to ensure this reservation is cleared should
     the settler disappear on the way. */
  adv_unit_new_task(punit, AUT_NONE, nullptr);

  /* Clear the vision before sending unit remove. Else, we might duplicate
   * the PACKET_UNIT_REMOVE if we lose vision of the unit tile. */
  vision_clear_sight(punit->server.vision);
  vision_free(punit->server.vision);
  punit->server.vision = nullptr;

  packet.unit_id = punit->id;
  // Send to onlookers.
  players_iterate(aplayer)
  {
    if (can_player_see_unit_at(aplayer, punit, unit_tile(punit),
                               transported)) {
      lsend_packet_unit_remove(aplayer->connections, &packet);
    }
  }
  players_iterate_end;
  // Send to global observers.
  conn_list_iterate(game.est_connections, pconn)
  {
    if (conn_is_global_observer(pconn)) {
      send_packet_unit_remove(pconn, &packet);
    }
  }
  conn_list_iterate_end;

  if (punit->server.moving != nullptr) {
    // Do not care of this unit for running moves.
    punit->server.moving->punit = nullptr;
  }

  if (punit->server.removal_callback != nullptr) {
    // Run the unit removal call back.
    punit->server.removal_callback(punit);
  }

  // check if this unit had UTYF_GAMELOSS flag
  if (unit_has_type_flag(punit, UTYF_GAMELOSS)
      && unit_owner(punit)->is_alive) {
    notify_conn(game.est_connections, ptile, E_UNIT_LOST_MISC, ftc_server,
                _("Unable to defend %s, %s has lost the game."),
                unit_link(punit), player_name(pplayer));
    notify_player(pplayer, ptile, E_GAME_END, ftc_server,
                  _("Losing %s meant losing the game! "
                    "Be more careful next time!"),
                  unit_link(punit));
    player_status_add(unit_owner(punit), PSTATUS_DYING);
  }

  script_server_signal_emit("unit_lost", punit, unit_owner(punit),
                            unit_loss_reason_name(reason));

  script_server_remove_exported_object(punit);
  game_remove_unit(&wld, punit);
  punit = nullptr;

  if (nullptr != ptrans) {
    // Update the occupy info.
    send_unit_info(nullptr, ptrans);
  }

  // This unit may have blocked tiles of adjacent cities. Update them.
  city_map_update_tile_now(ptile);
  sync_cities();

  if (phomecity) {
    city_refresh(phomecity);
    send_city_info(city_owner(phomecity), phomecity);
  }

  if (pcity && pcity != phomecity) {
    city_refresh(pcity);
    send_city_info(city_owner(pcity), pcity);
  }

  if (pcity && unit_list_size(ptile->units) == 0) {
    // The last unit in the city was killed: update the occupied flag.
    send_city_info(nullptr, pcity);
  }
}

/**
   We remove the unit and see if it's disappearance has affected the homecity
   and the city it was in.
 */
static void server_remove_unit(struct unit *punit,
                               enum unit_loss_reason reason)
{
  server_remove_unit_full(punit, unit_transported(punit), reason);
}

/**
   Handle units destroyed when their transport is destroyed
 */
static void unit_lost_with_transport(const struct player *pplayer,
                                     struct unit *pcargo,
                                     const struct unit_type *ptransport,
                                     struct player *killer)
{
  notify_player(pplayer, unit_tile(pcargo), E_UNIT_LOST_MISC, ftc_server,
                _("%s lost when %s was lost."), unit_tile_link(pcargo),
                utype_name_translation(ptransport));
  /* Unit is not transported any more at this point, but it has jumped
   * off the transport and drowns outside. So it must be removed from
   * all clients.
   * However, we don't know if given client has received ANY updates
   * about the swimming unit, and we can't remove it if it's not there
   * in the first place -> we send it once here just to be sure it's
   * there. */
  send_unit_info(nullptr, pcargo);
  wipe_unit_full(pcargo, false, ULR_TRANSPORT_LOST, killer);
}

/**
   Remove the unit, and passengers if it is a carrying any. Remove the
   _minimum_ number, eg there could be another boat on the square.
 */
static void wipe_unit_full(struct unit *punit, bool transported,
                           enum unit_loss_reason reason,
                           struct player *killer)
{
  struct tile *ptile = unit_tile(punit);
  struct player *pplayer = unit_owner(punit);
  const struct unit_type *putype_save =
      unit_type_get(punit); // for notify messages
  struct unit_list *helpless = unit_list_new();
  struct unit_list *imperiled = unit_list_new();
  struct unit_list *unsaved = unit_list_new();
  struct unit *ptrans = unit_transport_get(punit);
  struct city *pexclcity;

  // The unit is doomed.
  punit->server.dying = true;

  /* If a unit is being lost due to loss of its city, ensure that we don't
   * try to teleport any of its cargo to that city (which may not yet
   * have changed hands or disappeared). (It is assumed that the unit's
   * home city is always the one that is being lost/transferred/etc.) */
  if (reason == ULR_CITY_LOST) {
    pexclcity = unit_home(punit);
  } else {
    pexclcity = nullptr;
  }

  // Remove unit itself from its transport
  if (ptrans != nullptr) {
    unit_transport_unload(punit);
    send_unit_info(nullptr, ptrans);
  }

  // First pull all units off of the transporter.
  if (get_transporter_occupancy(punit) > 0) {
    /* Use iterate_safe as unloaded units will be removed from the list
     * while iterating. */
    unit_list_iterate_safe(unit_transport_cargo(punit), pcargo)
    {
      bool healthy = false;

      if (!can_unit_unload(pcargo, punit)) {
        unit_list_prepend(helpless, pcargo);
      } else {
        if (!can_unit_exist_at_tile(&(wld.map), pcargo, ptile)) {
          unit_list_prepend(imperiled, pcargo);
        } else {
          // These units do not need to be saved.
          healthy = true;
        }
      }

      /* Could use unit_transport_unload_send here, but that would
       * call send_unit_info for the transporter unnecessarily.
       * Note that this means that unit might to get seen briefly
       * by clients other than owner's, for example as a result of
       * update of homecity common to this cargo and some other
       * destroyed unit. */
      unit_transport_unload(pcargo);
      if (pcargo->activity == ACTIVITY_SENTRY) {
        /* Activate sentried units - like planes on a disbanded carrier.
         * Note this will activate ground units even if they just change
         * transporter. */
        set_unit_activity(pcargo, ACTIVITY_IDLE);
      }

      /* Unit info for unhealthy units will be sent when they are
       * assigned new transport or removed. */
      if (healthy) {
        send_unit_info(nullptr, pcargo);
      }
    }
    unit_list_iterate_safe_end;
  }

  // Now remove the unit.
  server_remove_unit_full(punit, transported, reason);

  switch (reason) {
  case ULR_KILLED:
  case ULR_EXECUTED:
  case ULR_SDI:
  case ULR_NUKE:
  case ULR_BRIBED:
  case ULR_CAPTURED:
  case ULR_CAUGHT:
  case ULR_ELIMINATED:
  case ULR_TRANSPORT_LOST:
    if (killer != nullptr) {
      killer->score.units_killed++;
    }
    pplayer->score.units_lost++;
    break;
  case ULR_BARB_UNLEASH:
  case ULR_CITY_LOST:
  case ULR_STARVED:
  case ULR_NONNATIVE_TERR:
  case ULR_ARMISTICE:
  case ULR_HP_LOSS:
  case ULR_FUEL:
  case ULR_STACK_CONFLICT:
  case ULR_SOLD:
    pplayer->score.units_lost++;
    break;
  case ULR_RETIRED:
  case ULR_DISBANDED:
  case ULR_USED:
  case ULR_EDITOR:
  case ULR_PLAYER_DIED:
  case ULR_DETONATED:
  case ULR_MISSILE:
    break;
  }

  // First, sort out helpless cargo.
  if (unit_list_size(helpless) > 0) {
    struct unit_list *remaining = unit_list_new();

    /* Grant priority to gameloss units and units with the EvacuateFirst
     * unit type flag. */
    unit_list_iterate_safe(helpless, pcargo)
    {
      if (unit_has_type_flag(pcargo, UTYF_EVAC_FIRST)
          || unit_has_type_flag(pcargo, UTYF_GAMELOSS)) {
        if (!try_to_save_unit(pcargo, putype_save, true,
                              unit_has_type_flag(pcargo, UTYF_EVAC_FIRST),
                              pexclcity)) {
          unit_list_prepend(unsaved, pcargo);
        }
      } else {
        unit_list_prepend(remaining, pcargo);
      }
    }
    unit_list_iterate_safe_end;

    // Handle non-priority units.
    unit_list_iterate_safe(remaining, pcargo)
    {
      if (!try_to_save_unit(pcargo, putype_save, true, false, pexclcity)) {
        unit_list_prepend(unsaved, pcargo);
      }
    }
    unit_list_iterate_safe_end;

    unit_list_destroy(remaining);
  }
  unit_list_destroy(helpless);

  // Then, save any imperiled cargo.
  if (unit_list_size(imperiled) > 0) {
    struct unit_list *remaining = unit_list_new();

    /* Grant priority to gameloss units and units with the EvacuateFirst
     * unit type flag. */
    unit_list_iterate_safe(imperiled, pcargo)
    {
      if (unit_has_type_flag(pcargo, UTYF_EVAC_FIRST)
          || unit_has_type_flag(pcargo, UTYF_GAMELOSS)) {
        if (!try_to_save_unit(pcargo, putype_save, false,
                              unit_has_type_flag(pcargo, UTYF_EVAC_FIRST),
                              pexclcity)) {
          unit_list_prepend(unsaved, pcargo);
        }
      } else {
        unit_list_prepend(remaining, pcargo);
      }
    }
    unit_list_iterate_safe_end;

    // Handle non-priority units.
    unit_list_iterate_safe(remaining, pcargo)
    {
      if (!try_to_save_unit(pcargo, putype_save, false, false, pexclcity)) {
        unit_list_prepend(unsaved, pcargo);
      }
    }
    unit_list_iterate_safe_end;

    unit_list_destroy(remaining);
  }
  unit_list_destroy(imperiled);

  // Finally, kill off the unsaved units.
  if (unit_list_size(unsaved) > 0) {
    unit_list_iterate_safe(unsaved, dying_unit)
    {
      unit_lost_with_transport(pplayer, dying_unit, putype_save, killer);
    }
    unit_list_iterate_safe_end;
  }
  unit_list_destroy(unsaved);
}

/**
   Remove the unit, and passengers if it is a carrying any. Remove the
   _minimum_ number, eg there could be another boat on the square.
 */
void wipe_unit(struct unit *punit, enum unit_loss_reason reason,
               struct player *killer)
{
  wipe_unit_full(punit, unit_transported(punit), reason, killer);
}

/**
   Determine if it is possible to save a given unit, and if so, save them.
   'pexclcity' is a city to avoid teleporting to, if 'teleporting' is set.
   Note that despite being saved from drowning, teleporting the units to
   "safety" may have killed them in the end.
 */
static bool try_to_save_unit(struct unit *punit,
                             const struct unit_type *pttype, bool helpless,
                             bool teleporting, const struct city *pexclcity)
{
  struct tile *ptile = unit_tile(punit);
  struct player *pplayer = unit_owner(punit);
  struct unit *ptransport = transporter_for_unit(punit);

  // Helpless units cannot board a transport in their current state.
  if (!helpless && ptransport != nullptr) {
    unit_transport_load_tp_status(punit, ptransport, false);
    send_unit_info(nullptr, punit);
    return true;
  } else {
    // Only units that cannot find transport are considered for teleport.
    if (teleporting) {
      struct city *pcity =
          find_closest_city(ptile, pexclcity, unit_owner(punit), false,
                            false, false, true, false, utype_class(pttype));
      if (pcity != nullptr) {
        char tplink[MAX_LEN_LINK]; // In case unit dies when teleported

        sz_strlcpy(tplink, unit_link(punit));

        if (teleport_unit_to_city(punit, pcity, 0, false)) {
          notify_player(
              pplayer, ptile, E_UNIT_RELOCATED, ftc_server,
              _("%s escaped the destruction of %s, and fled to %s."), tplink,
              utype_name_translation(pttype), city_link(pcity));
          return true;
        }
      }
    }
  }
  // The unit could not use transport on the tile, and could not teleport.
  return false;
}

/**
   We don't really change owner of the unit, but create completely new
   unit as its copy. The new pointer to 'punit' is returned.
 */
struct unit *unit_change_owner(struct unit *punit, struct player *pplayer,
                               int homecity, enum unit_loss_reason reason)
{
  struct unit *gained_unit;

  fc_assert(
      !utype_player_already_has_this_unique(pplayer, unit_type_get(punit)));

  /* Convert the unit to your cause. Fog is lifted in the create algorithm.
   */
  gained_unit = create_unit_full(
      pplayer, unit_tile(punit), unit_type_get(punit), punit->veteran,
      homecity, punit->moves_left, punit->hp, nullptr);

  // Owner changes, nationality not.
  gained_unit->nationality = punit->nationality;

  // Copy some more unit fields
  gained_unit->fuel = punit->fuel;
  gained_unit->paradropped = punit->paradropped;
  gained_unit->server.birth_turn = punit->server.birth_turn;

  send_unit_info(nullptr, gained_unit);

  // update unit upkeep in the homecity of the victim
  if (punit->homecity > 0) {
    // update unit upkeep
    city_units_upkeep(game_city_by_number(punit->homecity));
  }
  // update unit upkeep in the new homecity
  if (homecity > 0) {
    city_units_upkeep(game_city_by_number(homecity));
  }

  // Be sure to wipe the converted unit!
  wipe_unit(punit, reason, nullptr);

  return gained_unit; // Returns the replacement.
}

/**
   Called when one unit kills another in combat (this function is only
   called in one place).  It handles all side effects including
   notifications and killstack.
 */
void kill_unit(struct unit *pkiller, struct unit *punit, bool vet)
{
  char pkiller_link[MAX_LEN_LINK], punit_link[MAX_LEN_LINK];
  struct player *pvictim = unit_owner(punit);
  struct player *pvictor = unit_owner(pkiller);
  int ransom, unitcount = 0;
  bool escaped;

  sz_strlcpy(pkiller_link, unit_link(pkiller));
  sz_strlcpy(punit_link, unit_tile_link(punit));

  // The unit is doomed.
  punit->server.dying = true;

  if ((game.info.gameloss_style & GAMELOSS_STYLE_LOOT)
      && unit_has_type_flag(punit, UTYF_GAMELOSS)) {
    ransom = fc_rand(1 + pvictim->economic.gold);
    int n;

    // give map
    give_distorted_map(pvictim, pvictor, 50, true);

    log_debug("victim has money: %d", pvictim->economic.gold);
    pvictor->economic.gold += ransom;
    pvictim->economic.gold -= ransom;

    n = 1 + fc_rand(3);

    while (n > 0) {
      Tech_type_id ttid = steal_a_tech(pvictor, pvictim, A_UNSET);

      if (ttid == A_NONE) {
        log_debug("Worthless enemy doesn't have more techs to steal.");
        break;
      } else {
        log_debug("Pressed tech %s from captured enemy",
                  qUtf8Printable(research_advance_rule_name(
                      research_get(pvictor), ttid)));
        if (!fc_rand(3)) {
          break; // out of luck
        }
        n--;
      }
    }

    { // try to submit some cities
      int vcsize = city_list_size(pvictim->cities);
      int evcsize = vcsize;
      int conqsize = evcsize;

      if (evcsize < 3) {
        evcsize = 0;
      } else {
        evcsize -= 3;
      }
      // about a quarter on average with high numbers less probable
      conqsize = fc_rand(fc_rand(evcsize));

      log_debug("conqsize=%d", conqsize);

      if (conqsize > 0) {
        bool palace = game.server.savepalace;
        bool submit = false;

        game.server.savepalace = false; // moving it around is dumb

        city_list_iterate_safe(pvictim->cities, pcity)
        {
          // kindly ask the citizens to submit
          if (fc_rand(vcsize) < conqsize) {
            submit = true;
          }
          vcsize--;
          if (submit) {
            conqsize--;
            /* Transfer city to the victorious player
             * kill all its units outside of a radius of 7,
             * give verbose messages of every unit transferred,
             * and raze buildings according to raze chance
             * (also removes palace) */
            (void) transfer_city(pvictor, pcity, 7, true, true, true,
                                 !is_barbarian(pvictor));
            submit = false;
          }
          if (conqsize <= 0) {
            break;
          }
        }
        city_list_iterate_safe_end;
        game.server.savepalace = palace;
      }
    }
  }

  // barbarian leader ransom hack
  if (is_barbarian(pvictim) && unit_has_type_role(punit, L_BARBARIAN_LEADER)
      && (unit_list_size(unit_tile(punit)->units) == 1)
      && uclass_has_flag(unit_class_get(pkiller), UCF_COLLECT_RANSOM)) {
    // Occupying units can collect ransom if leader is alone in the tile
    ransom = (pvictim->economic.gold >= game.server.ransom_gold)
                 ? game.server.ransom_gold
                 : pvictim->economic.gold;
    notify_player(pvictor, unit_tile(pkiller), E_UNIT_WIN_ATT, ftc_server,
                  PL_("Barbarian leader captured; %d gold ransom paid.",
                      "Barbarian leader captured; %d gold ransom paid.",
                      ransom),
                  ransom);
    pvictor->economic.gold += ransom;
    pvictim->economic.gold -= ransom;
    send_player_info_c(pvictor, nullptr); // let me see my new gold :-)
    unitcount = 1;
  }

  if (unitcount == 0) {
    unit_list_iterate(unit_tile(punit)->units, vunit)
    {
      if (pplayers_at_war(pvictor, unit_owner(vunit))) {
        unitcount++;
      }
    }
    unit_list_iterate_end;
  }

  if (!is_stack_vulnerable(unit_tile(punit)) || unitcount == 1) {
    if (vet) {
      notify_unit_experience(pkiller);
    }
    wipe_unit(punit, ULR_KILLED, pvictor);
  } else { // unitcount > 1
    int i;
    int num_killed[player_slot_count()];
    int num_escaped[player_slot_count()];
    struct unit *other_killed[player_slot_count()];
    struct tile *ptile = unit_tile(punit);

    fc_assert(unitcount > 1);

    // initialize
    for (i = 0; i < player_slot_count(); i++) {
      num_killed[i] = 0;
      other_killed[i] = nullptr;
      num_escaped[i] = 0;
    }

    // count killed units
    unit_list_iterate(ptile->units, vunit)
    {
      struct player *vplayer = unit_owner(vunit);

      if (pplayers_at_war(pvictor, vplayer)
          && is_unit_reachable_at(vunit, pkiller, ptile)) {
        escaped = false;

        if (unit_has_type_flag(vunit, UTYF_CANESCAPE)
            && !unit_has_type_flag(pkiller, UTYF_CANKILLESCAPING)
            && vunit->hp > 0 && vunit->moves_left > pkiller->moves_left
            && fc_rand(2)) {
          int curr_def_bonus;
          int def_bonus = 0;
          struct tile *dsttile = nullptr;
          int move_cost;

          fc_assert(vunit->hp > 0);

          adjc_iterate(&(wld.map), ptile, ptile2)
          {
            if (can_exist_at_tile(&(wld.map), vunit->utype, ptile2)
                && nullptr == tile_city(ptile2)) {
              move_cost = map_move_cost_unit(&(wld.map), vunit, ptile2);
              if (pkiller->moves_left <= vunit->moves_left - move_cost
                  && (is_allied_unit_tile(ptile2, pvictim)
                      || unit_list_size(ptile2->units))
                         == 0) {
                curr_def_bonus =
                    tile_extras_defense_bonus(ptile2, vunit->utype);
                if (def_bonus <= curr_def_bonus) {
                  def_bonus = curr_def_bonus;
                  dsttile = ptile2;
                }
              }
            }
          }
          adjc_iterate_end;

          if (dsttile != nullptr) {
            /* TODO: Consider if forcing the unit to perform actions that
             * includes a move, like "Transport Embark", should be done when
             * a regular move is illegal or rather than a regular move. If
             * yes: remember to set action_requester to ACT_REQ_RULES. */
            move_cost = map_move_cost_unit(&(wld.map), vunit, dsttile);
            /* FIXME: Shouldn't unit_move_handling() be used here? This is
             * the unit escaping by moving itself. It should therefore
             * respect movement rules. */
            unit_move(vunit, dsttile, move_cost, nullptr, false, false);
            num_escaped[player_index(vplayer)]++;
            escaped = true;
            unitcount--;
          }
        }

        if (!escaped) {
          num_killed[player_index(vplayer)]++;

          if (vunit != punit) {
            other_killed[player_index(vplayer)] = vunit;
            other_killed[player_index(pvictor)] = vunit;
          }
        }
      }
    }
    unit_list_iterate_end;

    /* Inform the destroyer again if more than one unit was killed */
    if (unitcount > 1) {
      notify_player(pvictor, unit_tile(pkiller), E_UNIT_WIN_ATT, ftc_server,
                    /* TRANS: "... Cannon ... the Polish Destroyer ...." */
                    PL_("Your attacking %s succeeded against the %s %s "
                        "(and %d other unit)!",
                        "Your attacking %s succeeded against the %s %s "
                        "(and %d other units)!",
                        unitcount - 1),
                    pkiller_link, nation_adjective_for_player(pvictim),
                    punit_link, unitcount - 1);
    }

    if (vet) {
      notify_unit_experience(pkiller);
    }

    /* inform the owners: this only tells about owned units that were killed.
     * there may have been 20 units who died but if only 2 belonged to the
     * particular player they'll only learn about those.
     *
     * Also if a large number of units die you don't find out what type
     * they all are. */
    for (i = 0; i < player_slot_count(); i++) {
      if (num_killed[i] == 1) {
        if (i == player_index(pvictim)) {
          fc_assert(other_killed[i] == nullptr);
          notify_player(player_by_number(i), ptile, E_UNIT_LOST_DEF,
                        ftc_server,
                        // TRANS: "Cannon ... the Polish Destroyer."
                        _("%s lost to an attack by the %s %s."), punit_link,
                        nation_adjective_for_player(pvictor), pkiller_link);
        } else {
          fc_assert(other_killed[i] != punit);
          notify_player(player_by_number(i), ptile, E_UNIT_LOST_DEF,
                        ftc_server,
                        /* TRANS: "Cannon lost when the Polish Destroyer
                         * attacked the German Musketeers." */
                        _("%s lost when the %s %s attacked the %s %s."),
                        unit_link(other_killed[i]),
                        nation_adjective_for_player(pvictor), pkiller_link,
                        nation_adjective_for_player(pvictim), punit_link);
        }
      } else if (num_killed[i] > 1) {
        if (i == player_index(pvictim)) {
          int others = num_killed[i] - 1;

          if (others == 1) {
            notify_player(
                player_by_number(i), ptile, E_UNIT_LOST_DEF, ftc_server,
                /* TRANS: "Musketeers (and Cannon) lost to an
                 * attack from the Polish Destroyer." */
                _("%s (and %s) lost to an attack from the %s %s."),
                punit_link, unit_link(other_killed[i]),
                nation_adjective_for_player(pvictor), pkiller_link);
          } else {
            notify_player(
                player_by_number(i), ptile, E_UNIT_LOST_DEF, ftc_server,
                /* TRANS: "Musketeers and 3 other units lost to
                 * an attack from the Polish Destroyer."
                 * (only happens with at least 2 other units) */
                PL_("%s and %d other unit lost to an attack "
                    "from the %s %s.",
                    "%s and %d other units lost to an attack "
                    "from the %s %s.",
                    others),
                punit_link, others, nation_adjective_for_player(pvictor),
                pkiller_link);
          }
        } else {
          notify_player(
              player_by_number(i), ptile, E_UNIT_LOST_DEF, ftc_server,
              /* TRANS: "2 units lost when the Polish Destroyer
               * attacked the German Musketeers."
               * (only happens with at least 2 other units) */
              PL_("%d unit lost when the %s %s attacked the %s %s.",
                  "%d units lost when the %s %s attacked the %s %s.",
                  num_killed[i]),
              num_killed[i], nation_adjective_for_player(pvictor),
              pkiller_link, nation_adjective_for_player(pvictim),
              punit_link);
        }
      }
    }

    // Inform the owner of the units that escaped.
    for (i = 0; i < player_slot_count(); ++i) {
      if (0 < num_escaped[i]) {
        notify_player(player_by_number(i), unit_tile(punit), E_UNIT_ESCAPED,
                      ftc_server,
                      PL_("%d unit escaped from attack by %s %s",
                          "%d units escaped from attack by %s %s",
                          num_escaped[i]),
                      num_escaped[i], pkiller_link,
                      nation_adjective_for_player(pkiller->nationality));
      }
    }

    /* remove the units - note the logic of which units actually die
     * must be mimiced exactly in at least one place up above. */
    punit = nullptr; // wiped during following iteration so unsafe to use

    unit_list_iterate_safe(ptile->units, punit2)
    {
      if (pplayers_at_war(pvictor, unit_owner(punit2))
          && is_unit_reachable_at(punit2, pkiller, ptile)) {
        wipe_unit(punit2, ULR_KILLED, pvictor);
      }
    }
    unit_list_iterate_safe_end;
  }
}

/**
   Package a unit_info packet.  This packet contains basically all
   information about a unit.
 */
void package_unit(struct unit *punit, struct packet_unit_info *packet)
{
  packet->id = punit->id;
  packet->owner = player_number(unit_owner(punit));
  packet->nationality = player_number(unit_nationality(punit));
  packet->tile = tile_index(unit_tile(punit));
  packet->facing = punit->facing;
  packet->homecity = punit->homecity;
  fc_strlcpy(packet->name, punit->name.toUtf8(), ARRAY_SIZE(packet->name));
  output_type_iterate(o) { packet->upkeep[o] = punit->upkeep[o]; }
  output_type_iterate_end;
  packet->veteran = punit->veteran;
  packet->type = utype_number(unit_type_get(punit));
  packet->movesleft = punit->moves_left;
  packet->hp = punit->hp;
  packet->activity = punit->activity;
  packet->activity_count = punit->activity_count;

  if (punit->activity_target != nullptr) {
    packet->activity_tgt = extra_index(punit->activity_target);
  } else {
    packet->activity_tgt = EXTRA_NONE;
  }

  packet->changed_from = punit->changed_from;
  packet->changed_from_count = punit->changed_from_count;

  if (punit->changed_from_target != nullptr) {
    packet->changed_from_tgt = extra_index(punit->changed_from_target);
  } else {
    packet->changed_from_tgt = EXTRA_NONE;
  }

  packet->ssa_controller = punit->ssa_controller;
  packet->fuel = punit->fuel;
  packet->goto_tile =
      (nullptr != punit->goto_tile ? tile_index(punit->goto_tile) : -1);
  packet->paradropped = punit->paradropped;
  packet->done_moving = punit->done_moving;
  packet->stay = punit->stay;
  if (!unit_transported(punit)) {
    packet->transported = false;
    packet->transported_by = 0;
  } else {
    packet->transported = true;
    packet->transported_by = unit_transport_get(punit)->id;
  }
  if (punit->carrying != nullptr) {
    packet->carrying = goods_index(punit->carrying);
  } else {
    packet->carrying = -1;
  }
  packet->occupied = (get_transporter_occupancy(punit) > 0);
  packet->battlegroup = punit->battlegroup;
  packet->has_orders = punit->has_orders;
  if (punit->has_orders) {
    packet->orders_length = punit->orders.length;
    packet->orders_index = punit->orders.index;
    packet->orders_repeat = punit->orders.repeat;
    packet->orders_vigilant = punit->orders.vigilant;
    memcpy(packet->orders, punit->orders.list,
           punit->orders.length * sizeof(struct unit_order));
  } else {
    packet->orders_length = packet->orders_index = 0;
    packet->orders_repeat = packet->orders_vigilant = false;
    // No need to initialize array.
  }
  packet->action_turn = punit->action_turn;
  // client doesnt have access to game.server.unitwaittime, so use release
  // time
  time_t gotime = game.server.unitwaittime + punit->action_timestamp;
  // convert to UTC using Qt
  QDateTime localtime;
  localtime.setSecsSinceEpoch(gotime);
  QDateTime utctime(localtime.toUTC());
  gotime = utctime.toSecsSinceEpoch();
  packet->action_timestamp = gotime;
  packet->action_decision_want = punit->action_decision_want;
  packet->action_decision_tile =
      (punit->action_decision_tile ? tile_index(punit->action_decision_tile)
                                   : IDENTITY_NUMBER_ZERO);
}

/**
   Package a short_unit_info packet.  This contains a limited amount of
   information about the unit, and is sent to players who shouldn't know
   everything (like the unit's owner's enemies).
 */
void package_short_unit(struct unit *punit,
                        struct packet_unit_short_info *packet,
                        enum unit_info_use packet_use, int info_city_id)
{
  packet->packet_use = packet_use;
  packet->info_city_id = info_city_id;

  packet->id = punit->id;
  packet->owner = player_number(unit_owner(punit));
  packet->tile = tile_index(unit_tile(punit));
  packet->facing = punit->facing;
  packet->veteran = punit->veteran;
  packet->type = utype_number(unit_type_get(punit));
  packet->hp = punit->hp;
  packet->occupied = (get_transporter_occupancy(punit) > 0);
  fc_strlcpy(packet->name, punit->name.toUtf8(), ARRAY_SIZE(packet->name));
  if (punit->activity == ACTIVITY_EXPLORE
      || punit->activity == ACTIVITY_GOTO) {
    packet->activity = ACTIVITY_IDLE;
  } else {
    packet->activity = punit->activity;
  }

  if (punit->activity_target == nullptr) {
    packet->activity_tgt = EXTRA_NONE;
  } else {
    packet->activity_tgt = extra_index(punit->activity_target);
  }

  /* Transported_by information is sent to the client even for units that
   * aren't fully known.  Note that for non-allied players, any transported
   * unit can't be seen at all.  For allied players we have to know if
   * transporters have room in them so that we can load units properly. */
  if (!unit_transported(punit)) {
    packet->transported = false;
    packet->transported_by = 0;
  } else {
    packet->transported = true;
    packet->transported_by = unit_transport_get(punit)->id;
  }
}

/**
   Handle situation where unit goes out of player sight.
 */
void unit_goes_out_of_sight(struct player *pplayer, struct unit *punit)
{
  dlsend_packet_unit_remove(pplayer->connections, punit->id);
  if (punit->server.moving != nullptr) {
    // Update status of 'pplayer' vision for 'punit'.
    BV_CLR(punit->server.moving->can_see_unit, player_index(pplayer));
  }
}

/**
   Send the unit to the players who need the info.
   dest = nullptr means all connections (game.est_connections)
 */
void send_unit_info(struct conn_list *dest, struct unit *punit)
{
  const struct player *powner;
  struct packet_unit_info info;
  struct packet_unit_short_info sinfo;
  struct unit_move_data *pdata;

  if (dest == nullptr) {
    dest = game.est_connections;
  }

  CHECK_UNIT(punit);

  powner = unit_owner(punit);
  package_unit(punit, &info);
  package_short_unit(punit, &sinfo, UNIT_INFO_IDENTITY, 0);
  pdata = punit->server.moving;

  conn_list_iterate(dest, pconn)
  {
    struct player *pplayer = conn_get_player(pconn);

    // Be careful to consider all cases where pplayer is nullptr...
    if (pplayer == nullptr) {
      if (pconn->observer) {
        send_packet_unit_info(pconn, &info);
      }
    } else if (pplayer == powner) {
      send_packet_unit_info(pconn, &info);
      if (pdata != nullptr) {
        BV_SET(pdata->can_see_unit, player_index(pplayer));
      }
    } else if (can_player_see_unit(pplayer, punit)) {
      send_packet_unit_short_info(pconn, &sinfo, false);
      if (pdata != nullptr) {
        BV_SET(pdata->can_see_unit, player_index(pplayer));
      }
    }
  }
  conn_list_iterate_end;
}

/**
   For each specified connections, send information about all the units
   known to that player/conn.
 */
void send_all_known_units(struct conn_list *dest)
{
  conn_list_do_buffer(dest);
  conn_list_iterate(dest, pconn)
  {
    struct player *pplayer = pconn->playing;

    if (nullptr == pplayer && !pconn->observer) {
      continue;
    }

    players_iterate(unitowner)
    {
      unit_list_iterate(unitowner->units, punit)
      {
        send_unit_info(dest, punit);
      }
      unit_list_iterate_end;
    }
    players_iterate_end;
  }
  conn_list_iterate_end;
  conn_list_do_unbuffer(dest);
  flush_packets();
}

/**
   Nuke a square: 1) remove all units on the square, and 2) halve the
   size of the city on the square.

   If it isn't a city square or an ocean square then with 50% chance add
   some fallout, then notify the client about the changes.
 */
static void do_nuke_tile(struct player *pplayer, struct tile *ptile)
{
  struct city *pcity = nullptr;
  int pop_loss;

  pcity = tile_city(ptile);

  unit_list_iterate_safe(ptile->units, punit)
  {
    // unit in a city may survive
    if (pcity
        && fc_rand(100) < game.info.nuke_defender_survival_chance_pct) {
      continue;
    }
    notify_player(unit_owner(punit), ptile, E_UNIT_LOST_MISC, ftc_server,
                  _("Your %s was nuked by %s."), unit_tile_link(punit),
                  pplayer == unit_owner(punit)
                      ? _("yourself")
                      : nation_plural_for_player(pplayer));
    if (unit_owner(punit) != pplayer) {
      notify_player(pplayer, ptile, E_UNIT_WIN_ATT, ftc_server,
                    _("The %s %s was nuked."),
                    nation_adjective_for_player(unit_owner(punit)),
                    unit_tile_link(punit));
    }
    wipe_unit(punit, ULR_NUKE, pplayer);
  }
  unit_list_iterate_safe_end;

  if (pcity) {
    notify_player(city_owner(pcity), ptile, E_CITY_NUKED, ftc_server,
                  _("%s was nuked by %s."), city_link(pcity),
                  pplayer == city_owner(pcity)
                      ? _("yourself")
                      : nation_plural_for_player(pplayer));

    if (city_owner(pcity) != pplayer) {
      notify_player(pplayer, ptile, E_CITY_NUKED, ftc_server,
                    _("You nuked %s."), city_link(pcity));
    }

    city_built_iterate(pcity, pimprove)
    {
      /* only destroy regular improvements, not wonders, and not those
       * that are especially protected from sabotage (e.g. Walls) */
      if (is_improvement(pimprove) && pimprove->sabotage == 100
          && fc_rand(100) < (float(
                 get_player_bonus(pplayer, EFT_NUKE_IMPROVEMENT_PCT)))) {
        notify_player(city_owner(pcity), city_tile(pcity), E_CITY_NUKED,
                      ftc_server, _("%s destroyed in nuclear explosion!"),
                      improvement_name_translation(pimprove));
        city_remove_improvement(pcity, pimprove);
      }
    }
    city_built_iterate_end;

    pop_loss = (game.info.nuke_pop_loss_pct * city_size_get(pcity)) / 100;
    city_reduce_size(pcity, pop_loss, pplayer, "nuke");

    send_city_info(nullptr, pcity);
  }

  if (fc_rand(2) == 1) {
    struct extra_type *pextra;

    pextra = rand_extra_for_tile(ptile, EC_FALLOUT, false);
    if (pextra != nullptr && !tile_has_extra(ptile, pextra)) {
      tile_add_extra(ptile, pextra);
      update_tile_knowledge(ptile);
    }
  }
}

/**
   Nuke all the squares in a 3x3 square around the center of the explosion
   pplayer is the player that caused the explosion.
 */
void do_nuclear_explosion(struct player *pplayer, struct tile *ptile)
{
  square_iterate(&(wld.map), ptile, 1, ptile1)
  {
    do_nuke_tile(pplayer, ptile1);
  }
  square_iterate_end;

  script_server_signal_emit("nuke_exploded", 2, API_TYPE_TILE, ptile,
                            API_TYPE_PLAYER, pplayer);
  notify_conn(nullptr, ptile, E_NUKE, ftc_server,
              _("The %s detonated a nuke!"),
              nation_plural_for_player(pplayer));
}

/**
   Go by airline, if both cities have an airport and neither has been used
 this turn the unit will be transported by it and have its moves set to 0
 */
bool do_airline(struct unit *punit, struct city *pdest_city,
                const struct action *paction)
{
  struct city *psrc_city = tile_city(unit_tile(punit));

  notify_player(unit_owner(punit), city_tile(pdest_city), E_UNIT_RELOCATED,
                ftc_server, _("%s transported successfully."),
                unit_link(punit));

  unit_move(punit, pdest_city->tile, punit->moves_left, nullptr,
            // Can only airlift to allied and domestic cities
            false, false);

  // Update airlift fields.
  if (!(game.info.airlifting_style & AIRLIFTING_UNLIMITED_SRC)) {
    psrc_city->airlift--;
    send_city_info(city_owner(psrc_city), psrc_city);
  }
  if (!(game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST)) {
    pdest_city->airlift--;
    send_city_info(city_owner(pdest_city), pdest_city);
  }

  return true;
}

/**
   Autoexplore with unit.
 */
void do_explore(struct unit *punit)
{
  switch (manage_auto_explorer(punit)) {
  case MR_DEATH:
    // don't use punit!
    return;
  case MR_NOT_ALLOWED:
    // Needed for something else
    return;
  case MR_OK:
    /* FIXME: manage_auto_explorer() isn't supposed to change the activity,
     * but don't count on this.  See PR#39792.
     */
    if (punit->activity == ACTIVITY_EXPLORE) {
      break;
    }
    // fallthru
  default:
    unit_activity_handling(punit, ACTIVITY_IDLE);

    /* FIXME: When the manage_auto_explorer() call changes the activity from
     * EXPLORE to IDLE, in unit_activity_handling() ai.control is left
     * alone.  We reset it here.  See PR#12931. */
    punit->ssa_controller = SSA_NONE;
    break;
  }

  send_unit_info(nullptr, punit); // probably duplicate
}

/**
   Returns whether the drop was made or not. Note that it also returns 1
   in the case where the drop was succesful, but the unit was killed by
   barbarians in a hut.
 */
bool do_paradrop(struct unit *punit, struct tile *ptile,
                 const struct action *paction)
{
  struct player *pplayer = unit_owner(punit);

  // Hard requirements
  /* FIXME: hard requirements belong in common/actions's
   * is_action_possible() and the explanation texts belong in
   * server/unithand's action not enabled system (expl_act_not_enabl(),
   * ane_kind, explain_why_no_action_enabled(), etc)
   */
  if (!map_is_known_and_seen(ptile, pplayer, V_MAIN)) {
    // Only take in account values from player map.
    const struct player_tile *plrtile = map_get_player_tile(ptile, pplayer);

    if (nullptr == plrtile->site
        && !is_native_to_class(unit_class_get(punit), plrtile->terrain,
                               &(plrtile->extras))) {
      notify_player(pplayer, ptile, E_BAD_COMMAND, ftc_server,
                    _("This unit cannot paradrop into %s."),
                    terrain_name_translation(plrtile->terrain));
      return false;
    }

    if (nullptr != plrtile->site && plrtile->owner != nullptr
        && pplayers_non_attack(pplayer, plrtile->owner)) {
      notify_player(pplayer, ptile, E_BAD_COMMAND, ftc_server,
                    _("Cannot attack unless you declare war first."));
      return false;
    }
  }

  // Kill the unit when the landing goes wrong.

  // Safe terrain, really? Not transformed since player last saw it.
  if (!can_unit_exist_at_tile(&(wld.map), punit, ptile)
      && (!game.info.paradrop_to_transport
          || !unit_could_load_at(punit, ptile))) {
    map_show_circle(pplayer, ptile, unit_type_get(punit)->vision_radius_sq);
    notify_player(pplayer, ptile, E_UNIT_LOST_MISC, ftc_server,
                  _("Your %s paradropped into the %s and was lost."),
                  unit_tile_link(punit),
                  terrain_name_translation(tile_terrain(ptile)));
    pplayer->score.units_lost++;
    server_remove_unit(punit, ULR_NONNATIVE_TERR);
    return true;
  }

  if (is_non_attack_city_tile(ptile, pplayer)
      || (is_non_allied_city_tile(ptile, pplayer)
          && (pplayer->ai_common.barbarian_type == ANIMAL_BARBARIAN
              || !uclass_has_flag(unit_class_get(punit), UCF_CAN_OCCUPY_CITY)
              || unit_has_type_flag(punit, UTYF_CIVILIAN)))
      || is_non_allied_unit_tile(ptile, pplayer)) {
    map_show_circle(pplayer, ptile, unit_type_get(punit)->vision_radius_sq);
    maybe_make_contact(ptile, pplayer);
    notify_player(pplayer, ptile, E_UNIT_LOST_MISC, ftc_server,
                  _("Your %s was killed by enemy units at the "
                    "paradrop destination."),
                  unit_tile_link(punit));
    /* TODO: Should defender score.units_killed get increased too?
     * What if there's units of several allied players? Should the
     * city owner or owner of the first/random unit get the kill? */
    pplayer->score.units_lost++;
    server_remove_unit(punit, ULR_KILLED);
    return true;
  }

  // All ok
  punit->paradropped = true;
  if (unit_move(
          punit, ptile,
          // Done by Action_Success_Actor_Move_Cost
          0, nullptr, game.info.paradrop_to_transport,
          /* A paradrop into a non allied city results in a city
           * occupation. */
          /* FIXME: move the following actor requirements to the
           * ruleset. One alternative is to split "Paradrop Unit".
           * Another is to use different enablers. */
          (pplayer->ai_common.barbarian_type != ANIMAL_BARBARIAN
           && uclass_has_flag(unit_class_get(punit), UCF_CAN_OCCUPY_CITY)
           && !unit_has_type_flag(punit, UTYF_CIVILIAN)
           && is_non_allied_city_tile(ptile, pplayer)))) {
    // Ensure we finished on valid state.
    fc_assert(can_unit_exist_at_tile(&(wld.map), punit, unit_tile(punit))
              || unit_transported(punit));
  }

  return true;
}

/**
   Give 25 Gold or kill the unit. For H_LIMITEDHUTS
   Return TRUE if unit is alive, and FALSE if it was killed
 */
static bool hut_get_limited(struct unit *punit)
{
  bool ok = true;
  int hut_chance = fc_rand(12);
  struct player *pplayer = unit_owner(punit);
  // 1 in 12 to get barbarians
  if (hut_chance != 0) {
    int cred = 25;
    notify_player(pplayer, unit_tile(punit), E_HUT_GOLD, ftc_server,
                  PL_("You found %d gold.", "You found %d gold.", cred),
                  cred);
    pplayer->economic.gold += cred;
  } else if (city_exists_within_max_city_map(unit_tile(punit), true)
             || unit_has_type_flag(punit, UTYF_GAMELOSS)) {
    notify_player(pplayer, unit_tile(punit), E_HUT_BARB_CITY_NEAR,
                  ftc_server, _("An abandoned village is here."));
  } else {
    notify_player(pplayer, unit_tile(punit), E_HUT_BARB_KILLED, ftc_server,
                  _("Your %s has been killed by barbarians!"),
                  unit_tile_link(punit));
    wipe_unit(punit, ULR_BARB_UNLEASH, nullptr);
    ok = false;
  }
  return ok;
}

/**
   Due to the effects in the scripted hut behavior can not be predicted,
   unit_enter_hut returns nothing.
 */
static void unit_enter_hut(struct unit *punit)
{
  struct player *pplayer = unit_owner(punit);
  int id = punit->id;
  enum hut_behavior behavior = unit_class_get(punit)->hut_behavior;
  struct tile *ptile = unit_tile(punit);
  bool hut = false;

  if (behavior == HUT_NOTHING) {
    return;
  }

  extra_type_by_rmcause_iterate(ERM_ENTER, pextra)
  {
    if (tile_has_extra(ptile, pextra)
        && are_reqs_active(pplayer, tile_owner(ptile), nullptr, nullptr,
                           ptile, nullptr, nullptr, nullptr, nullptr,
                           nullptr, &pextra->rmreqs, RPT_CERTAIN)) {
      hut = true;
      // FIXME: are all enter-removes extras worth counting?
      pplayer->server.huts++;

      destroy_extra(ptile, pextra);
      update_tile_knowledge(unit_tile(punit));

      /* FIXME: enable different classes
       * to behave differently with different huts */
      if (behavior == HUT_FRIGHTEN) {
        script_server_signal_emit("hut_frighten", punit,
                                  extra_rule_name(pextra));
      } else if (is_ai(pplayer) && has_handicap(pplayer, H_LIMITEDHUTS)) {
        // AI with H_LIMITEDHUTS only gets 25 gold (or barbs if unlucky)
        (void) hut_get_limited(punit);
      } else {
        script_server_signal_emit("hut_enter", punit,
                                  extra_rule_name(pextra));
      }

      // We need punit for the callbacks, can't continue if the unit died
      if (!unit_is_alive(id)) {
        break;
      }
    }
  }
  extra_type_by_rmcause_iterate_end;

  if (hut) {
    send_player_info_c(pplayer, pplayer->connections); // eg, gold
  }
}

/**
   Put the unit onto the transporter, and tell everyone.
 */
void unit_transport_load_send(struct unit *punit, struct unit *ptrans)
{
  bv_player can_see_unit;

  fc_assert_ret(punit != nullptr);
  fc_assert_ret(ptrans != nullptr);

  BV_CLR_ALL(can_see_unit);
  players_iterate(pplayer)
  {
    if (can_player_see_unit(pplayer, punit)) {
      BV_SET(can_see_unit, player_index(pplayer));
    }
  }
  players_iterate_end;

  unit_transport_load(punit, ptrans, false);

  players_iterate(pplayer)
  {
    if (BV_ISSET(can_see_unit, player_index(pplayer))
        && !can_player_see_unit(pplayer, punit)) {
      unit_goes_out_of_sight(pplayer, punit);
    }
  }
  players_iterate_end;

  send_unit_info(nullptr, punit);
  send_unit_info(nullptr, ptrans);
}

/**
   Load unit to transport, send transport's loaded status to everyone.
 */
static void unit_transport_load_tp_status(struct unit *punit,
                                          struct unit *ptrans, bool force)
{
  bool had_cargo;

  fc_assert_ret(punit != nullptr);
  fc_assert_ret(ptrans != nullptr);

  had_cargo = get_transporter_occupancy(ptrans) > 0;

  unit_transport_load(punit, ptrans, force);

  if (!had_cargo) {
    // Transport's loaded status changed
    send_unit_info(nullptr, ptrans);
  }
}

/**
   Pull the unit off of the transporter, and tell everyone.
 */
void unit_transport_unload_send(struct unit *punit)
{
  struct unit *ptrans;

  fc_assert_ret(punit);

  ptrans = unit_transport_get(punit);

  fc_assert_ret(ptrans);

  unit_transport_unload(punit);

  send_unit_info(nullptr, punit);
  send_unit_info(nullptr, ptrans);
}

/**
   Used when unit_survive_autoattack()'s autoattack_prob_list
   autoattack frees its items.
 */
static void autoattack_prob_free(struct autoattack_prob *prob)
{
  delete prob;
}

/**
   This function is passed to autoattack_prob_list_sort() to sort a list of
   units and action probabilities according to their win chance against the
   autoattack target, modified by transportation relationships.

   The reason for making sure that a cargo unit is ahead of its
   transporter(s) is to leave transports out of combat if at all possible.
   (The transport could be destroyed during combat.)
 */
static int compare_units(const struct autoattack_prob *const *p1,
                         const struct autoattack_prob *const *q1)
{
  const struct unit *p1unit = game_unit_by_number((*p1)->unit_id);
  const struct unit *q1unit = game_unit_by_number((*q1)->unit_id);

  /* Sort by transport depth first. This makes sure that no transport
   * attacks before its cargo does -- cargo sorts earlier in the list. */
  {
    const struct unit *p1trans = p1unit, *q1trans = q1unit;

    /* Walk the transport stacks in parallel, so as to bail out as soon as
     * one of them is empty (avoid walking deep stacks more often than
     * necessary). */
    while (p1trans && q1trans) {
      p1trans = unit_transport_get(p1trans);
      q1trans = unit_transport_get(q1trans);
    }
    if (!p1trans && q1trans) {
      /* q1 is at greater depth (perhaps it's p1's cargo). It should sort
       * earlier in the list (p1 > q1). */
      return 1;
    } else if (p1trans && !q1trans) {
      // p1 is at greater depth, so should sort earlier (p1 < q1).
      return -1;
    }
    // else same depth, so move on to checking win chance:
  }

  /* Put the units with the highest probability of success first. The up
   * side of this is that units with bonuses against the victim attacks
   * before other units. The downside is that strong units can be led
   * away by sacrificial units. */
  return (-1
          // Assume the worst.
          * action_prob_cmp_pessimist((*p1)->prob, (*q1)->prob));
}

/**
   Check if unit survives enemy autoattacks. We assume that any
   unit that is adjacent to us can see us.
 */
static bool unit_survive_autoattack(struct unit *punit)
{
  struct autoattack_prob_list *autoattack;
  int moves = punit->moves_left;
  int sanity1 = punit->id;

  if (!game.server.autoattack) {
    return true;
  }

  autoattack = autoattack_prob_list_new_full(autoattack_prob_free);

  // Kludge to prevent attack power from dropping to zero during calc
  punit->moves_left = MAX(punit->moves_left, 1);

  adjc_iterate(&(wld.map), unit_tile(punit), ptile)
  {
    // First add all eligible units to a autoattack list
    unit_list_iterate(ptile->units, penemy)
    {
      auto *probability = new autoattack_prob;
      struct tile *tgt_tile = unit_tile(punit);

      fc_assert_action(tgt_tile, continue);

      probability->prob = action_auto_perf_unit_prob(
          AAPC_UNIT_MOVED_ADJ, penemy, unit_owner(punit), nullptr, tgt_tile,
          tile_city(tgt_tile), punit, nullptr);

      if (action_prob_possible(probability->prob)) {
        probability->unit_id = penemy->id;
        autoattack_prob_list_prepend(autoattack, probability);
      } else {
        delete probability;
        probability = nullptr;
      }
    }
    unit_list_iterate_end;
  }
  adjc_iterate_end;

  /* Sort the potential attackers from highest to lowest success
   * probability. */
  if (autoattack_prob_list_size(autoattack) >= 2) {
    autoattack_prob_list_sort(autoattack, &compare_units);
  }

  autoattack_prob_list_iterate_safe(autoattack, peprob, penemy)
  {
    int sanity2 = penemy->id;
    struct tile *ptile = unit_tile(penemy);
    struct unit *enemy_defender = get_defender(punit, ptile);
    double punitwin, penemywin;
    double threshold = 0.25;
    struct tile *tgt_tile = unit_tile(punit);

    fc_assert(tgt_tile);

    if (tile_city(ptile) && unit_list_size(ptile->units) == 1) {
      // Don't leave city defenseless
      threshold = 0.90;
    }

    if (nullptr != enemy_defender) {
      punitwin = unit_win_chance(punit, enemy_defender);
    } else {
      // 'penemy' can attack 'punit' but it may be not reciproque.
      punitwin = 1.0;
    }

    // Previous attacks may have changed the odds. Recalculate.
    peprob->prob = action_auto_perf_unit_prob(
        AAPC_UNIT_MOVED_ADJ, penemy, unit_owner(punit), nullptr, tgt_tile,
        tile_city(tgt_tile), punit, nullptr);

    if (!action_prob_possible(peprob->prob)) {
      // No longer legal.
      continue;
    }

    // Assume the worst.
    penemywin = action_prob_to_0_to_1_pessimist(peprob->prob);

    if ((penemywin > 1.0 - punitwin
         || unit_has_type_flag(punit, UTYF_PROVOKING))
        && penemywin > threshold) {
#ifdef REALLY_DEBUG_THIS
      log_test("AA %s -> %s (%d,%d) %.2f > %.2f && > %.2f",
               unit_rule_name(penemy), unit_rule_name(punit),
               TILE_XY(unit_tile(punit)), penemywin, 1.0 - punitwin,
               threshold);
#endif

      unit_activity_handling(penemy, ACTIVITY_IDLE);
      action_auto_perf_unit_do(AAPC_UNIT_MOVED_ADJ, penemy,
                               unit_owner(punit), nullptr, tgt_tile,
                               tile_city(tgt_tile), punit, nullptr);
    } else {
#ifdef REALLY_DEBUG_THIS
      log_test("!AA %s -> %s (%d,%d) %.2f > %.2f && > %.2f",
               unit_rule_name(penemy), unit_rule_name(punit),
               TILE_XY(unit_tile(punit)), penemywin, 1.0 - punitwin,
               threshold);
#endif
      continue;
    }

    if (game_unit_by_number(sanity2)) {
      send_unit_info(nullptr, penemy);
    }
    if (game_unit_by_number(sanity1)) {
      send_unit_info(nullptr, punit);
    } else {
      autoattack_prob_list_destroy(autoattack);
      return false; // moving unit dead
    }
  }
  autoattack_prob_list_iterate_safe_end;

  autoattack_prob_list_destroy(autoattack);
  if (game_unit_by_number(sanity1)) {
    // We could have lost movement in combat
    punit->moves_left = MIN(punit->moves_left, moves);
    send_unit_info(nullptr, punit);
    return true;
  } else {
    return false;
  }
}

/**
   Cancel orders for the unit.
 */
static void cancel_orders(struct unit *punit, const char *dbg_msg)
{
  free_unit_orders(punit);
  send_unit_info(nullptr, punit);
  log_debug("%s", dbg_msg);
}

/**
   Will wake up any neighboring enemy sentry units or patrolling
   units.
 */
static void wakeup_neighbor_sentries(struct unit *punit)
{
  bool alone_in_city;

  if (nullptr != tile_city(unit_tile(punit))) {
    int count = 0;

    unit_list_iterate(unit_tile(punit)->units, aunit)
    {
      // Consider only units not transported.
      if (!unit_transported(aunit)) {
        count++;
      }
    }
    unit_list_iterate_end;

    alone_in_city = (1 == count);
  } else {
    alone_in_city = false;
  }

  // There may be sentried units with a sightrange > sentry_range, but we
  // don't wake them up.
  square_iterate(&(wld.map), unit_tile(punit), game.control.sentry_range,
                 ptile)
  {
    unit_list_iterate(ptile->units, penemy)
    {
      int distance_sq = sq_map_distance(unit_tile(punit), ptile);
      int radius_sq = get_unit_vision_at(penemy, unit_tile(penemy), V_MAIN);

      if (!pplayers_allied(unit_owner(punit), unit_owner(penemy))
          && penemy->activity == ACTIVITY_SENTRY
          && radius_sq >= distance_sq
          /* If the unit moved on a city, and the unit is alone, consider
           * it is visible. */
          && (alone_in_city
              || can_player_see_unit(unit_owner(penemy), punit))
          // on board transport; don't awaken
          && can_unit_exist_at_tile(&(wld.map), penemy, unit_tile(penemy))) {
        set_unit_activity(penemy, ACTIVITY_IDLE);
        send_unit_info(nullptr, penemy);
        notify_player(unit_owner(penemy), unit_tile(punit), E_UNIT_ORDERS,
                      ftc_server, _("Sentried %s saw %s %s moving at %s"),
                      unit_tile_link(penemy),
                      nation_rule_name(nation_of_unit(punit)),
                      unit_link(punit), tile_link(punit->tile));
      }
    }
    unit_list_iterate_end;
  }
  square_iterate_end;

  // Wakeup patrolling units we bump into.
  square_iterate(&(wld.map), unit_tile(punit), game.control.sentry_range,
                 ptile)
  {
    unit_list_iterate(ptile->units, ppatrol)
    {
      if (punit != ppatrol && unit_has_orders(ppatrol)
          && ppatrol->orders.vigilant) {
        if (maybe_cancel_patrol_due_to_enemy(ppatrol)) {
          cancel_orders(ppatrol, "  stopping because of nearby enemy");
          notify_player(unit_owner(ppatrol), unit_tile(ppatrol),
                        E_UNIT_ORDERS, ftc_server,
                        _("Orders for %s aborted after enemy movement was "
                          "spotted."),
                        unit_link(ppatrol));
        }
      }
    }
    unit_list_iterate_end;
  }
  square_iterate_end;
}

/**
   Does: 1) updates the unit's homecity and the city it enters/leaves (the
            city's happiness varies). This also takes into account when the
            unit enters/leaves a fortress.
         2) updates adjacent cities' unavailable tiles.

   FIXME: Sometimes it is not necessary to send cities because the goverment
          doesn't care whether a unit is away or not.
 */
static bool unit_move_consequences(struct unit *punit, struct tile *src_tile,
                                   struct tile *dst_tile, bool passenger,
                                   bool conquer_city_allowed)
{
  struct city *fromcity = tile_city(src_tile);
  struct city *tocity = tile_city(dst_tile);
  struct city *homecity_start_pos = nullptr;
  struct city *homecity_end_pos = nullptr;
  int homecity_id_start_pos = punit->homecity;
  int homecity_id_end_pos = punit->homecity;
  struct player *pplayer_start_pos = unit_owner(punit);
  struct player *pplayer_end_pos = pplayer_start_pos;
  const struct unit_type *type_start_pos = unit_type_get(punit);
  const struct unit_type *type_end_pos = type_start_pos;
  bool refresh_homecity_start_pos = false;
  bool refresh_homecity_end_pos = false;
  int saved_id = punit->id;
  bool alive = true;

  if (tocity && conquer_city_allowed) {
    if (!passenger) {
      // The unit that does the move may conquer.
      unit_conquer_city(punit, tocity);
    }

    /* Run for passengers too. A passenger may have been killed when its
     * transport conquered a city. (unit_conquer_city() can cause Lua code
     * to run) */

    alive = unit_is_alive(saved_id);
    if (alive) {
      // In case script has changed something about unit
      pplayer_end_pos = unit_owner(punit);
      type_end_pos = unit_type_get(punit);
      homecity_id_end_pos = punit->homecity;
    }
  }

  if (homecity_id_start_pos != 0) {
    homecity_start_pos = game_city_by_number(homecity_id_start_pos);
  }
  if (homecity_id_start_pos != homecity_id_end_pos) {
    homecity_end_pos = game_city_by_number(homecity_id_end_pos);
  } else {
    homecity_end_pos = homecity_start_pos;
  }

  /* We only do refreshes for non-AI players to now make sure the AI turns
     doesn't take too long. Perhaps we should make a special refresh_city
     functions that only refreshed happines. */

  // might have changed owners or may be destroyed
  tocity = tile_city(dst_tile);

  if (tocity) { // entering a city
    if (tocity->owner == pplayer_end_pos) {
      if (tocity != homecity_end_pos && is_human(pplayer_end_pos)) {
        city_refresh(tocity);
        send_city_info(pplayer_end_pos, tocity);
      }
    }
    if (homecity_start_pos) {
      refresh_homecity_start_pos = true;
    }
  }

  if (fromcity) { // leaving a city
    if (homecity_start_pos) {
      refresh_homecity_start_pos = true;
    }
    if (fromcity != homecity_start_pos
        && fromcity->owner == pplayer_start_pos
        && is_human(pplayer_start_pos)) {
      city_refresh(fromcity);
      send_city_info(pplayer_start_pos, fromcity);
    }
  }

  /* entering/leaving a fortress or friendly territory */
  if (homecity_start_pos || homecity_end_pos) {
    if ((game.info.happyborders != HB_DISABLED
         && tile_owner(src_tile) != tile_owner(dst_tile))
        || (tile_has_base_flag_for_unit(dst_tile, type_end_pos,
                                        BF_NOT_AGGRESSIVE)
            && is_friendly_city_near(pplayer_end_pos, dst_tile))
        || (tile_has_base_flag_for_unit(src_tile, type_start_pos,
                                        BF_NOT_AGGRESSIVE)
            && is_friendly_city_near(pplayer_start_pos, src_tile))) {
      refresh_homecity_start_pos = true;
      refresh_homecity_end_pos = true;
    }
  }

  if (refresh_homecity_start_pos && is_human(pplayer_start_pos)) {
    city_refresh(homecity_start_pos);
    send_city_info(pplayer_start_pos, homecity_start_pos);
  }
  if (refresh_homecity_end_pos
      && (!refresh_homecity_start_pos
          || homecity_start_pos != homecity_end_pos)
      && is_human(pplayer_end_pos)) {
    city_refresh(homecity_end_pos);
    send_city_info(pplayer_end_pos, homecity_end_pos);
  }

  city_map_update_tile_now(dst_tile);
  sync_cities();

  return alive;
}

/**
   Check if the units activity is legal for a move , and reset it if
   it isn't.
 */
static void check_unit_activity(struct unit *punit)
{
  switch (punit->activity) {
  case ACTIVITY_IDLE:
  case ACTIVITY_SENTRY:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_GOTO:
    break;
  case ACTIVITY_POLLUTION:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_CULTIVATE:
  case ACTIVITY_PLANT:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_PILLAGE:
  case ACTIVITY_TRANSFORM:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_BASE:
  case ACTIVITY_GEN_ROAD:
  case ACTIVITY_CONVERT:
  case ACTIVITY_OLD_ROAD:
  case ACTIVITY_OLD_RAILROAD:
  case ACTIVITY_LAST:
    set_unit_activity(punit, ACTIVITY_IDLE);
    break;
  };
}

/**
   Create a new unit move data, or use previous one if available.
 */
static struct unit_move_data *unit_move_data(struct unit *punit,
                                             struct tile *psrctile,
                                             struct tile *pdesttile)
{
  struct unit_move_data *pdata;
  struct player *powner = unit_owner(punit);
  const v_radius_t radius_sq =
      V_RADIUS(get_unit_vision_at(punit, pdesttile, V_MAIN),
               get_unit_vision_at(punit, pdesttile, V_INVIS),
               get_unit_vision_at(punit, pdesttile, V_SUBSURFACE));
  struct vision *new_vision;
  bool success;

  if (punit->server.moving) {
    // Recursive moving (probably due to a script).
    pdata = punit->server.moving;
    pdata->ref_count++;
    fc_assert_msg(pdata->punit == punit,
                  "Unit number %d (%p) was going to die, but "
                  "server attempts to move it.",
                  punit->id, punit);
    fc_assert_msg(pdata->old_vision == nullptr,
                  "Unit number %d (%p) has done an incomplete move.",
                  punit->id, punit);
  } else {
    pdata = new struct unit_move_data;
    pdata->ref_count = 1;
    pdata->punit = punit;
    punit->server.moving = pdata;
    BV_CLR_ALL(pdata->can_see_unit);
  }
  pdata->powner = powner;
  BV_CLR_ALL(pdata->can_see_move);
  pdata->old_vision = punit->server.vision;

  // Remove unit from the source tile.
  fc_assert(unit_tile(punit) == psrctile);
  success = unit_list_remove(psrctile->units, punit);
  fc_assert(success == true);

  // Set new tile.
  unit_tile_set(punit, pdesttile);
  unit_list_prepend(pdesttile->units, punit);

  if (unit_transported(punit)) {
    // Silently free orders since they won't be applicable anymore.
    free_unit_orders(punit);
  }

  // Check unit activity.
  check_unit_activity(punit);
  unit_did_action(punit);
  unit_forget_last_activity(punit);

  /* We first unfog the destination, then send the move,
   * and then fog the old territory. This means that the player
   * gets a chance to see the newly explored territory while the
   * client moves the unit, and both areas are visible during the
   * move */

  // Enhance vision if unit steps into a fortress
  new_vision = vision_new(powner, pdesttile);
  punit->server.vision = new_vision;
  vision_change_sight(new_vision, radius_sq);
  ASSERT_VISION(new_vision);

  return pdata;
}

/**
   Decrease the reference counter and destroy if needed.
 */
static void unit_move_data_unref(struct unit_move_data *pdata)
{
  fc_assert_ret(pdata != nullptr);
  fc_assert_ret(pdata->ref_count > 0);
  fc_assert_msg(pdata->old_vision == nullptr,
                "Unit number %d (%p) has done an incomplete move.",
                pdata->punit != nullptr ? pdata->punit->id : -1,
                pdata->punit);

  pdata->ref_count--;
  if (pdata->ref_count == 0) {
    if (pdata->punit != nullptr) {
      fc_assert(pdata->punit->server.moving == pdata);
      pdata->punit->server.moving = nullptr;
    }
    delete pdata;
  }
}

/**
   Moves a unit. No checks whatsoever! This is meant as a practical
   function for other functions, like do_airline, which do the checking
   themselves.

   If you move a unit you should always use this function, as it also sets
   the transport status of the unit correctly. Note that the source tile (the
   current tile of the unit) and pdesttile need not be adjacent.

   Returns TRUE iff unit still alive.
 */
bool unit_move(struct unit *punit, struct tile *pdesttile, int move_cost,
               struct unit *embark_to, bool find_embark_target,
               bool conquer_city_allowed)
{
  struct player *pplayer;
  struct tile *psrctile;
  struct city *pcity;
  struct unit *ptransporter;
  struct packet_unit_info src_info, dest_info;
  struct packet_unit_short_info src_sinfo, dest_sinfo;
  struct unit_move_data_list *plist;
  struct unit_move_data *pdata;
  int saved_id;
  bool unit_lives;
  bool adj;
  enum direction8 facing;
  struct player *bowner;

  // Some checks.
  fc_assert_ret_val(punit != nullptr, false);
  fc_assert_ret_val(pdesttile != nullptr, false);

  plist = unit_move_data_list_new_full(unit_move_data_unref);
  pplayer = unit_owner(punit);
  saved_id = punit->id;
  psrctile = unit_tile(punit);
  adj =
      base_get_direction_for_step(&(wld.map), psrctile, pdesttile, &facing);

  conn_list_do_buffer(game.est_connections);

  // Unload the unit if on a transport.
  ptransporter = unit_transport_get(punit);
  if (ptransporter != nullptr) {
    // Unload unit _before_ setting the new tile!
    unit_transport_unload(punit);
    /* Send updated information to anyone watching that transporter
     * was unloading cargo. */
    send_unit_info(nullptr, ptransporter);
  }

  // Wakup units next to us before we move.
  wakeup_neighbor_sentries(punit);

  // Make info packets at 'psrctile'.
  if (adj) {
    /* If tiles are adjacent, we will show the move to users able
     * to see it. */
    package_unit(punit, &src_info);
    package_short_unit(punit, &src_sinfo, UNIT_INFO_IDENTITY, 0);
  }

  // Make new data for 'punit'.
  pdata = unit_move_data(punit, psrctile, pdesttile);
  unit_move_data_list_prepend(plist, pdata);

  // Set unit orientation
  if (adj) {
    // Only change orientation when moving to adjacent tile
    punit->facing = facing;
  }

  // Move magic.
  punit->moved = true;
  punit->moves_left = MAX(0, punit->moves_left - move_cost);
  if (punit->moves_left == 0 && !unit_has_orders(punit)) {
    // The next order may not require any remaining move fragments.
    punit->done_moving = true;
  }

  // No longer relevant.
  punit->action_decision_tile = nullptr;
  punit->action_decision_want = ACT_DEC_NOTHING;

  if (!adj && action_tgt_city(punit, pdesttile, false)) {
    /* The unit can perform an action to the city at the destination tile.
     * A long distance move (like an airlift) doesn't ask what action to
     * perform before moving. Ask now. */

    punit->action_decision_want = ACT_DEC_PASSIVE;
    punit->action_decision_tile = pdesttile;
  }

  // Claim ownership of fortress?
  bowner = extra_owner(pdesttile);
  if ((bowner == nullptr || pplayers_at_war(bowner, pplayer))
      && tile_has_claimable_base(pdesttile, unit_type_get(punit))) {
    /* Yes. We claim *all* bases if there's *any* claimable base(s).
     * Even if original unit cannot claim other kind of bases, the
     * first claimed base will have influence over other bases,
     * or something like that. */
    tile_claim_bases(pdesttile, pplayer);
  }

  // Move all contained units.
  unit_cargo_iterate(punit, pcargo)
  {
    pdata = unit_move_data(pcargo, psrctile, pdesttile);
    unit_move_data_list_append(plist, pdata);
  }
  unit_cargo_iterate_end;

  // Get data for 'punit'.
  pdata = unit_move_data_list_front(plist);

  /* Determine the players able to see the move(s), now that the player
   * vision has been increased. */
  if (adj) {
    /*  Main unit for adjacent move: the move is visible for every player
     * able to see on the matching unit layer. */
    enum vision_layer vlayer = unit_type_get(punit)->vlayer;

    players_iterate(oplayer)
    {
      if (map_is_known_and_seen(psrctile, oplayer, vlayer)
          || map_is_known_and_seen(pdesttile, oplayer, vlayer)) {
        BV_SET(pdata->can_see_unit, player_index(oplayer));
        BV_SET(pdata->can_see_move, player_index(oplayer));
      }
    }
    players_iterate_end;
  }
  unit_move_data_list_iterate(plist, pmove_data)
  {
    if (adj && pmove_data == pdata) {
      /* If positions are adjacent, we have already handled 'punit'. See
       * above. */
      continue;
    }

    players_iterate(oplayer)
    {
      if ((adj
           && can_player_see_unit_at(oplayer, pmove_data->punit, psrctile,
                                     pmove_data != pdata))
          || can_player_see_unit_at(oplayer, pmove_data->punit, pdesttile,
                                    pmove_data != pdata)) {
        BV_SET(pmove_data->can_see_unit, player_index(oplayer));
        BV_SET(pmove_data->can_see_move, player_index(oplayer));
      }
      if (can_player_see_unit_at(oplayer, pmove_data->punit, psrctile,
                                 pmove_data != pdata)) {
        /* The unit was seen with its source tile even if it was
         * teleported. */
        BV_SET(pmove_data->can_see_unit, player_index(oplayer));
      }
    }
    players_iterate_end;
  }
  unit_move_data_list_iterate_end;

  // Check timeout settings.
  if (current_turn_timeout() != 0 && game.server.timeoutaddenemymove > 0) {
    bool new_information_for_enemy = false;

    phase_players_iterate(penemy)
    {
      /* Increase the timeout if an enemy unit moves and the
       * timeoutaddenemymove setting is in use. */
      if (penemy->is_connected && pplayer != penemy
          && pplayers_at_war(pplayer, penemy)
          && BV_ISSET(pdata->can_see_move, player_index(penemy))) {
        new_information_for_enemy = true;
        break;
      }
    }
    phase_players_iterate_end;

    if (new_information_for_enemy) {
      increase_timeout_because_unit_moved();
    }
  }

  // Notifications of the move to the clients.
  if (adj) {
    /* Special case: 'punit' is moving to adjacent position. Then we show
     * 'punit' move to all users able to see 'psrctile' or 'pdesttile'. */

    // Make info packets at 'pdesttile'.
    package_unit(punit, &dest_info);
    package_short_unit(punit, &dest_sinfo, UNIT_INFO_IDENTITY, 0);

    conn_list_iterate(game.est_connections, pconn)
    {
      struct player *aplayer = conn_get_player(pconn);

      if (aplayer == nullptr) {
        if (pconn->observer) {
          // Global observers see all...
          send_packet_unit_info(pconn, &src_info);
          send_packet_unit_info(pconn, &dest_info);
        }
      } else if (BV_ISSET(pdata->can_see_move, player_index(aplayer))) {
        if (aplayer == pplayer) {
          send_packet_unit_info(pconn, &src_info);
          send_packet_unit_info(pconn, &dest_info);
        } else {
          send_packet_unit_short_info(pconn, &src_sinfo, false);
          send_packet_unit_short_info(pconn, &dest_sinfo, false);
        }
      }
    }
    conn_list_iterate_end;
  }

  // Other moves.
  unit_move_data_list_iterate(plist, pmove_data)
  {
    if (adj && pmove_data == pdata) {
      /* If positions are adjacent, we have already shown 'punit' move.
       * See above. */
      continue;
    }

    // Make info packets at 'pdesttile'.
    package_unit(pmove_data->punit, &dest_info);
    package_short_unit(pmove_data->punit, &dest_sinfo, UNIT_INFO_IDENTITY,
                       0);

    conn_list_iterate(game.est_connections, pconn)
    {
      struct player *aplayer = conn_get_player(pconn);

      if (aplayer == nullptr) {
        if (pconn->observer) {
          // Global observers see all...
          send_packet_unit_info(pconn, &dest_info);
        }
      } else if (BV_ISSET(pmove_data->can_see_move, player_index(aplayer))) {
        if (aplayer == pmove_data->powner) {
          send_packet_unit_info(pconn, &dest_info);
        } else {
          send_packet_unit_short_info(pconn, &dest_sinfo, false);
        }
      }
    }
    conn_list_iterate_end;
  }
  unit_move_data_list_iterate_end;

  // Clear old vision.
  unit_move_data_list_iterate(plist, pmove_data)
  {
    vision_clear_sight(pmove_data->old_vision);
    vision_free(pmove_data->old_vision);
    pmove_data->old_vision = nullptr;
  }
  unit_move_data_list_iterate_end;

  // Move consequences.
  unit_move_data_list_iterate(plist, pmove_data)
  {
    struct unit *aunit = pmove_data->punit;

    if (aunit != nullptr && unit_owner(aunit) == pmove_data->powner
        && unit_tile(aunit) == pdesttile) {
      (void) unit_move_consequences(aunit, psrctile, pdesttile,
                                    pdata != pmove_data,
                                    conquer_city_allowed);
    }
  }
  unit_move_data_list_iterate_end;

  unit_lives = (pdata->punit == punit);

  // Wakeup units and make contact.
  if (unit_lives) {
    wakeup_neighbor_sentries(punit);
  }
  maybe_make_contact(pdesttile, pplayer);

  if (unit_lives) {
    // Special checks for ground units in the ocean.
    if (embark_to
        || !can_unit_survive_at_tile(&(wld.map), punit, pdesttile)) {
      if (embark_to != nullptr) {
        ptransporter = embark_to;
      } else if (find_embark_target) {
        /* TODO: Consider to stop supporting find_embark_target and make all
         * callers that wants auto loading set embark_to. */
        ptransporter = transporter_for_unit(punit);
      } else {
        ptransporter = nullptr;
      }
      if (ptransporter) {
        unit_transport_load_tp_status(punit, ptransporter, false);

        // Set activity to sentry if boarding a ship.
        if (is_human(pplayer) && !unit_has_orders(punit)
            && punit->ssa_controller == SSA_NONE
            && !can_unit_exist_at_tile(&(wld.map), punit, pdesttile)) {
          set_unit_activity(punit, ACTIVITY_SENTRY);
        }

        send_unit_info(nullptr, punit);
      }
    }
  }

  // Remove units going out of sight.
  unit_move_data_list_iterate_rev(plist, pmove_data)
  {
    struct unit *aunit = pmove_data->punit;

    if (aunit == nullptr) {
      continue; // Died!
    }

    players_iterate(aplayer)
    {
      if (BV_ISSET(pmove_data->can_see_unit, player_index(aplayer))
          && !can_player_see_unit(aplayer, aunit)) {
        unit_goes_out_of_sight(aplayer, aunit);
      }
    }
    players_iterate_end;
  }
  unit_move_data_list_iterate_rev_end;

  /* Inform the owner's client about actor unit arrival. Can, depending on
   * the client settings, cause the client to start the process that makes
   * the action selection dialog pop up. */
  if ((pcity = tile_city(pdesttile))) {
    // Arrival in a city counts.

    unit_move_data_list_iterate(plist, pmove_data)
    {
      struct unit *ptrans;
      bool ok;
      struct unit *act_unit;
      struct player *act_player;

      act_unit = pmove_data->punit;
      act_player = unit_owner(act_unit);

      if (act_unit == nullptr || !unit_is_alive(act_unit->id)) {
        // The unit died before reaching this point.
        continue;
      }

      if (unit_tile(act_unit) != pdesttile) {
        // The unit didn't arrive at the destination tile.
        continue;
      }

      if (!is_human(act_player)) {
        // Only humans need reminders.
        continue;
      }

      if (!unit_transported(act_unit)) {
        /* Don't show the action selection dialog again. Non transported
         * units are handled before they move to the tile.  */
        continue;
      }

      /* Open action dialog only if 'act_unit' and all its transporters
       * (recursively) don't have orders. */
      if (unit_has_orders(act_unit)) {
        // The unit it self has orders.
        continue;
      }

      for (ptrans = unit_transport_get(act_unit);;
           ptrans = unit_transport_get(ptrans)) {
        if (nullptr == ptrans) {
          // No (recursive) transport has orders.
          ok = true;
          break;
        } else if (unit_has_orders(ptrans)) {
          // A unit transporting the unit has orders
          ok = false;
          break;
        }
      }

      if (!ok) {
        // A unit transporting act_unit has orders.
        continue;
      }

      if (action_tgt_city(act_unit, pdesttile, false)) {
        // There is a valid target.

        act_unit->action_decision_want = ACT_DEC_PASSIVE;
        act_unit->action_decision_tile = pdesttile;

        /* Let the client know that this unit wants the player to decide
         * what to do. */
        send_unit_info(player_reply_dest(act_player), act_unit);
      }
    }
    unit_move_data_list_iterate_end;
  }

  unit_move_data_list_destroy(plist);

  // Check cities at source and destination.
  if ((pcity = tile_city(psrctile))) {
    refresh_dumb_city(pcity);
  }
  if ((pcity = tile_city(pdesttile))) {
    refresh_dumb_city(pcity);
  }

  if (unit_lives) {
    // Let the scripts run ...
    script_server_signal_emit("unit_moved", punit, psrctile, pdesttile);
    unit_lives = unit_is_alive(saved_id);
  }

  if (unit_lives) {
    // Autoattack.
    unit_lives = unit_survive_autoattack(punit);
  }

  if (unit_lives) {
    // Is there a hut?
    unit_enter_hut(punit);
    unit_lives = unit_is_alive(saved_id);
  }

  conn_list_do_unbuffer(game.est_connections);

  if (unit_lives) {
    CALL_FUNC_EACH_AI(unit_move_seen, punit);
  }

  return unit_lives;
}

/**
   Maybe cancel the goto if there is an enemy in the way
 */
static bool maybe_cancel_goto_due_to_enemy(struct unit *punit,
                                           struct tile *ptile)
{
  return (is_non_allied_unit_tile(ptile, unit_owner(punit))
          || is_non_allied_city_tile(ptile, unit_owner(punit)));
}

/**
   Maybe cancel the patrol as there is an enemy near.

   If you modify the wakeup range you should change it in
   wakeup_neighbor_sentries() too.
 */
static bool maybe_cancel_patrol_due_to_enemy(struct unit *punit)
{
  bool cancel = false;
  int radius_sq = get_unit_vision_at(punit, unit_tile(punit), V_MAIN);
  struct player *pplayer = unit_owner(punit);

  circle_iterate(&(wld.map), unit_tile(punit), radius_sq, ptile)
  {
    struct unit *penemy = is_non_allied_unit_tile(ptile, pplayer);

    struct vision_site *pdcity = map_get_player_site(ptile, pplayer);

    if ((penemy && can_player_see_unit(pplayer, penemy))
        || (pdcity && !pplayers_allied(pplayer, vision_site_owner(pdcity))
            && pdcity->occupied)) {
      cancel = true;
      break;
    }
  }
  circle_iterate_end;

  return cancel;
}

/**
   Returns TRUE iff it is reasonable to assume that the player is wathing
   the unit.

   Since the player is watching the unit there is no need to inform him
   about things he could see happening. Remember that it still may
   be necessary to explain why something happened.
 */
static inline bool player_is_watching(struct unit *punit, const bool fresh)
{
  /* The player just sent the orders to the unit. The unit has moves left.
   * It is therefore safe to assume that the player already is paying
   * attention to the unit. */
  return fresh && punit->moves_left > 0;
}

/**
   Executes a unit's orders stored in punit->orders.  The unit is put on idle
   if an action fails or if "patrol" is set and an enemy unit is encountered.

   The return value will be TRUE if the unit lives, FALSE otherwise.  (This
   function used to return a goto_result enumeration, declared in gotohand.h.
   But this enumeration was never checked by the caller and just lead to
   confusion.  All the caller really needs to know is if the unit lived or
   died; everything else is handled internally within execute_orders.)

   If the orders are repeating the loop starts over at the beginning once it
   completes.  To avoid infinite loops on railroad we stop for this
   turn when the unit is back where it started, even if it have moves left.

   A unit will attack under orders only on its final action.

   The fresh parameter is true if the order execution happens because the
   orders just were received.
 */
bool execute_orders(struct unit *punit, const bool fresh)
{
  struct act_prob prob;
  bool performed;
  const char *name;
  bool res, last_order;
  int unitid = punit->id;
  struct player *pplayer = unit_owner(punit);
  int moves_made = 0;

  fc_assert_ret_val(unit_has_orders(punit), true);

  if (punit->activity != ACTIVITY_IDLE) {
    // Unit's in the middle of an activity; wait for it to finish.
    punit->done_moving = true;
    return true;
  }

  log_debug("Executing orders for %s %d", unit_rule_name(punit), punit->id);

  // Any time the orders are canceled we should give the player a message.

  while (true) {
    struct unit_order order;

    struct action *oaction;

    struct tile *dst_tile;
    struct city *tgt_city;
    struct unit *tgt_unit;
    int tgt_id;
    int sub_tgt_id;
    struct extra_type *pextra;

    if (punit->done_moving) {
      log_debug("  stopping because we're done this turn");
      return true;
    }

    if (punit->orders.vigilant && maybe_cancel_patrol_due_to_enemy(punit)) {
      // "Patrol" orders are stopped if an enemy is near.
      cancel_orders(punit, "  stopping because of nearby enemy");
      notify_player(pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                    _("Orders for %s aborted as there are units nearby."),
                    unit_link(punit));
      return true;
    }

    if (moves_made == punit->orders.length) {
      // For repeating orders, don't repeat more than once per turn.
      log_debug("  stopping because we ran a round");
      punit->done_moving = true;
      send_unit_info(nullptr, punit);
      return true;
    }
    moves_made++;

    order = punit->orders.list[punit->orders.index];

    /* An ORDER_PERFORM_ACTION that doesn't specify an action should not get
     * this far. */
    fc_assert_action((order.order != ORDER_PERFORM_ACTION
                      || action_id_exists(order.action)),
                     continue);

    switch (order.order) {
    case ORDER_MOVE:
    case ORDER_ACTION_MOVE:
    case ORDER_FULL_MP:
      if (0 == punit->moves_left) {
        log_debug("  stopping because of no more move points");
        return true;
      }
      break;
    case ORDER_PERFORM_ACTION:
      if (action_mp_full_makes_legal(punit, order.action)) {
        log_debug("  stopping. Not enough move points this turn");
        return true;
      }
      break;
    case ORDER_ACTIVITY:
    case ORDER_LAST:
      // Those actions don't require moves left.
      break;
    }

    last_order = (!punit->orders.repeat
                  && punit->orders.index + 1 == punit->orders.length);

    if (last_order) {
      /* Clear the orders before we engage in the move.  That way any
       * has_orders checks will yield FALSE and this will be treated as
       * a normal move.  This is important: for instance a caravan goto
       * will popup the caravan dialog on the last move only. */
      free_unit_orders(punit);
    }

    /* Advance the orders one step forward.  This is needed because any
     * updates sent to the client as a result of the action should include
     * the new index value.  Note that we have to send_unit_info somewhere
     * after this point so that the client is properly updated. */
    punit->orders.index++;

    switch (order.order) {
    case ORDER_FULL_MP:
      if (punit->moves_left < unit_move_rate(punit)) {
        /* If the unit doesn't have full MP then it just waits until the
         * next turn.  We assume that the next turn it will have full MP
         * (there's no check for that). */
        punit->done_moving = true;
        log_debug("  waiting this turn");
        send_unit_info(nullptr, punit);
      }
      break;
    case ORDER_ACTIVITY: {
      enum unit_activity activity = order.activity;

      fc_assert(activity == ACTIVITY_SENTRY);

      if (can_unit_do_activity(punit, activity)) {
        punit->done_moving = true;
        set_unit_activity(punit, activity);
        send_unit_info(nullptr, punit);

        break;
      }
    }

      cancel_orders(punit, "  orders canceled because of failed activity");
      notify_player(pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                    _("Orders for %s aborted since they "
                      "give an invalid activity."),
                    unit_link(punit));
      return true;
    case ORDER_MOVE:
    case ORDER_ACTION_MOVE:
      // Move unit
      if (!(dst_tile = mapstep(&(wld.map), unit_tile(punit), order.dir))) {
        cancel_orders(punit, "  move order sent us to invalid location");
        notify_player(pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                      _("Orders for %s aborted since they "
                        "give an invalid location."),
                      unit_link(punit));
        return true;
      }

      if (order.order != ORDER_ACTION_MOVE
          && maybe_cancel_goto_due_to_enemy(punit, dst_tile)) {
        // Plain move required: no attack, trade route etc.
        cancel_orders(punit, "  orders canceled because of enemy");
        notify_player(pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                      _("Orders for %s aborted as there "
                        "are units in the way."),
                      unit_link(punit));
        return true;
      }

      log_debug("  moving to %d,%d", TILE_XY(dst_tile));
      res = unit_move_handling(punit, dst_tile, false,
                               order.order != ORDER_ACTION_MOVE);
      if (!player_unit_by_number(pplayer, unitid)) {
        log_debug("  unit died while moving.");
        // A player notification should already have been sent.
        return false;
      }

      if (res && !same_pos(dst_tile, unit_tile(punit))) {
        // Movement succeeded but unit didn't move.
        log_debug("  orders resulted in combat.");
        send_unit_info(nullptr, punit);
        return true;
      }

      if (!res) {
        fc_assert(0 <= punit->moves_left);

        // Movement failed (ZOC, etc.)
        cancel_orders(punit, "  attempt to move failed.");

        if (!player_is_watching(punit, fresh)
            /* The final move "failed" because the unit needs to ask the
             * player what action it should take.
             *
             * The action decision request notifies the player. Its
             * location at the unit's last order makes it clear to the
             * player who the decision is for. ("The Spy I sent to Berlin
             * has arrived.")
             *
             * A notification message is therefore redundant. */
            && !(last_order && punit->action_decision_want == ACT_DEC_ACTIVE
                 && punit->action_decision_tile == dst_tile)) {
          /* The player may have missed this. No one else will announce it
           * in a satisfying manner. Inform the player. */
          notify_player(pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                        _("Orders for %s aborted because of failed move."),
                        unit_link(punit));
        }

        return true;
      }
      break;
    case ORDER_PERFORM_ACTION:
      oaction = action_by_number(order.action);

      // Checked in unit_order_list_is_sane()
      fc_assert_action(oaction != nullptr, continue);

      log_debug("  orders: doing action %s", action_rule_name(oaction));

      dst_tile = index_to_tile(&(wld.map), order.target);

      if (dst_tile == nullptr) {
        /* Could be at the edge of the map while trying to target a tile
         * outside of it. */

        cancel_orders(punit, "  target location doesn't exist");
        illegal_action_msg(unit_owner(punit), E_UNIT_ORDERS, punit,
                           order.action, dst_tile, nullptr, nullptr);

        return true;
      }

      // Get the target city from the target tile.
      tgt_city = tile_city(dst_tile);

      if (tgt_city == nullptr
          && action_id_get_target_kind(order.action) == ATK_CITY) {
        // This action targets a city but no city target was found.

        cancel_orders(punit, "  perform action vs city with no city");
        illegal_action_msg(unit_owner(punit), E_UNIT_ORDERS, punit,
                           order.action, dst_tile, tgt_city, nullptr);

        return true;
      }

      // Get a target unit at the target tile.
      tgt_unit = action_tgt_unit(punit, dst_tile, true);

      if (tgt_unit == nullptr
          && action_id_get_target_kind(order.action) == ATK_UNIT) {
        // This action targets a unit but no target unit was found.

        cancel_orders(punit, "  perform action vs unit with no unit");
        illegal_action_msg(unit_owner(punit), E_UNIT_ORDERS, punit,
                           order.action, dst_tile, tgt_city, tgt_unit);

        return true;
      }

      // Server side sub target assignment
      if (oaction->target_complexity == ACT_TGT_COMPL_FLEXIBLE
          && order.sub_target == NO_TARGET) {
        // Try to find a sub target.
        sub_tgt_id = action_sub_target_id_for_action(oaction, punit);
      } else {
        // The client should have specified a sub target if needed
        sub_tgt_id = order.sub_target;
      }

      // Get a target extra at the target tile
      pextra =
          (sub_tgt_id == NO_TARGET ? nullptr : extra_by_number(sub_tgt_id));

      if (action_get_sub_target_kind(oaction) == ASTK_EXTRA_NOT_THERE
          && pextra != nullptr && action_creates_extra(oaction, pextra)
          && tile_has_extra(dst_tile, pextra)) {
        // Already there. Move on to the next order.
        break;
      }

      if (action_get_sub_target_kind(oaction) == ASTK_EXTRA
          && pextra != nullptr && action_removes_extra(oaction, pextra)
          && !tile_has_extra(dst_tile, pextra)) {
        // Already not there. Move on to the next order.
        break;
      }

      // No target selected.
      tgt_id = -1;

      // Assume impossible until told otherwise.
      prob = ACTPROB_IMPOSSIBLE;

      switch (action_id_get_target_kind(order.action)) {
      case ATK_UNITS:
        prob = action_prob_vs_units(punit, order.action, dst_tile);
        tgt_id = dst_tile->index;
        break;
      case ATK_TILE:
        prob = action_prob_vs_tile(punit, order.action, dst_tile, pextra);
        tgt_id = dst_tile->index;
        break;
      case ATK_CITY:
        prob = action_prob_vs_city(punit, order.action, tgt_city);
        tgt_id = tgt_city->id;
        break;
      case ATK_UNIT:
        prob = action_prob_vs_unit(punit, order.action, tgt_unit);

        tgt_id = tgt_unit->id;
        break;
      case ATK_SELF:
        prob = action_prob_self(punit, order.action);

        tgt_id = unitid;
        break;
      case ATK_COUNT:
        qCritical("Invalid action target kind");

        /* The check below will abort and cancel the orders because prob
         * was initialized to impossible above this switch statement. */

        break;
      }

      if (!action_prob_possible(prob)) {
        /* The player has enough information to know that this action is
         * against the rules. Don't risk any punishment by trying to
         * perform it. */

        cancel_orders(punit, "  illegal action");
        notify_player(
            pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
            _("%s could not do %s to %s."), unit_link(punit),
            qUtf8Printable(action_id_name_translation(order.action)),
            tile_link(dst_tile));

        // Try to explain what rule made it illegal.
        illegal_action_msg(unit_owner(punit), E_BAD_COMMAND, punit,
                           order.action, dst_tile, tgt_city, tgt_unit);

        return true;
      }

      if (action_id_has_result_safe(order.action, ACTRES_FOUND_CITY)) {
        // This action needs a name.
        name = city_name_suggestion(pplayer, unit_tile(punit));
      } else {
        // This action doesn't need a name.
        name = "";
      }

      performed = unit_perform_action(pplayer, unitid, tgt_id, sub_tgt_id,
                                      name, order.action, ACT_REQ_PLAYER);

      if (!player_unit_by_number(pplayer, unitid)) {
        // The unit "died" while performing the action.
        return false;
      }

      if (!performed) {
        // The action wasn't performed as ordered.

        cancel_orders(punit, "  failed action");
        notify_player(
            pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
            _("Orders for %s aborted because "
              "doing %s to %s failed."),
            unit_link(punit),
            qUtf8Printable(action_id_name_translation(order.action)),
            tile_link(dst_tile));

        return true;
      }

      if (action_id_get_act_time(order.action, punit, dst_tile, pextra)
          != ACT_TIME_INSTANTANEOUS) {
        // Done at turn change.
        punit->done_moving = true;
        send_unit_info(nullptr, punit);
        break;
      }

      break;
    case ORDER_LAST:
      /* Should be caught when loading the unit orders from the savegame or
       * when receiving the unit orders from the client. */
      fc_assert_msg(order.order != ORDER_LAST, "Invalid order: last.");
      cancel_orders(punit, "  invalid order!");
      notify_player(pplayer, unit_tile(punit), E_UNIT_ORDERS, ftc_server,
                    _("Your %s has invalid orders."), unit_link(punit));
      return true;
    }

    if (last_order) {
      fc_assert(punit->has_orders == false);
      log_debug("  stopping because orders are complete");
      return true;
    }

    if (punit->orders.index == punit->orders.length) {
      fc_assert(punit->orders.repeat);
      // Start over.
      log_debug("  repeating orders.");
      punit->orders.index = 0;
    }
  } // end while
}

/**
   Return the vision the unit will have at the given tile.  The base vision
   range may be modified by effects.

   Note that vision MUST be independent of transported_by for this to work
   properly.
 */
int get_unit_vision_at(struct unit *punit, const struct tile *ptile,
                       enum vision_layer vlayer)
{
  const int base = unit_type_get(punit)->vision_radius_sq;
  const int bonus =
      get_unittype_bonus(unit_owner(punit), ptile, unit_type_get(punit),
                         EFT_UNIT_VISION_RADIUS_SQ, vlayer);
  switch (vlayer) {
  case V_MAIN:
    return MAX(0, base) + MAX(0, bonus);
  case V_INVIS:
  case V_SUBSURFACE:
    return CLIP(0, base, 2) + MAX(0, bonus);
  case V_COUNT:
    break;
  }

  qCritical("Unsupported vision layer variant: %d.", vlayer);
  return 0;
}

/**
   Refresh the unit's vision.

   This function has very small overhead and can be called any time effects
   may have changed the vision range of the city.
 */
void unit_refresh_vision(struct unit *punit)
{
  struct vision *uvision = punit->server.vision;
  const struct tile *utile = unit_tile(punit);
  const v_radius_t radius_sq =
      V_RADIUS(get_unit_vision_at(punit, utile, V_MAIN),
               get_unit_vision_at(punit, utile, V_INVIS),
               get_unit_vision_at(punit, utile, V_SUBSURFACE));

  vision_change_sight(uvision, radius_sq);
  ASSERT_VISION(uvision);
}

/**
   Refresh the vision of all units in the list - see unit_refresh_vision.
 */
void unit_list_refresh_vision(struct unit_list *punitlist)
{
  unit_list_iterate(punitlist, punit) { unit_refresh_vision(punit); }
  unit_list_iterate_end;
}

/**
   Used to implement the game rule controlled by the unitwaittime setting.
   Notifies the unit owner if the unit is unable to act.
 */
bool unit_can_do_action_now(const struct unit *punit)
{
  time_t dt;

  if (!punit) {
    return false;
  }

  if (game.server.unitwaittime <= 0) {
    return true;
  }

  if (punit->action_turn != game.info.turn - 1) {
    return true;
  }

  dt = time(nullptr) - punit->action_timestamp;
  if (dt < game.server.unitwaittime) {
    char buf[64];
    format_time_duration(game.server.unitwaittime - dt, buf, sizeof(buf));
    notify_player(unit_owner(punit), unit_tile(punit), E_BAD_COMMAND,
                  ftc_server,
                  _("Your unit may not act for another %s "
                    "this turn. See /help unitwaittime."),
                  buf);
    return false;
  }

  return true;
}

/**
   Mark a unit as having done something at the current time. This is used
   in conjunction with unit_can_do_action_now() and the unitwaittime setting.
 */
void unit_did_action(struct unit *punit)
{
  // Dont spam network with unitwaitime changes if its disabled
  if (!punit || !game.server.unitwaittime) {
    return;
  }

  punit->action_timestamp = time(nullptr);
  punit->action_turn = game.info.turn;
}

/**
   Units (usually barbarian units) may disband spontaneously if they are
   far from any enemy units or cities. It is to remove barbarians that do
   not engage into any activity for a long time.
 */
bool unit_can_be_retired(struct unit *punit)
{
  // check if there is enemy nearby
  square_iterate(&(wld.map), unit_tile(punit), 3, ptile)
  {
    if (is_enemy_city_tile(ptile, unit_owner(punit))
        || is_enemy_unit_tile(ptile, unit_owner(punit))) {
      return false;
    }
  }
  square_iterate_end;

  return true;
}

/**
   Returns TRUE iff the unit order array is sane.
 */
bool unit_order_list_is_sane(int length, const struct unit_order *orders)
{
  int i;

  for (i = 0; i < length; i++) {
    struct action *paction;
    struct extra_type *pextra;

    if (orders[i].order > ORDER_LAST) {
      qCritical("invalid order %d at index %d", orders[i].order, i);
      return false;
    }
    switch (orders[i].order) {
    case ORDER_MOVE:
    case ORDER_ACTION_MOVE:
      if (!map_untrusted_dir_is_valid(orders[i].dir)) {
        qCritical("in order %d, invalid move direction %d.", i,
                  orders[i].dir);
        return false;
      }
      break;
    case ORDER_ACTIVITY:
      switch (orders[i].activity) {
      case ACTIVITY_SENTRY:
        if (i != length - 1) {
          // Only allowed as the last order.
          qCritical("activity %d is not allowed at index %d.",
                    orders[i].activity, i);
          return false;
        }
        break;
      // Replaced by action orders
      case ACTIVITY_BASE:
      case ACTIVITY_GEN_ROAD:
      case ACTIVITY_FALLOUT:
      case ACTIVITY_POLLUTION:
      case ACTIVITY_PILLAGE:
      case ACTIVITY_MINE:
      case ACTIVITY_IRRIGATE:
      case ACTIVITY_PLANT:
      case ACTIVITY_CULTIVATE:
      case ACTIVITY_TRANSFORM:
      case ACTIVITY_CONVERT:
      case ACTIVITY_FORTIFYING:
        qCritical("at index %d, use action rather than activity %d.", i,
                  orders[i].activity);
        return false;
      // Not supported.
      case ACTIVITY_EXPLORE:
      case ACTIVITY_IDLE:
      // Not set from the client.
      case ACTIVITY_GOTO:
      case ACTIVITY_FORTIFIED:
      // Compatiblity, used in savegames.
      case ACTIVITY_OLD_ROAD:
      case ACTIVITY_OLD_RAILROAD:
      case ACTIVITY_FORTRESS:
      case ACTIVITY_AIRBASE:
      // Unused.
      case ACTIVITY_PATROL_UNUSED:
      case ACTIVITY_LAST:
      case ACTIVITY_UNKNOWN:
        qCritical("at index %d, unsupported activity %d.", i,
                  orders[i].activity);
        return false;
      }

      break;
    case ORDER_PERFORM_ACTION:
      if (!action_id_exists(orders[i].action)) {
        // Non-existent action.
        qCritical("at index %d, the action %d doesn't exist.", i,
                  orders[i].action);
        return false;
      }

      paction = action_by_number(orders[i].action);

      // Validate main target.
      if (index_to_tile(&(wld.map), orders[i].target) == nullptr) {
        qCritical("at index %d, invalid tile target %d for the action %d.",
                  i, orders[i].target, orders[i].action);
        return false;
      }

      if (orders[i].dir != DIR8_ORIGIN) {
        qCritical("at index %d, the action %d sets the outdated target"
                  " specification dir.",
                  i, orders[i].action);
      }

      // Validate sub target.
      switch (action_id_get_sub_target_kind(orders[i].action)) {
      case ASTK_BUILDING:
        // Sub target is a building.
        if (!improvement_by_number(orders[i].sub_target)) {
          // Sub target is invalid.
          qCritical("at index %d, cannot do %s without a target.", i,
                    action_id_rule_name(orders[i].action));
          return false;
        }
        break;
      case ASTK_TECH:
        // Sub target is a technology.
        if (orders[i].sub_target == A_NONE
            || (!valid_advance_by_number(orders[i].sub_target)
                && orders[i].sub_target != A_FUTURE)) {
          // Target tech is invalid.
          qCritical("at index %d, cannot do %s without a target.", i,
                    action_id_rule_name(orders[i].action));
          return false;
        }
        break;
      case ASTK_EXTRA:
      case ASTK_EXTRA_NOT_THERE:
        // Sub target is an extra.
        pextra = (!(orders[i].sub_target == NO_TARGET
                    || (orders[i].sub_target < 0
                        || (orders[i].sub_target
                            >= game.control.num_extra_types)))
                      ? extra_by_number(orders[i].sub_target)
                      : nullptr);
        fc_assert(pextra == nullptr || !(pextra->ruledit_disabled));
        if (pextra == nullptr) {
          if (paction->target_complexity != ACT_TGT_COMPL_FLEXIBLE) {
            // Target extra is invalid.
            qCritical("at index %d, cannot do %s without a target.", i,
                      action_id_rule_name(orders[i].action));
            return false;
          }
        } else {
          if (!(action_removes_extra(paction, pextra)
                || action_creates_extra(paction, pextra))) {
            // Target extra is irrelevant for the action.
            qCritical("at index %d, cannot do %s to %s.", i,
                      action_id_rule_name(orders[i].action),
                      extra_rule_name(pextra));
            return false;
          }
        }
        break;
      case ASTK_NONE:
        // No validation required.
        break;
      // Invalid action?
      case ASTK_COUNT:
        fc_assert_ret_val_msg(
            action_id_get_sub_target_kind(orders[i].action) != ASTK_COUNT,
            false, "Bad action %d in order number %d.", orders[i].action, i);
      }

      // Some action orders are sane only in the last order.
      if (i != length - 1) {
        // If the unit is dead,
        if (utype_is_consumed_by_action(paction, nullptr)
            /* or if Freeciv21 has no idea where the unit will end up after
             * it has performed this action, */
            || !(utype_is_unmoved_by_action(paction, nullptr)
                 || utype_is_moved_to_tgt_by_action(paction, nullptr))
            // or if the unit will end up standing still,
            || action_has_result(paction, ACTRES_FORTIFY)) {
          /* than having this action in the middle of a unit's orders is
           * probably wrong. */
          qCritical("action %d is not allowed at index %d.",
                    orders[i].action, i);
          return false;
        }
      }

      /* Don't validate that the target tile really contains a target or
       * that the actor player's map think the target tile has one.
       * The player may target a something from his player map that isn't
       * there any more, a target he thinks is there even if his player map
       * doesn't have it or even a target he assumes will be there when the
       * unit reaches the target tile.
       *
       * With that said: The client should probably at least have an
       * option to only aim city targeted actions at cities. */

      break;
    case ORDER_FULL_MP:
      break;
    case ORDER_LAST:
      // An invalid order.  This is handled above.
      break;
    }
  }

  return true;
}

/**
   Sanity-check unit order arrays from a packet and create a unit_order array
   from their contents if valid.
 */
struct unit_order *create_unit_orders(int length,
                                      const struct unit_order *orders)
{
  if (!unit_order_list_is_sane(length, orders)) {
    return nullptr;
  }

  auto *unit_orders = new unit_order[length];
  memcpy(unit_orders, orders, length * sizeof(*(unit_orders)));

  return unit_orders;
}
