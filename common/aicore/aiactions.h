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

/* common */
#include "fc_types.h"


bool aia_utype_is_considered_spy_vs_city(const struct unit_type *putype);
bool aia_utype_is_considered_spy(const struct unit_type *putype);
bool aia_utype_is_considered_worker(const struct unit_type *putype);
bool aia_utype_is_considered_caravan_trade(const struct unit_type *putype);
bool aia_utype_is_considered_caravan(const struct unit_type *putype);


