/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2022 Freeciv21 and Freeciv
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

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

// utility
#include "fcintl.h"
#include "log.h"
#include "nation.h"
#include "support.h"

// common
#include "city.h"
#include "culture.h"
#include "game.h"
#include "specialist.h"
#include "unitlist.h"

// client
#include "citydlg_common.h" // city_production_cost_str()
#include "governor.h"
#include "options.h"

#include "cityrepdata.h"

/**
   cr_entry = return an entry (one column for one city) for the city report
   These return ptrs to filled in static strings.
   Note the returned string may not be exactly the right length; that
   is handled later.
 */
static QString cr_entry_cityname(const struct city *pcity, const void *data)
{
  /* We used to truncate the name to 14 bytes.  This should not be needed
   * in any modern GUI library and may give an invalid string if a
   * multibyte character is clipped. */
  return city_name_get(pcity);
}

/**
   Translated name of nation who owns this city.
 */
static QString cr_entry_nation(const struct city *pcity, const void *data)
{
  return nation_adjective_for_player(city_owner(pcity));
}

/**
   Returns city size written to string. Returned string is statically
   allocated and its contents change when this function is called again.
 */
static QString cr_entry_size(const struct city *pcity, const void *data)
{
  static char buf[8];

  fc_snprintf(buf, sizeof(buf), "%2d", city_size_get(pcity));
  return buf;
}

/**
   Returns concise city happiness state written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_hstate_concise(const struct city *pcity,
                                       const void *data)
{
  if (city_unhappy(pcity)) {
    return "X"; // Disorder
  } else if (city_celebrating(pcity)) {
    return "*"; // Celebrating
  } else if (city_happy(pcity)) {
    return "+"; // Happy
  } else {
    return " "; // Content
  }
  static char buf[4];
  fc_snprintf(
      buf, sizeof(buf), "%s",
      (city_celebrating(pcity) ? "*" : (city_unhappy(pcity) ? "X" : " ")));
  return buf;
}

/**
   Returns verbose city happiness state written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_hstate_verbose(const struct city *pcity,
                                       const void *data)
{
  Q_UNUSED(data);
  return get_city_dialog_status_text(pcity);
}

/**
   Returns number of citizens of each happiness state written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_workers(const struct city *pcity, const void *data)
{
  static char buf[32];

  fc_snprintf(buf, sizeof(buf), "%d/%d/%d/%d",
              pcity->feel[CITIZEN_HAPPY][FEELING_FINAL],
              pcity->feel[CITIZEN_CONTENT][FEELING_FINAL],
              pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL],
              pcity->feel[CITIZEN_ANGRY][FEELING_FINAL]);
  return buf;
}

/**
   Returns number of happy citizens written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_happy(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_HAPPY][FEELING_FINAL]);
  return buf;
}

/**
   Returns city total culture written to string
 */
static QString cr_entry_culture(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->client.culture);
  return buf;
}

/**
   Returns city history culture value written to string
 */
static QString cr_entry_history(const struct city *pcity, const void *data)
{
  static char buf[20];
  int perturn = city_history_gain(pcity);

  if (perturn != 0) {
    fc_snprintf(buf, sizeof(buf), "%3d (%+d)", pcity->history, perturn);
  } else {
    fc_snprintf(buf, sizeof(buf), "%3d", pcity->history);
  }
  return buf;
}

/**
   Returns city performance culture value written to string
 */
static QString cr_entry_performance(const struct city *pcity,
                                    const void *data)
{
  static char buf[8];

  /*
   * Infer the actual performance component of culture from server-supplied
   * values, rather than using the client's guess at EFT_PERFORMANCE.
   * XXX: if culture ever gets more complicated than history+performance,
   * this will need revising, possibly to use a server-supplied value.
   */
  fc_snprintf(buf, sizeof(buf), "%3d",
              pcity->client.culture - pcity->history);
  return buf;
}

/**
   Returns number of content citizens written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_content(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_CONTENT][FEELING_FINAL]);
  return buf;
}

/**
   Returns number of unhappy citizens written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_unhappy(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]);
  return buf;
}

/**
   Returns number of angry citizens written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_angry(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_ANGRY][FEELING_FINAL]);
  return buf;
}

/**
   Returns list of specialists written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_specialists(const struct city *pcity,
                                    const void *data)
{
  return specialists_string(pcity->specialists);
}

/**
   Returns number of specialists of type given as data written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_specialist(const struct city *pcity,
                                   const void *data)
{
  static char buf[8];
  const struct specialist *sp = static_cast<const specialist *>(data);

  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->specialists[specialist_index(sp)]);
  return buf;
}

/**
   Returns string with best attack values of units in city.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_attack(const struct city *pcity, const void *data)
{
  static char buf[32];
  int attack_best[4] = {-1, -1, -1, -1}, i;

  unit_list_iterate(pcity->tile->units, punit)
  {
    // What about allied units?  Should we just count them?
    attack_best[3] = unit_type_get(punit)->attack_strength;

    /* Now that the element is appended to the end of the list, we simply
       do an insertion sort. */
    for (i = 2; i >= 0 && attack_best[i] < attack_best[i + 1]; i--) {
      int tmp = attack_best[i];
      attack_best[i] = attack_best[i + 1];
      attack_best[i + 1] = tmp;
    }
  }
  unit_list_iterate_end;

  buf[0] = '\0';
  for (i = 0; i < 3; i++) {
    if (attack_best[i] >= 0) {
      cat_snprintf(buf, sizeof(buf), "%s%d", (i > 0) ? "/" : "",
                   attack_best[i]);
    } else {
      cat_snprintf(buf, sizeof(buf), "%s-", (i > 0) ? "/" : "");
    }
  }

  return buf;
}

/**
   Returns string with best defend values of units in city.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_defense(const struct city *pcity, const void *data)
{
  static char buf[32];
  int defense_best[4] = {-1, -1, -1, -1}, i;

  unit_list_iterate(pcity->tile->units, punit)
  {
    // What about allied units?  Should we just count them?
    defense_best[3] = unit_type_get(punit)->defense_strength;

    /* Now that the element is appended to the end of the list, we simply
       do an insertion sort. */
    for (i = 2; i >= 0 && defense_best[i] < defense_best[i + 1]; i--) {
      int tmp = defense_best[i];

      defense_best[i] = defense_best[i + 1];
      defense_best[i + 1] = tmp;
    }
  }
  unit_list_iterate_end;

  buf[0] = '\0';
  for (i = 0; i < 3; i++) {
    if (defense_best[i] >= 0) {
      cat_snprintf(buf, sizeof(buf), "%s%d", (i > 0) ? "/" : "",
                   defense_best[i]);
    } else {
      cat_snprintf(buf, sizeof(buf), "%s-", (i > 0) ? "/" : "");
    }
  }

  return buf;
}

/**
   Returns number of supported units written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_supported(const struct city *pcity, const void *data)
{
  static char buf[8];
  int num_supported = unit_list_size(pcity->units_supported);

  fc_snprintf(buf, sizeof(buf), "%2d", num_supported);

  return buf;
}

/**
   Returns number of present units written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_present(const struct city *pcity, const void *data)
{
  static char buf[8];
  int num_present = unit_list_size(pcity->tile->units);

  fc_snprintf(buf, sizeof(buf), "%2d", num_present);

  return buf;
}

/**
   Returns string listing amounts of resources.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_resources(const struct city *pcity, const void *data)
{
  static char buf[32];
  fc_snprintf(buf, sizeof(buf), "%d/%d/%d", pcity->surplus[O_FOOD],
              pcity->surplus[O_SHIELD], pcity->surplus[O_TRADE]);
  return buf;
}

/**
   Returns food surplus written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_foodplus(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_FOOD]);
  return buf;
}

/**
   Returns production surplus written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_prodplus(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_SHIELD]);
  return buf;
}

/**
   Returns trade surplus written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_tradeplus(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_TRADE]);
  return buf;
}

/**
   Returns string describing resource output.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_output(const struct city *pcity, const void *data)
{
  static char buf[32];
  int goldie = pcity->surplus[O_GOLD];

  fc_snprintf(buf, sizeof(buf), "%3d/%d/%d", goldie, pcity->prod[O_LUXURY],
              pcity->prod[O_SCIENCE]);
  return buf;
}

/**
   Returns gold surplus written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_gold(const struct city *pcity, const void *data)
{
  static char buf[8];

  if (pcity->surplus[O_GOLD] > 0) {
    fc_snprintf(buf, sizeof(buf), "+%d", pcity->surplus[O_GOLD]);
  } else {
    fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_GOLD]);
  }
  return buf;
}

/**
   Returns luxury output written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_luxury(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->prod[O_LUXURY]);
  return buf;
}

/**
   Returns science output written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_science(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->prod[O_SCIENCE]);
  return buf;
}

/**
   Returns number of turns before city grows written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_growturns(const struct city *pcity, const void *data)
{
  int turns = city_turns_to_grow(pcity);
  char buffer[8];
  static char buf[32];

  if (turns == FC_INFINITY) {
    // 'never' wouldn't be easily translatable here.
    fc_snprintf(buffer, sizeof(buffer), "---");
  } else {
    // Shrinking cities get a negative value.
    fc_snprintf(buffer, sizeof(buffer), "%4d", turns);
  }
  fc_snprintf(buf, sizeof(buf), "%s (%d/%d)", buffer, pcity->food_stock,
              city_granary_size(city_size_get(pcity)));
  return buf;
}

/**
   Returns pollution output written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_pollution(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->pollution);
  return buf;
}

/**
   Returns number and output of trade routes written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_trade_routes(const struct city *pcity,
                                     const void *data)
{
  static char buf[16];
  int num = 0, value = 0;

  trade_routes_iterate(pcity, proute)
  {
    num++;
    value += proute->value;
  }
  trade_routes_iterate_end;

  if (0 == num) {
    sz_strlcpy(buf, "0");
  } else {
    fc_snprintf(buf, sizeof(buf), "%d (+%d)", num, value);
  }
  return buf;
}

/**
   Returns number of build slots written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_build_slots(const struct city *pcity,
                                    const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", city_build_slots(pcity));
  return buf;
}

/**
   Returns name of current production.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_building(const struct city *pcity, const void *data)
{
  static char buf[192];
  const char *from_worklist = worklist_is_empty(&pcity->worklist) ? ""
                              : gui_options->concise_city_production
                                  ? "+"
                                  : _("(worklist)");

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_snprintf(buf, sizeof(buf), "%s (%d)%s",
                city_production_name_translation(pcity),
                MAX(0, pcity->surplus[O_SHIELD]), from_worklist);
  } else {
    fc_snprintf(buf, sizeof(buf), "%s (%d/%s)%s",
                city_production_name_translation(pcity), pcity->shield_stock,
                city_production_cost_str(pcity), from_worklist);
  }

  return buf;
}

/**
   Returns cost of buying current production and turns to completion
   written to string. Returned string is statically allocated and its
   contents change when this function is called again.
 */
static QString cr_entry_build_cost(const struct city *pcity,
                                   const void *data)
{
  Q_UNUSED(data)
  char bufone[8];
  char buftwo[8];
  static char buf[32];
  int price;
  int turns;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_snprintf(buf, sizeof(buf), "*");
    return buf;
  }
  price = pcity->client.buy_cost;
  turns = city_production_turns_to_build(pcity, true);

  if (price > 99999) {
    fc_snprintf(bufone, sizeof(bufone), "---");
  } else {
    fc_snprintf(bufone, sizeof(bufone), "%d", price);
  }
  if (turns > 999) {
    fc_snprintf(buftwo, sizeof(buftwo), "--");
  } else {
    fc_snprintf(buftwo, sizeof(buftwo), "%3d", turns);
  }
  fc_snprintf(buf, sizeof(buf), "%s/%s", buftwo, bufone);
  return buf;
}

/**
 * Returns cost of buying current production only written to string.
 * Returned string is statically allocated and its contents change
 * when this function is called again.
 */
static QString cr_entry_build_cost_gold(const struct city *pcity,
                                        const void *data)
{
  Q_UNUSED(data)
  char bufone[8];
  static char buf[32];
  int price;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_snprintf(buf, sizeof(buf), "*");
    return buf;
  }
  price = pcity->client.buy_cost;

  if (price > 99999) {
    fc_snprintf(bufone, sizeof(bufone), "---");
  } else {
    fc_snprintf(bufone, sizeof(bufone), "%d", price);
  }
  fc_snprintf(buf, sizeof(buf), "%s", bufone);
  return buf;
}

/**
 * Returns current production turns to completion written to string.
 * Returned string is statically allocated and its contents change
 * when this function is called again.
 */
static QString cr_entry_build_cost_turns(const struct city *pcity,
                                         const void *data)
{
  Q_UNUSED(data)
  char bufone[8];
  static char buf[32];
  int turns;

  turns = city_production_turns_to_build(pcity, true);

  if (turns > 999) {
    fc_snprintf(bufone, sizeof(bufone), "--");
  } else {
    fc_snprintf(bufone, sizeof(bufone), "%3d", turns);
  }
  fc_snprintf(buf, sizeof(buf), "%s", bufone);
  return buf;
}

/**
   Returns corruption amount written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_corruption(const struct city *pcity,
                                   const void *data)
{
  Q_UNUSED(data)
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", -(pcity->waste[O_TRADE]));
  return buf;
}

/**
   Returns waste amount written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_waste(const struct city *pcity, const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", -(pcity->waste[O_SHIELD]));
  return buf;
}

/**
   Returns risk percentage of plague written to string.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_plague_risk(const struct city *pcity,
                                    const void *data)
{
  Q_UNUSED(data)
  static char buf[8];
  if (!game.info.illness_on) {
    fc_snprintf(buf, sizeof(buf), " -.-");
  } else {
    fc_snprintf(buf, sizeof(buf), "%4.1f",
                static_cast<float>(city_illness_calc(pcity, nullptr, nullptr,
                                                     nullptr, nullptr))
                    / 10.0);
  }
  return buf;
}

/**
   Returns number of continent
 */
static QString cr_entry_continent(const struct city *pcity, const void *data)
{
  Q_UNUSED(data)
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->tile->continent);
  return buf;
}

/**
   Returns city cma description.
   Returned string is statically allocated and its contents change when
   this function is called again.
 */
static QString cr_entry_cma(const struct city *pcity, const void *data)
{
  Q_UNUSED(data)
  return cmafec_get_short_descr_of_city(pcity);
}

/* City report options (which columns get shown)
 * To add a new entry, you should just have to:
 * - add a function like those above
 * - add an entry in the base_city_report_specs[] table
 */

// This generates the function name and the tagname:
#define FUNC_TAG(var) cr_entry_##var, #var

static const struct city_report_spec base_city_report_specs[] = {
    // CITY columns
    {true, -15, 0, nullptr, N_("?city:Name"), N_("City: Name"), nullptr,
     FUNC_TAG(cityname)},
    {false, -15, 0, nullptr, N_("Nation"), N_("City: Nation"), nullptr,
     FUNC_TAG(nation)},
    {false, 3, 1, nullptr, N_("?Continent:Cn"), N_("City: Continent number"),
     nullptr, FUNC_TAG(continent)},
    {true, 2, 1, nullptr, N_("?size [short]:Sz"), N_("City: Size"), nullptr,
     FUNC_TAG(size)},
    {// TRANS: Header "It will take this many turns before city grows"
     true, 14, 1, N_("?food (population):Grow"),
     N_("?Stock/Target:(Have/Need)"), N_("City: Turns until growth/famine"),
     nullptr, FUNC_TAG(growturns)},
    {true, -8, 1, nullptr, N_("State"),
     N_("City: State: Celebrating/Happy/Peace/Disorder"), nullptr,
     FUNC_TAG(hstate_verbose)},
    {false, 1, 1, nullptr, nullptr,
     N_("City: Concise state: *=Celebrating, +=Happy, X=Disorder"), nullptr,
     FUNC_TAG(hstate_concise)},
    {true, 15, 1, nullptr, N_("?cma:Governor"), N_("City: Governor"),
     nullptr, FUNC_TAG(cma)},
    {false, 4, 1, N_("?plague risk [short]:Pla"), N_("(%)"),
     N_("City: Plague risk per turn"), nullptr, FUNC_TAG(plague_risk)},

    // CITIZEN columns
    {false, 2, 1, nullptr, N_("?Happy workers:H"), N_("Citizens: Happy"),
     nullptr, FUNC_TAG(happy)},
    {false, 2, 1, nullptr, N_("?Content workers:C"), N_("Citizens: Content"),
     nullptr, FUNC_TAG(content)},
    {false, 2, 1, nullptr, N_("?Unhappy workers:U"), N_("Citizens: Unhappy"),
     nullptr, FUNC_TAG(unhappy)},
    {false, 2, 1, nullptr, N_("?Angry workers:A"), N_("Citizens: Angry"),
     nullptr, FUNC_TAG(angry)},
    {false, 10, 1, N_("?city:Citizens"),
     N_("?happy/content/unhappy/angry:H/C/U/A"),
     N_("Citizens: Concise: Happy/Content/Unhappy/Angry"), nullptr,
     FUNC_TAG(workers)},

    // UNIT columns
    {false, 8, 1, N_("Best"), N_("attack"), N_("Units: Best attackers"),
     nullptr, FUNC_TAG(attack)},
    {false, 8, 1, N_("Best"), N_("defense"), N_("Units: Best defenders"),
     nullptr, FUNC_TAG(defense)},
    {false, 2, 1, N_("Units"),
     // TRANS: Header "Number of units inside city"
     N_("?Present (units):Here"), N_("Units: Number present"), nullptr,
     FUNC_TAG(present)},
    {false, 2, 1, N_("Units"),
     // TRANS: Header "Number of units supported by given city"
     N_("?Supported (units):Owned"), N_("Units: Number supported"), nullptr,
     FUNC_TAG(supported)},
    // TRANS: "UpT" = "Units per turn"
    {false, 3, 1, nullptr, N_("UpT"),
     N_("Units: Maximum buildable per turn"), nullptr,
     FUNC_TAG(build_slots)},

    // RESOURCE columns
    {false, 10, 1, N_("Surplus"), N_("?food/production/trade:F/P/T"),
     N_("Resources: Food, Production, Trade"), nullptr, FUNC_TAG(resources)},
    {true, 3, 1, nullptr, N_("?Food surplus [short]:+F"),
     N_("Resources: Food"), nullptr, FUNC_TAG(foodplus)},
    {true, 3, 1, nullptr, N_("?Production surplus [short]:+P"),
     N_("Resources: Production"), nullptr, FUNC_TAG(prodplus)},
    {false, 3, 1, nullptr, N_("?Production loss (waste) [short]:-P"),
     N_("Resources: Waste (lost production)"), nullptr, FUNC_TAG(waste)},
    {true, 3, 1, nullptr, N_("?Trade surplus [short]:+T"),
     N_("Resources: Trade"), nullptr, FUNC_TAG(tradeplus)},
    {true, 3, 1, nullptr, N_("?Trade loss (corruption) [short]:-T"),
     N_("Resources: Corruption (lost trade)"), nullptr,
     FUNC_TAG(corruption)},
    {false, 1, 1, N_("?number_trade_routes:n"), N_("?number_trade_routes:R"),
     N_("Resources: Number (and total value) of trade routes"), nullptr,
     FUNC_TAG(trade_routes)},
    {false, 3, 1, nullptr, N_("?pollution [short]:Pol"),
     N_("Production: Pollution"), nullptr, FUNC_TAG(pollution)},

    // ECONOMY columns
    {false, 10, 1, N_("Surplus"), N_("?gold/luxury/science:G/L/S"),
     N_("Surplus: Gold, Luxuries, Science"), nullptr, FUNC_TAG(output)},
    {true, 3, 1, nullptr, N_("?Gold:G"), N_("Surplus: Gold"), nullptr,
     FUNC_TAG(gold)},
    {false, 3, 1, nullptr, N_("?Luxury:L"), N_("Surplus: Luxury Goods"),
     nullptr, FUNC_TAG(luxury)},
    {true, 3, 1, nullptr, N_("?Science:S"), N_("Surplus: Science"), nullptr,
     FUNC_TAG(science)},

    // CULTURE columns
    {false, 3, 1, nullptr, N_("?Culture:Clt"),
     N_("Culture: History + Performance"), nullptr, FUNC_TAG(culture)},
    {false, 3, 1, nullptr, N_("?History:Hst"),
     N_("Culture: History (and gain per turn)"), nullptr, FUNC_TAG(history)},
    {false, 3, 1, nullptr, N_("?Performance:Prf"),
     N_("Culture: Performance"), nullptr, FUNC_TAG(performance)},

    // PRODUCTION columns
    {true, 9, 1, N_("Production"), N_("Turns/Buy"),
     /*N_("Turns or gold to complete production"), future menu needs
        translation */
     N_("Production: Turns/gold to complete"), nullptr,
     FUNC_TAG(build_cost)},
    {false, 0, 1, N_("Buy"), N_("(Gold)"), N_("Production: Buy Cost"),
     nullptr, FUNC_TAG(build_cost_gold)},
    {false, 0, 1, N_("Finish"), N_("(Turns)"), N_("Production: Build Turns"),
     nullptr, FUNC_TAG(build_cost_turns)},
    {true, 0, 1, N_("Currently Building"), N_("?Stock/Target:(Have/Need)"),
     N_("Production: Currently Building"), nullptr, FUNC_TAG(building)}};

std::vector<city_report_spec> city_report_specs;
static int num_creport_cols;

/**
   Simple wrapper for num_creport_cols()
 */
int num_city_report_spec() { return num_creport_cols; }

/**
   Simple wrapper for city_report_specs.show
 */
bool *city_report_spec_show_ptr(int i)
{
  return &(city_report_specs[i].show);
}

/**
   Simple wrapper for city_report_specs.tagname
 */
const char *city_report_spec_tagname(int i)
{
  return city_report_specs[i].tagname;
}

/**
   Initialize city report data.  This deals with ruleset-depedent
   columns and pre-translates the fields (to make things easier on
   the GUI writers).  Should be called before the GUI starts up.
 */
void init_city_report_game_data()
{
  static char sp_explanation[SP_MAX][128];
  static char sp_explanations[SP_MAX * 128];
  struct city_report_spec *p;
  int i;

  num_creport_cols =
      ARRAY_SIZE(base_city_report_specs) + specialist_count() + 1;
  city_report_specs = std::vector<city_report_spec>(num_creport_cols);
  p = &city_report_specs[0];

  fc_snprintf(sp_explanations, sizeof(sp_explanations), "%s",
              _("Specialists: "));
  specialist_type_iterate(sp)
  {
    struct specialist *s = specialist_by_number(sp);

    p->show = false;
    p->width = 2;
    p->space = 1;
    p->title1 = Q_("?specialist:S");
    p->title2 = specialist_abbreviation_translation(s);
    fc_snprintf(sp_explanation[sp], sizeof(sp_explanation[sp]),
                _("Specialists: %s"), specialist_plural_translation(s));
    cat_snprintf(sp_explanations, sizeof(sp_explanations), "%s%s",
                 (sp == 0) ? "" : ", ", specialist_plural_translation(s));
    p->explanation = sp_explanation[sp];
    p->data = s;
    p->func = cr_entry_specialist;
    p->tagname = specialist_rule_name(s);
    p++;
  }
  specialist_type_iterate_end;

  // Summary column for all specialists.
  {
    static char sp_summary[128];

    p->show = false;
    p->width = MAX(7, specialist_count() * 2 - 1);
    p->space = 1;
    p->title1 = _("Special");
    fc_snprintf(sp_summary, sizeof(sp_summary), "%s",
                specialists_abbreviation_string());
    p->title2 = sp_summary;
    p->explanation = sp_explanations;
    p->data = nullptr;
    p->func = cr_entry_specialists;
    p->tagname = "specialists";
    p++;
  }

  memcpy(p, base_city_report_specs, sizeof(base_city_report_specs));

  for (i = 0; i < ARRAY_SIZE(base_city_report_specs); i++) {
    if (p->title1) {
      p->title1 = Q_(p->title1);
    }
    if (p->title2) {
      p->title2 = Q_(p->title2);
    }
    p->explanation = _(p->explanation);
    p++;
  }

  fc_assert(NUM_CREPORT_COLS
            == ARRAY_SIZE(base_city_report_specs) + specialist_count() + 1);
}

/**
  The following several functions allow intelligent sorting city report
  fields by column.  This doesn't necessarily do the right thing, but
  it's better than sorting alphabetically.

  The GUI gives us two values to compare (as strings).  We try to split
  them into an array of numeric and string fields, then we compare
  lexicographically.  Two numeric fields are compared in the obvious
  way, two character fields are compared alphabetically.  Arbitrarily, a
  numeric field is sorted before a character field (for "justification"
  note that numbers are before letters in the ASCII table).
 */

/* A datum is one short string, or one number.
   A datum_vector represents a long string of alternating strings and
   numbers.
 */
struct datum {
  union {
    float numeric_value;
    char *string_value;
  } val;
  bool is_numeric;
};
#define SPECVEC_TAG datum
#include "specvec.h"

/**
   Init a datum from a substring.
 */
static void init_datum_string(struct datum *dat, const char *left,
                              const char *right)
{
  int len = right - left;

  dat->is_numeric = false;
  dat->val.string_value = new char[len + 1];
  memcpy(dat->val.string_value, left, len);
  dat->val.string_value[len] = 0;
}

/**
   Init a datum from a number (a float because we happen to use
   strtof).
 */
static void init_datum_number(struct datum *dat, float val)
{
  dat->is_numeric = true;
  dat->val.numeric_value = val;
}

/**
   Free the data associated with a datum -- that is, free the string if
   it was allocated.
 */
static void free_datum(struct datum *dat)
{
  if (!dat->is_numeric) {
    delete[] dat->val.string_value;
  }
}

/**
   Compare two data items as described above:
   - numbers in the obvious way
   - strings alphabetically
   - number < string for no good reason
 */
static int datum_compare(const struct datum *a, const struct datum *b)
{
  if (a->is_numeric == b->is_numeric) {
    if (a->is_numeric) {
      if (a->val.numeric_value == b->val.numeric_value) {
        return 0;
      } else if (a->val.numeric_value < b->val.numeric_value) {
        return -1;
      } else if (a->val.numeric_value > b->val.numeric_value) {
        return +1;
      } else {
        return 0; // shrug
      }
    } else {
      return strcmp(a->val.string_value, b->val.string_value);
    }
  } else {
    if (a->is_numeric) {
      return -1;
    } else {
      return 1;
    }
  }
}

/**
   Compare two strings of data lexicographically.
 */
static int data_compare(const struct datum_vector *a,
                        const struct datum_vector *b)
{
  int i, n;

  n = MIN(a->size, b->size);

  for (i = 0; i < n; i++) {
    int cmp = datum_compare(&a->p[i], &b->p[i]);

    if (cmp != 0) {
      return cmp;
    }
  }

  /* The first n fields match; whoever has more fields goes last.
     If they have equal numbers, the two really are equal. */
  return a->size - b->size;
}

/**
   Split a string into a vector of datum.
 */
static void split_string(struct datum_vector *data, const char *str)
{
  const char *string_start;

  datum_vector_init(data);
  string_start = str;
  while (*str) {
    char *endptr;
    float value;

    errno = 0;
    value = strtof(str, &endptr);
    if (errno != 0 || endptr == str || !std::isfinite(value)) {
      // that wasn't a sensible number; go on
      str++;
    } else {
      /* that was a number, so stop the string we were parsing, add
         it (unless it's empty), then add the number we just parsed */
      struct datum d;

      if (str != string_start) {
        init_datum_string(&d, string_start, str);
        datum_vector_append(data, d);
      }

      init_datum_number(&d, value);
      datum_vector_append(data, d);

      // finally, update the string position pointers
      string_start = str = endptr;
    }
  }

  // if we have anything leftover then it's a string
  if (str != string_start) {
    struct datum d;

    init_datum_string(&d, string_start, str);
    datum_vector_append(data, d);
  }
}

/**
   Free every datum in the vector.
 */
static void free_data(struct datum_vector *data)
{
  int i;

  for (i = 0; i < data->size; i++) {
    free_datum(&data->p[i]);
  }
  datum_vector_free(data);
}

/**
   The real function: split the two strings, and compare them.
 */
int cityrepfield_compare(const char *str1, const char *str2)
{
  struct datum_vector data1, data2;
  int retval;

  if (str1 == str2) {
    return 0;
  } else if (nullptr == str1) {
    return 1;
  } else if (nullptr == str2) {
    return -1;
  }

  split_string(&data1, str1);
  split_string(&data2, str2);

  retval = data_compare(&data1, &data2);

  free_data(&data1);
  free_data(&data2);

  return retval;
}

/**
   Same as can_city_sell_building(), but with universal argument.
 */
bool can_city_sell_universal(const struct city *pcity,
                             const struct universal *target)
{
  return target->kind == VUT_IMPROVEMENT
         && can_city_sell_building(pcity, target->value.building);
}
