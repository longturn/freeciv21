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

#ifndef FC__MEM_H
#define FC__MEM_H



#include <stdlib.h> /* size_t; actually stddef.h, but stdlib.h
                                 * might be more reliable? --dwp */

/* utility */
#include "log.h"
#include "support.h" /* fc__warn_unused_result */

/* fc_malloc, fc_realloc, fc_calloc:
 * fc_ stands for freeciv; the return value is checked,
 * and freeciv-specific processing occurs if it is NULL:
 * a log message, possibly cleanup, and ending with exit(1)
 */

#define fc_malloc(sz) malloc(sz)
#define fc_realloc(ptr, sz) realloc(ptr, sz)
#define fc_calloc(n, esz) calloc(n, esz)

#define NFCPP_FREE(ptr)                                                     \
if (ptr) delete[] (ptr);                                                    \

#define NFC_FREE(ptr)                                                       \
if (ptr) delete (ptr);                                                      \

#define FCPP_FREE(ptr)                                                      \
delete[] (ptr);                                                             \
(ptr) = NULL;

#define FC_FREE(ptr)                                                        \
  do {                                                                      \
    delete (ptr);                                                           \
    (ptr) = NULL;                                                           \
  } while (FALSE)

#define fc_strdup(str) real_fc_strdup((str), "strdup", __FC_LINE__, __FILE__)

/***********************************************************************/

char *real_fc_strdup(const char *str, const char *called_as, int line,
                     const char *file) fc__warn_unused_result;



#endif /* FC__MEM_H */
