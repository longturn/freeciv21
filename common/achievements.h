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
#ifndef FC__ACHIEVEMENTS_H
#define FC__ACHIEVEMENTS_H


/* common */
#include "fc_types.h"
#include "name_translation.h"
#include "player.h"

struct achievement {
  int id;
  struct name_translation name;
  bool ruledit_disabled;
  enum achievement_type type;
  int value;
  bool unique;
  int culture;
  struct player *first;
  bv_player achievers;

  /* Messages at server side only */
  char *first_msg;
  char *cons_msg;
};

void achievements_init(void);
void achievements_free(void);

int achievement_index(const struct achievement *pach);
int achievement_number(const struct achievement *pach);
struct achievement *achievement_by_number(int id);

const char *achievement_name_translation(struct achievement *pach);
const char *achievement_rule_name(struct achievement *pach);
struct achievement *achievement_by_rule_name(const char *name);

struct player *achievement_plr(struct achievement *ach,
                               struct player_list *achievers);
bool achievement_check(struct achievement *ach, struct player *pplayer);

const char *achievement_first_msg(struct achievement *pach);
const char *achievement_later_msg(struct achievement *pach);

bool achievement_player_has(const struct achievement *pach,
                            const struct player *pplayer);
bool achievement_claimed(const struct achievement *pach);

#define achievements_iterate(_ach_)                                         \
  {                                                                         \
    int _i_;                                                                \
    for (_i_ = 0; _i_ < game.control.num_achievement_types; _i_++) {        \
      struct achievement *_ach_ = achievement_by_number(_i_);

#define achievements_iterate_end                                            \
  }                                                                         \
  }

#define achievements_re_active_iterate(_p)                                  \
  achievements_iterate(_p)                                                  \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define achievements_re_active_iterate_end                                  \
  }                                                                         \
  }                                                                         \
  achievements_iterate_end;

int get_literacy(const struct player *pplayer);


#endif /* FC__ACHIEVEMENTS_H */
