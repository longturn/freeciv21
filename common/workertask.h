// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

struct worker_task {
  struct tile *ptile;
  enum unit_activity act;
  struct extra_type *tgt;
  int want;
};

// get 'struct worker_task_list' and related functions:
#define SPECLIST_TAG worker_task
#define SPECLIST_TYPE struct worker_task
#include "speclist.h"

#define worker_task_list_iterate(tasklist, ptask)                           \
  TYPED_LIST_ITERATE(struct worker_task, tasklist, ptask)
#define worker_task_list_iterate_end LIST_ITERATE_END

void worker_task_init(struct worker_task *ptask);
