// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "shared.h"

// common
#include "fc_types.h"
#include "player.h"
#include "requirements.h"

enum fc_tristate mke_eval_req(
    const struct player *pow_player, const struct player *target_player,
    const struct player *other_player, const struct city *target_city,
    const struct impr_type *target_building, const struct tile *target_tile,
    const struct unit *target_unit, const struct output_type *target_output,
    const struct specialist *target_specialist,
    const struct requirement *req, const enum req_problem_type prob_type);

enum fc_tristate mke_eval_reqs(
    const struct player *pow_player, const struct player *target_player,
    const struct player *other_player, const struct city *target_city,
    const struct impr_type *target_building, const struct tile *target_tile,
    const struct unit *target_unit, const struct output_type *target_output,
    const struct specialist *target_specialist,
    const struct requirement_vector *reqs,
    const enum req_problem_type prob_type);

bool can_see_techs_of_target(const struct player *pow_player,
                             const struct player *target_player);
