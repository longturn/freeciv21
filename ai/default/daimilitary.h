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

/* server/advisors */
#include "advchoice.h"

/* When an enemy has this or lower number of cities left, try harder
   to finish him off. */
#define FINISH_HIM_CITY_COUNT 5

typedef struct unit_list *(player_unit_list_getter)(struct player *pplayer);

struct unit_type *dai_choose_defender_versus(struct city *pcity,
                                             struct unit *attacker);
void military_advisor_choose_tech(struct player *pplayer,
                                  struct adv_choice *choice);
struct adv_choice *military_advisor_choose_build(
    struct ai_type *ait, struct player *pplayer, struct city *pcity,
    const struct civ_map *mamap, player_unit_list_getter ul_cb);
void dai_assess_danger_player(struct ai_type *ait, struct player *pplayer,
                              const struct civ_map *dmap);
int assess_defense_quadratic(struct ai_type *ait, struct city *pcity);
int assess_defense_unit(struct ai_type *ait, struct city *pcity,
                        struct unit *punit, bool igwall);
int assess_defense(struct ai_type *ait, struct city *pcity);
int dai_unit_defence_desirability(struct ai_type *ait,
                                  const struct unit_type *punittype);
int dai_unit_attack_desirability(struct ai_type *ait,
                                 const struct unit_type *punittype);
bool dai_process_defender_want(struct ai_type *ait, struct player *pplayer,
                               struct city *pcity, unsigned int danger,
                               struct adv_choice *choice);


