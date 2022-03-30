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

// common
#include "fc_types.h"
#include "map.h"

void auto_settlers_ruleset_init();

struct settlermap;
class PFPath;

void adv_settlers_free();

void auto_settlers_player(struct player *pplayer);

void auto_settler_findwork(struct player *pplayer, struct unit *punit,
                           struct settlermap *state, int recursion);

bool auto_settler_setup_work(struct player *pplayer, struct unit *punit,
                             struct settlermap *state, int recursion,
                             PFPath *path, struct tile *best_tile,
                             enum unit_activity best_act,
                             struct extra_type **best_target,
                             int completion_time);

adv_want settler_evaluate_improvements(unit *punit, unit_activity *best_act,
                                       extra_type **best_target,
                                       tile **best_tile, PFPath *path,
                                       settlermap *state);

struct city *settler_evaluate_city_requests(struct unit *punit,
                                            struct worker_task **best_task,
                                            PFPath *path,
                                            struct settlermap *state);

void adv_unit_new_task(struct unit *punit, enum adv_unit_task task,
                       struct tile *ptile);

bool adv_settler_safe_tile(const struct player *pplayer, struct unit *punit,
                           struct tile *ptile);

adv_want adv_settlers_road_bonus(struct tile *ptile,
                                 struct road_type *proad);

bool auto_settlers_speculate_can_act_at(const struct unit *punit,
                                        enum unit_activity activity,
                                        bool omniscient_cheat,
                                        struct extra_type *target,
                                        const struct tile *ptile);

extern action_id as_actions_transform[MAX_NUM_ACTIONS];

#define as_transform_action_iterate(_act_)                                  \
  {                                                                         \
    action_list_iterate(as_actions_transform, _act_)

#define as_transform_action_iterate_end                                     \
  action_list_iterate_end                                                   \
  }

extern action_id as_actions_extra[MAX_NUM_ACTIONS];

#define as_extra_action_iterate(_act_)                                      \
  {                                                                         \
    action_list_iterate(as_actions_extra, _act_)

#define as_extra_action_iterate_end                                         \
  action_list_iterate_end                                                   \
  }

extern action_id as_actions_rmextra[MAX_NUM_ACTIONS];

#define as_rmextra_action_iterate(_act_)                                    \
  {                                                                         \
    action_list_iterate(as_actions_rmextra, _act_)

#define as_rmextra_action_iterate_end                                       \
  action_list_iterate_end                                                   \
  }
