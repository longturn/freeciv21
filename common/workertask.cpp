// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "workertask.h"

/**
   Initialize empty worker_task.
 */
void worker_task_init(struct worker_task *ptask)
{
  ptask->ptile = nullptr;
  ptask->want = 0;
}
