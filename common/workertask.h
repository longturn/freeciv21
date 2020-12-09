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
#ifndef FC__WORKERTASK_H
#define FC__WORKERTASK_H


struct worker_task {
  struct tile *ptile;
  enum unit_activity act;
  struct extra_type *tgt;
  int want;
};

/* get 'struct worker_task_list' and related functions: */
#define SPECLIST_TAG worker_task
#define SPECLIST_TYPE struct worker_task
#include "speclist.h"

#define worker_task_list_iterate(tasklist, ptask)                           \
  TYPED_LIST_ITERATE(struct worker_task, tasklist, ptask)
#define worker_task_list_iterate_end LIST_ITERATE_END

void worker_task_init(struct worker_task *ptask);


#endif /* FC__WORKERTASK_H */
