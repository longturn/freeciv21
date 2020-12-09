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
#pragma once

/* common */
#include "fc_types.h"
#include "unittype.h"

void dai_manage_tech(struct ai_type *ait, struct player *pplayer);
void dai_clear_tech_wants(struct ai_type *ait, struct player *pplayer);
void dai_next_tech_goal(struct player *pplayer);
struct unit_type *dai_wants_role_unit(struct ai_type *ait,
                                      struct player *pplayer,
                                      struct city *pcity, int role,
                                      int want);
struct unit_type *dai_wants_defender_against(struct ai_type *ait,
                                             struct player *pplayer,
                                             struct city *pcity,
                                             const struct unit_type *att,
                                             int want);


