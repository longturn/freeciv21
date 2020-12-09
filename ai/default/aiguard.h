/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once
#include "support.h" /* bool type */

#include "fc_types.h"

#define CHECK_GUARD(ait, guard) aiguard_check_guard(ait, guard)
#define CHECK_CHARGE_UNIT(ait, charge) aiguard_check_charge_unit(ait, charge)

void aiguard_check_guard(struct ai_type *ait, const struct unit *guard);
void aiguard_check_charge_unit(struct ai_type *ait,
                               const struct unit *charge);
void aiguard_clear_charge(struct ai_type *ait, struct unit *guard);
void aiguard_clear_guard(struct ai_type *ait, struct unit *charge);
void aiguard_assign_guard_unit(struct ai_type *ait, struct unit *charge,
                               struct unit *guard);
void aiguard_assign_guard_city(struct ai_type *ait, struct city *charge,
                               struct unit *guard);
void aiguard_request_guard(struct ai_type *ait, struct unit *punit);
bool aiguard_wanted(struct ai_type *ait, struct unit *charge);
bool aiguard_has_charge(struct ai_type *ait, struct unit *charge);
bool aiguard_has_guard(struct ai_type *ait, struct unit *charge);
struct unit *aiguard_guard_of(struct ai_type *ait, struct unit *charge);
struct unit *aiguard_charge_unit(struct ai_type *ait, struct unit *guard);
struct city *aiguard_charge_city(struct ai_type *ait, struct unit *guard);
void aiguard_update_charge(struct ai_type *ait, struct unit *guard);

