// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.
***************************************************************************/

// common
#include "city.h" // struct city
#include "fc_types.h"
#include "unit.h" // struct unit

void idex_init(struct world *iworld);
void idex_free(struct world *iworld);

void idex_register_city(struct world *iworld, struct city *pcity);
void idex_register_unit(struct world *iworld, struct unit *punit);

void idex_unregister_city(struct world *iworld, struct city *pcity);
void idex_unregister_unit(struct world *iworld, struct unit *punit);

struct city *idex_lookup_city(struct world *iworld, int id);
struct unit *idex_lookup_unit(struct world *iworld, int id);
