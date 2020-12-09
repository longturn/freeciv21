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
#ifndef FC__CLIENTUTILS_H
#define FC__CLIENTUTILS_H


/* common */
#include "fc_types.h"

struct extra_type;
struct tile;
struct unit;

int turns_to_activity_done(const struct tile *ptile, Activity_type_id act,
                           const struct extra_type *tgt,
                           const struct unit *pnewunit);
const char *concat_tile_activity_text(struct tile *ptile);


#endif /* FC__CLIENTUTILS_H */
