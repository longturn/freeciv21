/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

/* utility */
#include "registry.h"
#include "shared.h"

/* common */
#include "worklist.h"

struct global_worklist;      /* Opaque type. */
struct global_worklist_list; /* Opaque type. */

void global_worklists_init(void);
void global_worklists_free(void);
void global_worklists_build(void);
void global_worklists_unbuild(void);

void global_worklists_load(struct section_file *file);
void global_worklists_save(struct section_file *file);
int global_worklists_number(void);

struct global_worklist *global_worklist_new(const char *name);
void global_worklist_destroy(struct global_worklist *pgwl);

struct global_worklist *global_worklist_by_id(int id);

bool global_worklist_is_valid(const struct global_worklist *pgwl);
bool global_worklist_set(struct global_worklist *pgwl,
                         const struct worklist *pwl);
const struct worklist *
global_worklist_get(const struct global_worklist *pgwl);
int global_worklist_id(const struct global_worklist *pgwl);
void global_worklist_set_name(struct global_worklist *pgwl,
                              const char *name);
const char *global_worklist_name(const struct global_worklist *pgwl);

#define SPECLIST_TAG global_worklist
#define SPECLIST_TYPE struct global_worklist
#include "speclist.h"

/* Iterates all global worklists, include the ones which are not valid. */
#define global_worklists_iterate_all(pgwl)                                  \
  if (client.worklists) {                                                   \
  TYPED_LIST_ITERATE(struct global_worklist, client.worklists, pgwl)
#define global_worklists_iterate_all_end                                    \
  LIST_ITERATE_END                                                          \
  }

/* Iterates all valid global worklists. */
#define global_worklists_iterate(pgwl)                                      \
  global_worklists_iterate_all(pgwl)                                        \
  {                                                                         \
    if (global_worklist_is_valid(pgwl)) {
#define global_worklists_iterate_end                                        \
  }                                                                         \
  }                                                                         \
  global_worklists_iterate_all_end;


