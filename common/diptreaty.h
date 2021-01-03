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

/* utility */
#include "requirements.h"

/* Used in the network protocol */
#define SPECENUM_NAME clause_type
#define SPECENUM_VALUE0 CLAUSE_ADVANCE
#define SPECENUM_VALUE0NAME "Advance"
#define SPECENUM_VALUE1 CLAUSE_GOLD
#define SPECENUM_VALUE1NAME "Gold"
#define SPECENUM_VALUE2 CLAUSE_MAP
#define SPECENUM_VALUE2NAME "Map"
#define SPECENUM_VALUE3 CLAUSE_SEAMAP
#define SPECENUM_VALUE3NAME "Seamap"
#define SPECENUM_VALUE4 CLAUSE_CITY
#define SPECENUM_VALUE4NAME "City"
#define SPECENUM_VALUE5 CLAUSE_CEASEFIRE
#define SPECENUM_VALUE5NAME "Ceasefire"
#define SPECENUM_VALUE6 CLAUSE_PEACE
#define SPECENUM_VALUE6NAME "Peace"
#define SPECENUM_VALUE7 CLAUSE_ALLIANCE
#define SPECENUM_VALUE7NAME "Alliance"
#define SPECENUM_VALUE8 CLAUSE_VISION
#define SPECENUM_VALUE8NAME "Vision"
#define SPECENUM_VALUE9 CLAUSE_EMBASSY
#define SPECENUM_VALUE9NAME "Embassy"
#define SPECENUM_COUNT CLAUSE_COUNT
#include "specenum_gen.h"

#define is_pact_clause(x)                                                   \
  ((x == CLAUSE_CEASEFIRE) || (x == CLAUSE_PEACE) || (x == CLAUSE_ALLIANCE))

struct clause_info {
  enum clause_type type;
  bool enabled;
  struct requirement_vector giver_reqs;
  struct requirement_vector receiver_reqs;
};

/* For when we need to iterate over treaties */
struct Clause;
#define SPECLIST_TAG clause
#define SPECLIST_TYPE struct Clause
#include "speclist.h"

#define clause_list_iterate(clauselist, pclause)                            \
  TYPED_LIST_ITERATE(struct Clause, clauselist, pclause)
#define clause_list_iterate_end LIST_ITERATE_END

struct Clause {
  enum clause_type type;
  struct player *from;
  int value;
};

struct Treaty {
  struct player *plr0, *plr1;
  bool accept0, accept1;
  struct clause_list *clauses;
};

bool diplomacy_possible(const struct player *pplayer,
                        const struct player *aplayer);
bool could_meet_with_player(const struct player *pplayer,
                            const struct player *aplayer);
bool could_intel_with_player(const struct player *pplayer,
                             const struct player *aplayer);

void init_treaty(struct Treaty *ptreaty, struct player *plr0,
                 struct player *plr1);
bool add_clause(struct Treaty *ptreaty, struct player *pfrom,
                enum clause_type type, int val);
bool remove_clause(struct Treaty *ptreaty, struct player *pfrom,
                   enum clause_type type, int val);
void clear_treaty(struct Treaty *ptreaty);

void clause_infos_init();
void clause_infos_free();
struct clause_info *clause_info_get(enum clause_type type);

bool clause_enabled(enum clause_type type, struct player *from,
                    struct player *to);
