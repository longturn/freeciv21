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

#include "fc_types.h"

#if (IS_BETA_VERSION || IS_DEVEL_VERSION) || defined(FREECIV_DEBUG)
#define SANITY_CHECKING
#endif

#ifdef SANITY_CHECKING

#define sanity_check_city(x)                                                \
  real_sanity_check_city(x, __FILE__, __FUNCTION__, __FC_LINE__)
void real_sanity_check_city(struct city *pcity, const char *file,
                            const char *function, int line);

#define sanity_check_tile(x)                                                \
  real_sanity_check_tile(x, __FILE__, __FUNCTION__, __FC_LINE__)
void real_sanity_check_tile(struct tile *ptile, const char *file,
                            const char *function, int line);

#define sanity_check() real_sanity_check(__FILE__, __FUNCTION__, __FC_LINE__)
void real_sanity_check(const char *file, const char *function, int line);

#else /* SANITY_CHECKING */

#define sanity_check_city(x) (void) 0
#define sanity_check_tile(x) (void) 0
#define sanity_check() (void) 0

#endif /* SANITY_CHECKING */


