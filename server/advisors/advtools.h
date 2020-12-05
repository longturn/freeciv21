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
#ifndef FC__ADVTOOLS_H
#define FC__ADVTOOLS_H

/* common */
#include "fc_types.h"

#define MORT 24

adv_want amortize(adv_want benefit, int delay);

/*
 * To prevent integer overflows the product "power * hp * firepower"
 * is divided by POWER_DIVIDER.
 *
 */
#define POWER_DIVIDER (POWER_FACTOR * 3)

#endif /* FC__ADVTOOLS_H */
