// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"
#include "requirements.h" // struct requirement_vector

#define is_pact_clause(x)                                                   \
  ((x == CLAUSE_CEASEFIRE) || (x == CLAUSE_PEACE) || (x == CLAUSE_ALLIANCE))

#define clause_list_iterate(clauselist, pclause)                            \
  TYPED_LIST_ITERATE(struct Clause, clauselist, pclause)
#define clause_list_iterate_end LIST_ITERATE_END

struct clause_info {
  enum clause_type type;
  bool enabled;
  struct requirement_vector giver_reqs;
  struct requirement_vector receiver_reqs;
};

// For when we need to iterate over treaties
struct Clause;
#define SPECLIST_TAG clause
#define SPECLIST_TYPE struct Clause
#include "speclist.h"

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
