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
#include "unit.h"     // for diplomat_actions
#include "unittype.h" // for unit_type_flag_id

#include <vector>

// get 'struct unit_list' and related functions:
#define SPECLIST_TAG unit
#define SPECLIST_TYPE struct unit
#include "speclist.h"

#define unit_list_iterate(unitlist, punit)                                  \
  TYPED_LIST_ITERATE(struct unit, unitlist, punit)
#define unit_list_iterate_end LIST_ITERATE_END
#define unit_list_both_iterate(unitlist, plink, punit)                      \
  TYPED_LIST_BOTH_ITERATE(struct unit_list_link, struct unit, unitlist,     \
                          plink, punit)
#define unit_list_both_iterate_end LIST_BOTH_ITERATE_END

#define unit_list_iterate_safe(unitlist, _unit)                             \
  {                                                                         \
    int _unit##_size = unit_list_size(unitlist);                            \
                                                                            \
    if (_unit##_size > 0) {                                                 \
      int _unit##_numbers[_unit##_size];                                    \
      int _unit##_index = 0;                                                \
                                                                            \
      unit_list_iterate(unitlist, _unit)                                    \
      {                                                                     \
        _unit##_numbers[_unit##_index++] = _unit->id;                       \
      }                                                                     \
      unit_list_iterate_end;                                                \
                                                                            \
      for (_unit##_index = 0; _unit##_index < _unit##_size;                 \
           _unit##_index++) {                                               \
        struct unit *_unit =                                                \
            game_unit_by_number(_unit##_numbers[_unit##_index]);            \
                                                                            \
        if (nullptr != _unit) {

#define unit_list_iterate_safe_end                                          \
  }                                                                         \
  }                                                                         \
  }                                                                         \
  }

struct unit *unit_list_find(const struct unit_list *punitlist, int unit_id);

void unit_list_sort_ord_map(struct unit_list *punitlist);
void unit_list_sort_ord_city(struct unit_list *punitlist);

bool can_units_do(const std::vector<unit *> &units,
                  bool(can_fn)(const struct unit *punit));
bool can_units_do_activity(const std::vector<unit *> &units,
                           enum unit_activity activity);
bool can_units_do_activity_targeted(const std::vector<unit *> &units,
                                    enum unit_activity activity,
                                    struct extra_type *pextra);
bool can_units_do_any_road(const std::vector<unit *> &units);
bool can_units_do_base_gui(const std::vector<unit *> &units,
                           enum base_gui_type base_gui);
bool units_have_type_flag(const std::vector<unit *> &units,
                          enum unit_type_flag_id flag, bool has_flag);
bool units_contain_cityfounder(const std::vector<unit *> &units);
bool units_can_do_action(const std::vector<unit *> &units, action_id act_id,
                         bool can_do);
bool units_are_occupied(const std::vector<unit *> &units);
bool units_can_load(const std::vector<unit *> &units);
bool units_can_unload(const std::vector<unit *> &units);
bool units_have_activity_on_tile(const std::vector<unit *> &units,
                                 enum unit_activity activity);

bool units_can_upgrade(const std::vector<unit *> &units);
bool units_can_convert(const std::vector<unit *> &units);
bool any_unit_in_city(const std::vector<unit *> &units);
bool units_on_the_same_tile(const std::vector<unit *> &units);
