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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "shared.h"

#include "iterator.h"

/*******************************************************************/ /**
   'next' function implementation for an "invalid" iterator.
 ***********************************************************************/
static void invalid_iter_next(struct iterator *it)
{ /* Do nothing. */
}

/*******************************************************************/ /**
   'get' function implementation for an "invalid" iterator.
 ***********************************************************************/
static void *invalid_iter_get(const struct iterator *it) { return NULL; }

/*******************************************************************/ /**
   'valid' function implementation for an "invalid" iterator.
 ***********************************************************************/
static bool invalid_iter_valid(const struct iterator *it) { return FALSE; }

/*******************************************************************/ /**
   Initializes the iterator vtable so that generic_iterate assumes that
   the iterator is invalid.
 ***********************************************************************/
struct iterator *invalid_iter_init(struct iterator *it)
{
  it->next = invalid_iter_next;
  it->get = invalid_iter_get;
  it->valid = invalid_iter_valid;
  return it;
}
