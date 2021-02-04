/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#pragma once

// common
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
