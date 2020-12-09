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



#include "fc_types.h"
#include "improvement.h"
#include "unittype.h"

#include "citydlg_common.h" /* for city request functions */

struct improvement_entry {
  struct impr_type *type;
  int count, redundant, cost, total_cost;
};

struct unit_entry {
  struct unit_type *type;
  int count, cost, total_cost;
};

void get_economy_report_data(struct improvement_entry *entries,
                             int *num_entries_used, int *total_cost,
                             int *total_income);
/* This function returns an array with the gold upkeeped units.
 * FIXME: Many clients doesn't yet use this function and show also only the
 * buildings in the economy reports
 * I think that there should be only one function which returns an array of
 * char* arrays like some other common functions but that means updating all
 * client simultaneously and I simply can't */
void get_economy_report_units_data(struct unit_entry *entries,
                                   int *num_entries_used, int *total_cost);

void sell_all_improvements(const struct impr_type *pimprove,
                           bool redundant_only, char *message,
                           size_t message_sz);
void disband_all_units(const struct unit_type *punittype,
                       bool in_cities_only, char *message,
                       size_t message_sz);




