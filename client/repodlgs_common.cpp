/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2023 Freeciv21 and Freeciv
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

// utility
#include "fcintl.h"
#include "log.h"
#include "support.h" // fc_snprintf()

// common
#include "game.h"
#include "government.h"

// client
#include "citydlg_common.h"
#include "client_main.h"
#include "control.h"
#include "repodlgs_common.h"

/**
   Fills out the array of struct improvement_entry given by
   entries. The array must be able to hold at least B_LAST entries.
 */
void get_economy_report_data(struct improvement_entry *entries,
                             int *num_entries_used, int *total_cost,
                             int *total_income)
{
  *num_entries_used = 0;
  *total_cost = 0;
  *total_income = 0;
  QString str;

  if (nullptr == client.conn.playing) {
    return;
  }

  improvement_iterate(pimprove)
  {
    if (is_improvement(pimprove)) {
      QStringList redundant_cities;
      int count = 0, cost = 0;
      city_list_iterate(client.conn.playing->cities, pcity)
      {
        if (city_has_building(pcity, pimprove)) {
          count++;
          cost += city_improvement_upkeep(pcity, pimprove);
          if (is_improvement_redundant(pcity, pimprove)) {
            redundant_cities.append(pcity->name);
          }
        }
      }
      city_list_iterate_end;

      if (count == 0) {
        continue;
      }

      if (redundant_cities.isEmpty()) {
        str = (_("None"));
      } else {
        // Convert the string list we built to a standard string for display.
        str = redundant_cities.join(", ");
      }

      entries[*num_entries_used].type = pimprove;
      entries[*num_entries_used].count = count;
      entries[*num_entries_used].redundant = redundant_cities.size();
      entries[*num_entries_used].total_cost = cost;
      entries[*num_entries_used].cost = cost / count;
      entries[*num_entries_used].city_names = str;
      (*num_entries_used)++;

      /* Currently there is no building expense under anarchy.  It's
       * not a good idea to hard-code this in the client, but what
       * else can we do? */
      if (government_of_player(client.conn.playing)
          != game.government_during_revolution) {
        *total_cost += cost;
      }
    }
  }
  improvement_iterate_end;

  city_list_iterate(client.conn.playing->cities, pcity)
  {
    *total_income += pcity->prod[O_GOLD];
    if (city_production_has_flag(pcity, IF_GOLD)) {
      *total_income += MAX(0, pcity->surplus[O_SHIELD]);
    }
  }
  city_list_iterate_end;
}

/**
   Returns an array of units with gold_upkeep. Number of units in
   the array is added to num_entries_used.
 */
void get_economy_report_units_data(struct unit_entry *entries,
                                   int *num_entries_used, int *total_cost)
{
  int count, cost, partial_cost;

  *num_entries_used = 0;
  *total_cost = 0;

  if (nullptr == client.conn.playing) {
    return;
  }

  unit_type_iterate(unittype)
  {
    cost = utype_upkeep_cost(unittype, client.conn.playing, O_GOLD);

    if (cost == 0) {
      // Short-circuit all of the following checks.
      continue;
    }

    count = 0;
    partial_cost = 0;

    city_list_iterate(client.conn.playing->cities, pcity)
    {
      unit_list_iterate(pcity->units_supported, punit)
      {
        if (unit_type_get(punit) == unittype) {
          count++;
          partial_cost += punit->upkeep[O_GOLD];
        }
      }
      unit_list_iterate_end;
    }
    city_list_iterate_end;

    if (count == 0) {
      continue;
    }

    (*total_cost) += partial_cost;

    entries[*num_entries_used].type = unittype;
    entries[*num_entries_used].count = count;
    entries[*num_entries_used].cost = cost;
    entries[*num_entries_used].total_cost = partial_cost;
    (*num_entries_used)++;
  }
  unit_type_iterate_end;
}

/**
   Sell all improvements of the given type in all cities.  If
 "redundant_only" is specified then only those improvements that are replaced
 will be sold.

   The "message" string will be filled with a GUI-friendly message about
   what was sold.
 */
void sell_all_improvements(const struct impr_type *pimprove,
                           bool redundant_only, char *message,
                           size_t message_sz)
{
  int count = 0, gold = 0;

  if (!can_client_issue_orders()) {
    fc_snprintf(message, message_sz, _("You cannot sell improvements."));
    return;
  }

  city_list_iterate(client.conn.playing->cities, pcity)
  {
    if (!pcity->did_sell && city_has_building(pcity, pimprove)
        && (!redundant_only || is_improvement_redundant(pcity, pimprove))) {
      count++;
      gold += impr_sell_gold(pimprove);
      city_sell_improvement(pcity, improvement_number(pimprove));
    }
  }
  city_list_iterate_end;

  if (count > 0) {
    // FIXME: plurality of count is ignored!
    /* TRANS: "Sold 3 Harbor for 90 gold." (Pluralisation is in gold --
     * second %d -- not in buildings.) */
    fc_snprintf(
        message, message_sz,
        PL_("Sold %d %s for %d gold.", "Sold %d %s for %d gold.", gold),
        count, improvement_name_translation(pimprove), gold);
  } else {
    fc_snprintf(message, message_sz, _("No %s could be sold."),
                improvement_name_translation(pimprove));
  }
}

/**
   Disband all supported units of the given type.  If in_cities_only is
   specified then only units inside our cities will be disbanded.

   The "message" string will be filled with a GUI-friendly message about
   what was sold.
 */
void disband_all_units(const struct unit_type *punittype,
                       bool in_cities_only, char *message, size_t message_sz)
{
  int count = 0;

  if (!can_client_issue_orders()) {
    // TRANS: Obscure observer error.
    fc_snprintf(message, message_sz, _("You cannot disband units."));
    return;
  }

  if (!utype_can_do_action(punittype, ACTION_DISBAND_UNIT)) {
    fc_snprintf(message, message_sz, _("%s cannot be disbanded."),
                utype_name_translation(punittype));
    return;
  }

  city_list_iterate(client.conn.playing->cities, pcity)
  {
    /* Only supported units are disbanded.  Units with no homecity have no
     * cost and are not disbanded. */
    unit_list_iterate(pcity->units_supported, punit)
    {
      struct city *incity = tile_city(unit_tile(punit));

      if (unit_type_get(punit) == punittype
          && (!in_cities_only
              || (incity && city_owner(incity) == client.conn.playing))) {
        count++;
        request_unit_disband(punit);
      }
    }
    unit_list_iterate_end;
  }
  city_list_iterate_end;

  if (count > 0) {
    fc_snprintf(message, message_sz, _("Disbanded %d %s."), count,
                utype_name_translation(punittype));
  } else {
    fc_snprintf(message, message_sz, _("No %s could be disbanded."),
                utype_name_translation(punittype));
  }
}
