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

#include <math.h>

#include "advtools.h"

/**********************************************************************/ /**
   Amortize means gradually paying off a cost or debt over time. In freeciv
   terms this means we calculate how much less worth something is to us
   depending on how long it will take to complete.

   This is based on a global interest rate as defined by the MORT value.
 **************************************************************************/
adv_want amortize(adv_want benefit, int delay)
{
  double discount = 1.0 - 1.0 / ((double) MORT);

  /* Note there's no rounding here.  We could round but it would probably
   * be better just to return (and take) a double for the benefit. */
  return benefit * pow(discount, delay);
}
