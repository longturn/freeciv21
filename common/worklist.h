// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

void worklist_init(struct worklist *pwl);

int worklist_length(const struct worklist *pwl);
bool worklist_is_empty(const struct worklist *pwl);
bool worklist_peek(const struct worklist *pwl, struct universal *prod);
bool worklist_peek_ith(const struct worklist *pwl, struct universal *prod,
                       int idx);
void worklist_advance(struct worklist *pwl);

void worklist_copy(struct worklist *dst, const struct worklist *src);
void worklist_remove(struct worklist *pwl, int idx);
bool worklist_append(struct worklist *pwl, const struct universal *prod);
bool worklist_insert(struct worklist *pwl, const struct universal *prod,
                     int idx);
// Used in packets_gen.cpp
bool are_worklists_equal(const struct worklist *wlist1,
                         const struct worklist *wlist2);

// Iterate over all entries in the worklist.
#define worklist_iterate(_list, _p)                                         \
  {                                                                         \
    struct universal _p;                                                    \
    int _p##_index = 0;                                                     \
                                                                            \
    while (_p##_index < worklist_length(_list)) {                           \
      worklist_peek_ith(_list, &_p, _p##_index++);

#define worklist_iterate_end                                                \
  }                                                                         \
  }
