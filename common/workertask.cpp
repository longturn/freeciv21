/***********************************************************************
Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
 /\/\             part of Freeciv21. Freeciv21 is free software: you can
   \_\  _..._    redistribute it and/or modify it under the terms of the
   (" )(_..._)      GNU General Public License  as published by the Free
    ^^  // \\      Software Foundation, either version 3 of the License,
                  or (at your option) any later version. You should have
received a copy of the GNU General Public License along with Freeciv21.
                              If not, see https://www.gnu.org/licenses/.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "log.h"

// common
#include "city.h"

#include "workertask.h"

/**
   Initialize empty worker_task.
 */
void worker_task_init(struct worker_task *ptask)
{
  ptask->ptile = nullptr;
  ptask->want = 0;
}
