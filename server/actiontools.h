/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once
/* common */
#include "actions.h"
#include "player.h"
#include "tile.h"
#include "unit.h"


void action_consequence_caught(const struct action *paction,
                               struct player *offender,
                               struct player *victim_player,
                               const struct tile *victim_tile,
                               const char *victim_link);

void action_consequence_success(const struct action *paction,
                                struct player *offender,
                                struct player *victim_player,
                                const struct tile *victim_tile,
                                const char *victim_link);

void action_consequence_complete(const struct action *paction,
                                 struct player *offender,
                                 struct player *victim_player,
                                 const struct tile *victim_tile,
                                 const char *victim_link);

void action_success_target_pay_mp(struct action *paction, int target_id,
                                  struct unit *target);

void action_success_actor_price(struct action *paction, int actor_id,
                                struct unit *actor);

struct city *action_tgt_city(struct unit *actor, struct tile *target_tile,
                             bool accept_all_actions);

struct unit *action_tgt_unit(struct unit *actor, struct tile *target_tile,
                             bool accept_all_actions);

struct tile *action_tgt_tile(struct unit *actor, struct tile *target_tile,
                             const struct extra_type *target_extra,
                             bool accept_all_actions);

struct extra_type *action_tgt_tile_extra(const struct unit *actor,
                                         const struct tile *target_tile,
                                         bool accept_all_actions);

int action_sub_target_id_for_action(const struct action *paction,
                                    struct unit *actor_unit);

const struct action_auto_perf *action_auto_perf_unit_sel(
    const enum action_auto_perf_cause cause, const struct unit *actor,
    const struct player *other_player, const struct output_type *output);

const struct action *action_auto_perf_unit_do(
    const enum action_auto_perf_cause cause, struct unit *actor,
    const struct player *other_player, const struct output_type *output,
    const struct tile *target_tile, const struct city *target_city,
    const struct unit *target_unit, const struct extra_type *target_extra);

struct act_prob action_auto_perf_unit_prob(
    const enum action_auto_perf_cause cause, struct unit *actor,
    const struct player *other_player, const struct output_type *output,
    const struct tile *target_tile, const struct city *target_city,
    const struct unit *target_unit, const struct extra_type *target_extra);

bool action_failed_dice_roll(const struct player *act_player,
                             const struct unit *act_unit,
                             const struct city *tgt_city,
                             const struct player *tgt_player,
                             const struct action *paction);


