/**************************************************************************
    Copyright (c) 1996-2020 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

/* common */
#include "actions.h"
#include "fc_types.h"



adv_want dai_action_value_unit_vs_city(struct action *paction,
                                       struct unit *actor_unit,
                                       struct city *target_city,
                                       int sub_tgt_id, int count_tech);

int dai_action_choose_sub_tgt_unit_vs_city(struct action *paction,
                                           struct unit *actor_unit,
                                           struct city *target_city);




