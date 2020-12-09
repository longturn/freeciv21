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
#pragma once


/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.
***************************************************************************/

/* common */
#include "fc_types.h"
#include "world_object.h"

void idex_init(struct world *iworld);
void idex_free(struct world *iworld);

void idex_register_city(struct world *iworld, struct city *pcity);
void idex_register_unit(struct world *iworld, struct unit *punit);

void idex_unregister_city(struct world *iworld, struct city *pcity);
void idex_unregister_unit(struct world *iworld, struct unit *punit);

struct city *idex_lookup_city(struct world *iworld, int id);
struct unit *idex_lookup_unit(struct world *iworld, int id);


