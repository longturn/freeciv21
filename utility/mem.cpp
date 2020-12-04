/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/**********************************************************************
 *
 * This module contains freeciv-specific memory management functions
 *
 **********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h" /* TRUE, FALSE */

#include "mem.h"

/******************************************************************/ /**
   Function used by fc_strdup macro, strdup() replacement
   No need to check return value.
 **********************************************************************/
char *real_fc_strdup(const char *str, const char *called_as, int line,
                     const char *file)
{
  char *dest = new char[strlen(str) + 1];

  // no need to check whether dest is non-NULL (raises std::bad_alloc)
  strcpy(dest, str);
  return dest;
}
