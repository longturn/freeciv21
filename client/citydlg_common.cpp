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

#include <cstdarg>

// utility
#include "fcintl.h"
#include "log.h"
#include "support.h"

// common
#include "city.h"
#include "culture.h"
#include "game.h"
#include "specialist.h"

/* client/include */
#include "citydlg_g.h"
// client
#include "citydlg_common.h"
#include "client_main.h" // for can_client_issue_orders()
#include "climap.h"
#include "mapview_common.h"
#include "options.h"  // for concise_city_production
#include "tilespec.h" // for tileset_is_isometric(tileset)

static int citydlg_map_width, citydlg_map_height;

/**
   Return the width of the city dialog canvas.
 */
int get_citydlg_canvas_width() { return citydlg_map_width; }

/**
   Return the height of the city dialog canvas.
 */
int get_citydlg_canvas_height() { return citydlg_map_height; }

/**
   Calculate the citydlg width and height.
 */
void generate_citydlg_dimensions()
{
  int min_x = 0, max_x = 0, min_y = 0, max_y = 0;
  int max_rad = rs_max_city_radius_sq();

  // use maximum possible squared city radius.
  city_map_iterate_without_index(max_rad, city_x, city_y)
  {
    float canvas_x, canvas_y;

    map_to_gui_vector(tileset, &canvas_x, &canvas_y, CITY_ABS2REL(city_x),
                      CITY_ABS2REL(city_y));

    min_x = MIN(canvas_x, min_x);
    max_x = MAX(canvas_x, max_x);
    min_y = MIN(canvas_y, min_y);
    max_y = MAX(canvas_y, max_y);
  }
  city_map_iterate_without_index_end;

  citydlg_map_width = max_x - min_x + tileset_tile_width(tileset);
  citydlg_map_height = max_y - min_y + tileset_tile_height(tileset);
}

/**
   Return a string describing the cost for the production of the city
   considerung several build slots for units.
 */
char *city_production_cost_str(const struct city *pcity)
{
  static char cost_str[50];
  int cost = city_production_build_shield_cost(pcity);
  int build_slots = city_build_slots(pcity);
  int num_units;

  if (build_slots > 1
      && city_production_build_units(pcity, true, &num_units)) {
    // the city could build more than one unit of the selected type
    if (num_units == 0) {
      // no unit will be finished this turn but one is build
      num_units++;
    }

    if (build_slots > num_units) {
      // some build slots for units will be unused
      fc_snprintf(cost_str, sizeof(cost_str), "{%d*%d}", num_units, cost);
    } else {
      // maximal number of units will be build
      fc_snprintf(cost_str, sizeof(cost_str), "[%d*%d]", num_units, cost);
    }
  } else {
    // nothing special
    fc_snprintf(cost_str, sizeof(cost_str), "%3d", cost);
  }

  return cost_str;
}

/**
   Find the city dialog city production text for the given city, and
   place it into the buffer.  This will check the
   concise_city_production option.  pcity may be nullptr; in this case a
   filler string is returned.
 */
void get_city_dialog_production(struct city *pcity, char *buffer,
                                size_t buffer_len)
{
  char time_str[50], *cost_str;
  int turns, stock;

  if (pcity == nullptr) {
    /*
     * Some GUIs use this to build a "filler string" so that they can
     * properly size the widget to hold the string.  This has some
     * obvious problems; the big one is that we have two forms of time
     * information: "XXX turns" and "never".  Later this may need to
     * be extended to return the longer of the two; in the meantime
     * translators can fudge it by changing this "filler" string.
     */
    // TRANS: Use longer of "XXX turns" and "never"
    fc_strlcpy(buffer, Q_("?filler:XXX/XXX XXX turns"), buffer_len);

    return;
  }

  if (city_production_has_flag(pcity, IF_GOLD)) {
    int gold = MAX(0, pcity->surplus[O_SHIELD]);
    fc_snprintf(buffer, buffer_len,
                PL_("%3d gold per turn", "%3d gold per turn", gold), gold);
    return;
  }

  turns = city_production_turns_to_build(pcity, true);
  stock = pcity->shield_stock;
  cost_str = city_production_cost_str(pcity);

  if (turns < FC_INFINITY) {
    if (gui_options.concise_city_production) {
      fc_snprintf(time_str, sizeof(time_str), "%3d", turns);
    } else {
      fc_snprintf(time_str, sizeof(time_str),
                  PL_("%3d turn", "%3d turns", turns), turns);
    }
  } else {
    fc_snprintf(time_str, sizeof(time_str), "%s",
                gui_options.concise_city_production ? "-" : _("never"));
  }

  if (gui_options.concise_city_production) {
    fc_snprintf(buffer, buffer_len, _("%3d/%s:%s"), stock, cost_str,
                time_str);
  } else {
    fc_snprintf(buffer, buffer_len, _("%3d/%s %s"), stock, cost_str,
                time_str);
  }
}

/**
   Helper structure to accumulate a breakdown of the constributions
   to some numeric city property. Contributions are returned in order,
   with duplicates merged.
 */
struct msum {
  // The net value that is accumulated.
  double value;
  /* Description; compared for duplicate-merging.
   * Both of these are maintained/compared until the net 'value' is known;
   * then posdesc is used if value>=0, negdesc if value<0 */
  QString posdesc, negdesc;
  // Whether posdesc is printed for total==0
  bool suppress_if_zero;
  // An auxiliary value that is also accumulated, but not tested
  double aux;
  // ...and the format string for the net aux value (appended to *desc)
  QString auxfmt;
};

struct city_sum {
  QString format;
  size_t n;
  QVector<msum> sums;
};

/**
   Create a new city_sum.
   'format' is how to print each contribution, of the form "%+4.0f : %s"
   (the first item is the numeric value and must have a 'f' conversion
   spec; the second is the description).
 */
static struct city_sum *city_sum_new(const char *format)
{
  struct city_sum *sum = new city_sum();

  sum->format = format;
  sum->n = 0;
  sum->sums.clear();

  return sum;
}

/**
   Helper: add a new contribution to the city_sum.
   If 'posdesc'/'negdesc' and other properties match an existing entry,
   'value' is added to the existing entry, else a new one is appended.
 */
static void city_sum_add_real(struct city_sum *sum, double value,
                              bool suppress_if_zero, const QString &auxfmt,
                              double aux, const QString &posdesc,
                              const QString &negdesc)
{
  size_t i;

  // likely to lead to quadratic behaviour, but who cares:
  for (i = 0; i < sum->n; i++) {
    if (sum->sums[i].posdesc == posdesc && sum->sums[i].negdesc == negdesc
        && sum->sums[i].auxfmt == auxfmt
        && sum->sums[i].suppress_if_zero == suppress_if_zero) {
      // Looks like we already have an entry like this. Accumulate values.
      sum->sums[i].value += value;
      sum->sums[i].aux += aux;

      return;
    }
  }

  // Didn't find description already, so add it to the end.
  sum->sums.resize(sum->n + 1);
  sum->sums[sum->n].value = value;
  sum->sums[sum->n].posdesc = posdesc;
  sum->sums[sum->n].negdesc = negdesc;
  sum->sums[sum->n].suppress_if_zero = suppress_if_zero;
  sum->sums[sum->n].aux = aux;
  sum->sums[sum->n].auxfmt = auxfmt;
  sum->n++;
}

/**
   Add a new contribution to the city_sum (complex).
    - Allows different descriptions for net positive and negative
      contributions (posfmt/negfmt);
    - Allows specifying another auxiliary number which isn't significant
      for comparisons but will also be accumulated, and appended to the
      description when rendered (e.g. the 50% in "+11: Bonus from
      Marketplace+Luxury (+50%)")
    - Allows control over whether the item will be discarded if its
      net value is zero (suppress_if_zero).
 */
static void fc__attribute((__format__(__printf__, 6, 8)))
    fc__attribute((__format__(__printf__, 7, 8)))
        fc__attribute((nonnull(1, 6, 7)))
            city_sum_add_full(struct city_sum *sum, double value,
                              bool suppress_if_zero, const char *auxfmt,
                              double aux, const char *posfmt,
                              const char *negfmt, ...)
{
  va_list args;

  // Format both descriptions
  va_start(args, negfmt); // sic -- arguments follow negfmt
  auto posdesc = QString::vasprintf(posfmt, args);
  va_end(args);

  va_start(args, negfmt); // sic -- arguments follow negfmt
  auto negdesc = QString::vasprintf(negfmt, args);
  va_end(args);

  city_sum_add_real(sum, value, suppress_if_zero, auxfmt, aux, posdesc,
                    negdesc);
}

/**
   Add a new contribution to the city_sum (simple).
   Compared to city_sum_add_full():
    - description does not depend on net value
    - not suppressed if net value is zero (compare city_sum_add_nonzero())
    - no auxiliary number
 */
static void fc__attribute((__format__(__printf__, 3, 4)))
    fc__attribute((nonnull(1, 3)))
        city_sum_add(struct city_sum *sum, double value, const char *descfmt,
                     ...)
{
  va_list args;

  // Format description (same used for positive or negative net value)
  va_start(args, descfmt);
  auto desc = QString::vasprintf(descfmt, args);
  va_end(args);

  // Descriptions will be freed individually, so need to strdup
  city_sum_add_real(sum, value, false, nullptr, 0, desc, desc);
}

/**
   Add a new contribution to the city_sum (simple).
   Compared to city_sum_add_full():
    - description does not depend on net value
    - suppressed if net value is zero (compare city_sum_add())
    - no auxiliary number
 */
static void fc__attribute((__format__(__printf__, 3, 4)))
    fc__attribute((nonnull(1, 3)))
        city_sum_add_if_nonzero(struct city_sum *sum, double value,
                                const char *descfmt, ...)
{
  va_list args;

  // Format description (same used for positive or negative net value)
  va_start(args, descfmt);
  auto desc = QString::vasprintf(descfmt, args);
  va_end(args);

  // Descriptions will be freed individually, so need to strdup
  city_sum_add_real(sum, value, true, nullptr, 0, desc, desc);
}

/**
   Return the net total accumulated in the sum so far.
 */
static double city_sum_total(struct city_sum *sum)
{
  size_t i;
  double total = 0;

  for (i = 0; i < sum->n; i++) {
    total += sum->sums[i].value;
  }
  return total;
}

/**
   Compare two values, taking care of floating point comparison issues.
     -1: val1 <  val2
      0: val1 == val2 (approximately)
     +1: val1 >  val2
 */
static inline int city_sum_compare(double val1, double val2)
{
  /* Fudgey epsilon -- probably the numbers we're dealing with have at
   * most 1% or 0.1% real difference */
  if (fabs(val1 - val2) < 0.0000001) {
    return 0;
  }
  return (val1 > val2 ? +1 : -1);
}

/**
   Print out the sum, including total, and free the city_sum.
   totalfmt's first format string must be some kind of %f, and first
   argument must be a double (if account_for_unknown).
   account_for_unknown is optional, as not every sum wants it (consider
   pollution's clipping).
 */
static QString fc__attribute((__format__(__printf__, 3, 4)))
    fc__attribute((nonnull(1, 3)))
        city_sum_print(struct city_sum *sum, bool account_for_unknown,
                       const char *totalfmt, ...)
{
  va_list args;
  size_t i;

  /* This probably ought not to happen in well-designed rulesets, but it's
   * possible for incomplete client knowledge to give an inaccurate
   * breakdown. If it does happen, at least acknowledge to the user that
   * we are confused, rather than displaying an incorrect sum. */
  if (account_for_unknown) {
    double total = city_sum_total(sum);
    double actual_total;

    va_start(args, totalfmt);
    actual_total = va_arg(args, double);
    va_end(args);

    if (city_sum_compare(total, actual_total) != 0) {
      city_sum_add(sum, actual_total - total,
                   /* TRANS: Client cannot explain some aspect of city
                    * output. Should never happen. */
                   Q_("?city_sum:(unknown)"));
    }
  }

  QString result;
  for (i = 0; i < sum->n; i++) {
    if (!sum->sums[i].suppress_if_zero
        || city_sum_compare(sum->sums[i].value, 0) != 0) {
      result += QString::asprintf(
          qUtf8Printable(sum->format), sum->sums[i].value,
          (sum->sums[i].value < 0) ? qUtf8Printable(sum->sums[i].negdesc)
                                   : qUtf8Printable(sum->sums[i].posdesc));
      if (!sum->sums[i].auxfmt.isEmpty()) {
        result += QString::asprintf(qUtf8Printable(sum->sums[i].auxfmt),
                                    sum->sums[i].aux);
      }
      result += QStringLiteral("\n");
    }
  }

  va_start(args, totalfmt);
  result += QString::asprintf(totalfmt, args);
  va_end(args);

  delete sum;
  return result;
}

/**
   Return text describing the production output.
 */
QString get_city_dialog_output_text(const struct city *pcity,
                                    Output_type_id otype)
{
  int priority;
  int tax[O_LAST];
  struct output_type *output = &output_types[otype];
  /* TRANS: format string for a row of the city output sum that adds up
   * to "Total surplus" */
  struct city_sum *sum = city_sum_new(Q_("?city_surplus:%+4.0f : %s"));

  city_sum_add(sum, pcity->citizen_base[otype],
               Q_("?city_surplus:Citizens"));

  // Hack to get around the ugliness of add_tax_income.
  memset(tax, 0, O_LAST * sizeof(*tax));
  add_tax_income(city_owner(pcity), pcity->prod[O_TRADE], tax);
  city_sum_add_if_nonzero(sum, tax[otype],
                          Q_("?city_surplus:Taxed from trade"));

  /* Special cases for "bonus" production.  See set_city_production in
   * city.c. */
  if (otype == O_TRADE) {
    trade_routes_iterate(pcity, proute)
    {
      /* NB: (proute->value == 0) is valid case.  The trade route
       * is established but doesn't give trade surplus. */
      struct city *trade_city = game_city_by_number(proute->partner);
      // TRANS: Trade partner unknown to client
      const char *name =
          trade_city ? city_name_get(trade_city) : _("(unknown)");
      int value = proute->value
                  * (100 + get_city_bonus(pcity, EFT_TRADEROUTE_PCT)) / 100;

      switch (proute->dir) {
      case RDIR_BIDIRECTIONAL:
        city_sum_add(sum, value, Q_("?city_surplus:Trading %s with %s"),
                     goods_name_translation(proute->goods), name);
        break;
      case RDIR_FROM:
        city_sum_add(sum, value, Q_("?city_surplus:Trading %s to %s"),
                     goods_name_translation(proute->goods), name);
        break;
      case RDIR_TO:
        city_sum_add(sum, value, Q_("?city_surplus:Trading %s from %s"),
                     goods_name_translation(proute->goods), name);
        break;
      }
    }
    trade_routes_iterate_end;
  } else if (otype == O_GOLD) {
    int tithes = get_city_tithes_bonus(pcity);

    city_sum_add_if_nonzero(sum, tithes,
                            Q_("?city_surplus:Building tithes"));
  }

  for (priority = 0; priority < 2; priority++) {
    enum effect_type eft[] = {EFT_OUTPUT_BONUS, EFT_OUTPUT_BONUS_2};

    {
      int base = city_sum_total(sum), bonus = 100;
      struct effect_list *plist = effect_list_new();

      (void) get_city_bonus_effects(plist, pcity, output, eft[priority]);

      effect_list_iterate(plist, peffect)
      {
        char buf2[512];
        int delta;
        int new_total;

        get_effect_req_text(peffect, buf2, sizeof(buf2));

        if (peffect->multiplier) {
          int mul = player_multiplier_effect_value(city_owner(pcity),
                                                   peffect->multiplier);

          if (mul == 0) {
            /* Suppress text when multiplier setting suppresses effect
             * (this will also suppress it when the city owner's policy
             * settings are not known to us) */
            continue;
          }
          delta = (peffect->value * mul) / 100;
        } else {
          delta = peffect->value;
        }
        bonus += delta;
        new_total = bonus * base / 100;
        city_sum_add_full(sum, new_total - city_sum_total(sum), true,
                          /* TRANS: percentage city output bonus/loss from
                           * some source; preserve leading space */
                          Q_("?city_surplus: (%+.0f%%)"), delta,
                          Q_("?city_surplus:Bonus from %s"),
                          Q_("?city_surplus:Loss from %s"), buf2);
      }
      effect_list_iterate_end;
      effect_list_destroy(plist);
    }
  }

  if (pcity->waste[otype] != 0) {
    int wastetypes[OLOSS_LAST];
    bool breakdown_ok;
    int regular_waste;
    /* FIXME: this will give the wrong answer in rulesets with waste on
     * taxed outputs, such as 'science waste', as our total so far includes
     * contributions taxed from trade, whereas the equivalent bit in
     * set_city_production() does not */
    if (city_waste(pcity, otype, city_sum_total(sum), wastetypes)
        == pcity->waste[otype]) {
      // Our calculation matches the server's, so we trust our breakdown.
      city_sum_add_if_nonzero(sum, -wastetypes[OLOSS_SIZE],
                              Q_("?city_surplus:Size penalty"));
      regular_waste = wastetypes[OLOSS_WASTE];
      breakdown_ok = true;
    } else {
      /* Our calculation doesn't match what the server sent. Account it all
       * to corruption/waste. */
      regular_waste = pcity->waste[otype];
      breakdown_ok = false;
    }
    if (regular_waste > 0) {
      const char *fmt;
      switch (otype) {
      case O_SHIELD:
      default: // FIXME other output types?
        /* TRANS: %s is normally empty, but becomes '?' if client is
         * uncertain about its accounting (should never happen) */
        fmt = Q_("?city_surplus:Waste%s");
        break;
      case O_TRADE:
        /* TRANS: %s is normally empty, but becomes '?' if client is
         * uncertain about its accounting (should never happen) */
        fmt = Q_("?city_surplus:Corruption%s");
        break;
      }
      city_sum_add(sum, -regular_waste, fmt, breakdown_ok ? "" : "?");
    }
  }

  city_sum_add_if_nonzero(sum, -pcity->unhappy_penalty[otype],
                          Q_("?city_surplus:Disorder"));

  if (pcity->usage[otype] > 0) {
    city_sum_add(sum, -pcity->usage[otype], Q_("?city_surplus:Used"));
  }

  return city_sum_print(sum, true,
                        Q_("?city_surplus:"
                           "==== : Adds up to\n"
                           "%4.0f : Total surplus"),
                        static_cast<double>(pcity->surplus[otype]));
}

/**
   Return text describing the chance for a plague.
 */
QString get_city_dialog_illness_text(const struct city *pcity)
{
  int illness, ill_base, ill_size, ill_trade, ill_pollution;
  struct effect_list *plist;
  struct city_sum *sum;

  if (!game.info.illness_on) {
    return _("Illness deactivated in ruleset.");
  }

  sum = city_sum_new(Q_("?city_plague:%+5.1f%% : %s"));

  illness = city_illness_calc(pcity, &ill_base, &ill_size, &ill_trade,
                              &ill_pollution);

  city_sum_add(sum, static_cast<float>(ill_size) / 10.0,
               Q_("?city_plague:Risk from overcrowding"));
  city_sum_add(sum, static_cast<float>(ill_trade) / 10.0,
               Q_("?city_plague:Risk from trade"));
  city_sum_add(sum, static_cast<float>(ill_pollution) / 10.0,
               Q_("?city_plague:Risk from pollution"));

  plist = effect_list_new();

  (void) get_city_bonus_effects(plist, pcity, nullptr, EFT_HEALTH_PCT);

  effect_list_iterate(plist, peffect)
  {
    char buf2[512];
    int delta;

    get_effect_req_text(peffect, buf2, sizeof(buf2));

    if (peffect->multiplier) {
      int mul = player_multiplier_effect_value(city_owner(pcity),
                                               peffect->multiplier);

      if (mul == 0) {
        /* Suppress text when multiplier setting suppresses effect
         * (this will also suppress it when the city owner's policy
         * settings are not known to us) */
        continue;
      }
      delta = (peffect->value * mul) / 100;
    } else {
      delta = peffect->value;
    }

    city_sum_add_full(sum, -(0.1 * ill_base * delta / 100), true,
                      Q_("?city_plague: (%+.0f%%)"), -delta,
                      Q_("?city_plague:Risk from %s"),
                      Q_("?city_plague:Bonus from %s"), buf2);
  }
  effect_list_iterate_end;
  effect_list_destroy(plist);

  /* XXX: account_for_unknown==FALSE: the displayed sum can fail to
   * add up due to rounding. Making it always add up probably requires
   * arbitrary assignment of 0.1% rounding figures to particular
   * effects with something like distribute(). */
  return city_sum_print(sum, false,
                        Q_("?city_plague:"
                           "====== : Adds up to\n"
                           "%5.1f%% : Plague chance per turn"),
                        (static_cast<double>(illness) / 10.0));
}

/**
   Return text describing the pollution output.
 */
QString get_city_dialog_pollution_text(const struct city *pcity)
{
  int pollu, prod, pop, mod;
  struct city_sum *sum = city_sum_new(Q_("?city_pollution:%+4.0f : %s"));

  /* On the server, pollution is calculated before production is deducted
   * for disorder; we need to compensate for that */
  pollu = city_pollution_types(
      pcity, pcity->prod[O_SHIELD] + pcity->unhappy_penalty[O_SHIELD], &prod,
      &pop, &mod);

  city_sum_add(sum, prod, Q_("?city_pollution:Pollution from shields"));
  city_sum_add(sum, pop, Q_("?city_pollution:Pollution from citizens"));
  city_sum_add(sum, mod, Q_("?city_pollution:Pollution modifier"));
  return city_sum_print(sum, false,
                        Q_("?city_pollution:"
                           "==== : Adds up to\n"
                           "%4.0f : Total surplus"),
                        static_cast<double>(pollu));
}

/**
   Return text describing the culture output.
 */
QString get_city_dialog_culture_text(const struct city *pcity)
{
  struct effect_list *plist;
  struct city_sum *sum = city_sum_new(Q_("?city_culture:%4.0f : %s"));

  /* XXX: no way to check whether client's idea of gain/turn is accurate */
  city_sum_add(sum, pcity->history, Q_("?city_culture:History (%+d/turn)"),
               city_history_gain(pcity));

  plist = effect_list_new();

  (void) get_city_bonus_effects(plist, pcity, nullptr, EFT_PERFORMANCE);

  effect_list_iterate(plist, peffect)
  {
    char buf2[512];
    int mul = 100;
    int value;

    get_effect_req_text(peffect, buf2, sizeof(buf2));

    if (peffect->multiplier) {
      mul = player_multiplier_effect_value(city_owner(pcity),
                                           peffect->multiplier);

      if (mul == 0) {
        /* Suppress text when multiplier setting suppresses effect
         * (this will also suppress it when the city owner's policy
         * settings are not known to us) */
        continue;
      }
    }

    value = (peffect->value * mul) / 100;
    // TRANS: text describing source of culture bonus ("Library+Republic")
    city_sum_add_if_nonzero(sum, value, Q_("?city_culture:%s"), buf2);
  }
  effect_list_iterate_end;
  effect_list_destroy(plist);

  return city_sum_print(sum, true,
                        Q_("?city_culture:"
                           "==== : Adds up to\n"
                           "%4.0f : Total culture"),
                        static_cast<double>(pcity->client.culture));
}

/**
   Return text describing airlift capacity.
 */
QString get_city_dialog_airlift_text(const struct city *pcity)
{
  char src[512];
  char dest[512];
  int unlimited = 0;

  if (game.info.airlifting_style & AIRLIFTING_UNLIMITED_SRC
      && pcity->airlift >= 1) {
    /* AIRLIFTING_UNLIMITED_SRC applies only when the source city has
     * remaining airlift. */

    unlimited++;

    /* TRANS: airlift. Possible take offs text. String is a
     * proviso that take offs can't occur if landings spend all the
     * remaining airlift when landings are limited and empty when they
     * aren't limited. */
    fc_snprintf(src, sizeof(src), _("unlimited take offs%s"),
                game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST
                    /* TRANS: airlift unlimited take offs proviso used above.
                     * Plural based on remaining airlift capacity. */
                    ? ""
                    : PL_(" (until the landing has been spent)",
                          " (until all landings have been spent)",
                          pcity->airlift));
  } else {
    fc_snprintf(src, sizeof(src),
                /* TRANS: airlift. Possible take offs text. Number is
                 * airlift capacity. */
                PL_("%d take off", "%d take offs", pcity->airlift),
                pcity->airlift);
  }

  if (game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST) {
    /* AIRLIFTING_UNLIMITED_DEST works even if the source city has no
     * remaining airlift. */

    unlimited++;

    // TRANS: airlift. Possible landings text.
    fc_snprintf(dest, sizeof(dest), _("unlimited landings"));
  } else {
    fc_snprintf(dest, sizeof(dest),
                /* TRANS: airlift. Possible landings text.
                 * Number is airlift capacity. */
                PL_("%d landing", "%d landings", pcity->airlift),
                pcity->airlift);
  }

  switch (unlimited) {
  case 2:
    // TRANS: airlift take offs and landings
    return _("unlimited take offs and landings");
    break;
  case 1:
    /* TRANS: airlift take offs and landings. One is unlimited. The first
     * string is the take offs text. The 2nd string is the landings text. */
    return QString::asprintf(_("%s and %s"), src, dest);
    break;
  default:
    return QString::asprintf(
        /* TRANS: airlift take offs or landings, no unlimited.
         * Number is airlift capacity. */
        PL_("%d take off or landing", "%d take offs or landings",
            pcity->airlift),
        pcity->airlift);
    break;
  }
}

/**
   Return airlift capacity.
 */
QString get_city_dialog_airlift_value(const struct city *pcity)
{
  char src[512];
  char dest[512];
  int unlimited = 0;

  if (game.info.airlifting_style & AIRLIFTING_UNLIMITED_SRC
      && pcity->airlift >= 1) {
    /* AIRLIFTING_UNLIMITED_SRC applies only when the source city has
     * remaining airlift. */

    unlimited++;

    /* TRANS: airlift. Possible take offs text. String is a symbol that
     * indicates that terms and conditions apply when landings are limited
     * and empty when they aren't limited. */
    fc_snprintf(src, sizeof(src), _("∞%s"),
                game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST
                    /* TRANS: airlift unlimited take offs may be spent symbol
                     * used above. */
                    ? ""
                    : _("*"));
  } else {
    /* TRANS: airlift. Possible take offs text. Number is
     * airlift capacity. */
    fc_snprintf(src, sizeof(src), _("%d"), pcity->airlift);
  }

  if (game.info.airlifting_style & AIRLIFTING_UNLIMITED_DEST) {
    /* AIRLIFTING_UNLIMITED_DEST works even if the source city has no
     * remaining airlift. */

    unlimited++;

    // TRANS: airlift. Possible landings text.
    fc_snprintf(dest, sizeof(dest), _("∞"));
  } else {
    // TRANS: airlift. Possible landings text.
    fc_snprintf(dest, sizeof(dest), _("%d"), pcity->airlift);
  }

  switch (unlimited) {
  case 2:
    // TRANS: unlimited airlift take offs and landings
    return _("∞");
    break;
  case 1:
    /* TRANS: airlift take offs and landings. One is unlimited. The first
     * string is the take offs text. The 2nd string is the landings text. */
    return QString::asprintf(_("s: %s d: %s"), src, dest);
    break;
  default:
    // TRANS: airlift take offs or landings, no unlimited
    return QString::asprintf(_("%s"), src);
    break;
  }
}

/**
   Provide a list of all citizens in the city, in order.  "index"
   should be the happiness index (currently [0..4]; 4 = final
   happiness).  "citizens" should be an array large enough to hold all
   citizens (use MAX_CITY_SIZE to be on the safe side).
 */
int get_city_citizen_types(struct city *pcity, enum citizen_feeling idx,
                           enum citizen_category *categories)
{
  int i = 0, n;

  fc_assert(idx >= 0 && idx < FEELING_LAST);

  for (n = 0; n < pcity->feel[CITIZEN_HAPPY][idx]; n++, i++) {
    categories[i] = CITIZEN_HAPPY;
  }
  for (n = 0; n < pcity->feel[CITIZEN_CONTENT][idx]; n++, i++) {
    categories[i] = CITIZEN_CONTENT;
  }
  for (n = 0; n < pcity->feel[CITIZEN_UNHAPPY][idx]; n++, i++) {
    categories[i] = CITIZEN_UNHAPPY;
  }
  for (n = 0; n < pcity->feel[CITIZEN_ANGRY][idx]; n++, i++) {
    categories[i] = CITIZEN_ANGRY;
  }

  specialist_type_iterate(sp)
  {
    for (n = 0; n < pcity->specialists[sp]; n++, i++) {
      categories[i] = static_cast<citizen_category>(
          static_cast<int>(CITIZEN_SPECIALIST) + static_cast<int>(sp));
    }
  }
  specialist_type_iterate_end;

  if (city_size_get(pcity) != i) {
    qCritical("get_city_citizen_types() %d citizens "
              "not equal %d city size in \"%s\".",
              i, city_size_get(pcity), city_name_get(pcity));
  }
  return i;
}

/**
   Rotate the given specialist citizen to the next type of citizen.
 */
void city_rotate_specialist(struct city *pcity, int citizen_index)
{
  enum citizen_category categories[MAX_CITY_SIZE];
  Specialist_type_id from, to;
  int num_citizens =
      get_city_citizen_types(pcity, FEELING_FINAL, categories);

  if (citizen_index < 0 || citizen_index >= num_citizens
      || categories[citizen_index] < CITIZEN_SPECIALIST) {
    return;
  }
  from = categories[citizen_index] - CITIZEN_SPECIALIST;

  /* Loop through all specialists in order until we find a usable one
   * (or run out of choices). */
  to = from;
  fc_assert(to >= 0 && to < specialist_count());
  do {
    to = (to + 1) % specialist_count();
  } while (to != from && !city_can_use_specialist(pcity, to));

  if (from != to) {
    city_change_specialist(pcity, from, to);
  }
}

/**
   Change the production of a given city.  Return the request ID.
 */
int city_change_production(struct city *pcity, struct universal *target)
{
  return dsend_packet_city_change(&client.conn, pcity->id, target->kind,
                                  universal_number(target));
}

/**
   Set the worklist for a given city.  Return the request ID.

   Note that the worklist does NOT include the current production.
 */
int city_set_worklist(struct city *pcity, const struct worklist *pworklist)
{
  return dsend_packet_city_worklist(&client.conn, pcity->id, pworklist);
}

/**
   Insert an item into the city's queue.  This function will send new
   production requests to the server but will NOT send the new worklist
   to the server - the caller should call city_set_worklist() if the
   function returns TRUE.

   Note that the queue DOES include the current production.
 */
static bool base_city_queue_insert(struct city *pcity, int position,
                                   struct universal *item)
{
  if (position == 0) {
    struct universal old = pcity->production;

    // Insert as current production.
    if (!can_city_build_direct(pcity, item)) {
      return false;
    }

    if (!worklist_insert(&pcity->worklist, &old, 0)) {
      return false;
    }

    city_change_production(pcity, item);
  } else if (position >= 1
             && position <= worklist_length(&pcity->worklist)) {
    // Insert into middle.
    if (!can_city_build_later(pcity, item)) {
      return false;
    }
    if (!worklist_insert(&pcity->worklist, item, position - 1)) {
      return false;
    }
  } else {
    // Insert at end.
    if (!can_city_build_later(pcity, item)) {
      return false;
    }
    if (!worklist_append(&pcity->worklist, item)) {
      return false;
    }
  }
  return true;
}

/**
   Insert an item into the city's queue.

   Note that the queue DOES include the current production.
 */
bool city_queue_insert(struct city *pcity, int position,
                       struct universal *item)
{
  if (base_city_queue_insert(pcity, position, item)) {
    city_set_worklist(pcity, &pcity->worklist);
    return true;
  }
  return false;
}

/**
   Insert the worklist into the city's queue at the given position.

   Note that the queue DOES include the current production.
 */
bool city_queue_insert_worklist(struct city *pcity, int position,
                                const struct worklist *worklist)
{
  bool success = false;

  if (worklist_length(worklist) == 0) {
    return true;
  }

  worklist_iterate(worklist, target)
  {
    if (base_city_queue_insert(pcity, position, &target)) {
      if (position > 0) {
        /* Move to the next position (unless position == -1 in which case
         * we're appending. */
        position++;
      }
      success = true;
    }
  }
  worklist_iterate_end;

  if (success) {
    city_set_worklist(pcity, &pcity->worklist);
  }

  return success;
}

/**
   Get the city current production and the worklist, like it should be.
 */
void city_get_queue(struct city *pcity, struct worklist *pqueue)
{
  worklist_copy(pqueue, &pcity->worklist);

  /* The GUI wants current production to be in the task list, but the
     worklist API wants it out for reasons unknown. Perhaps someone enjoyed
     making things more complicated than necessary? So I dance around it. */

  // We want the current production to be in the queue. Always.
  worklist_remove(pqueue, MAX_LEN_WORKLIST - 1);

  worklist_insert(pqueue, &pcity->production, 0);
}

/**
   Set the city current production and the worklist, like it should be.
 */
bool city_set_queue(struct city *pcity, const struct worklist *pqueue)
{
  struct worklist copy;
  struct universal target;

  worklist_copy(&copy, pqueue);

  /* The GUI wants current production to be in the task list, but the
     worklist API wants it out for reasons unknown. Perhaps someone enjoyed
     making things more complicated than necessary? So I dance around it. */
  if (worklist_peek(&copy, &target)) {
    if (!city_can_change_build(pcity)
        && !are_universals_equal(&pcity->production, &target)) {
      /* We cannot change production to one from worklist.
       * Do not replace old worklist with new one. */
      return false;
    }

    worklist_advance(&copy);

    city_set_worklist(pcity, &copy);
    city_change_production(pcity, &target);
  } else {
    // You naughty boy, you can't erase the current production. Nyah!
    if (worklist_is_empty(&pcity->worklist)) {
      refresh_city_dialog(pcity);
    } else {
      city_set_worklist(pcity, &copy);
    }
  }

  return true;
}

/**
   Return TRUE iff the city can buy.
 */
bool city_can_buy(const struct city *pcity)
{
  /* See really_handle_city_buy() in the server.  However this function
   * doesn't allow for error messages.  It doesn't check the cost of
   * buying; that's handled separately (and with an error message). */
  return (can_client_issue_orders() && nullptr != pcity
          && city_owner(pcity) == client.conn.playing
          && pcity->turn_founded != game.info.turn && !pcity->did_buy
          && (VUT_UTYPE == pcity->production.kind
              || !improvement_has_flag(pcity->production.value.building,
                                       IF_GOLD))
          && !(VUT_UTYPE == pcity->production.kind && pcity->anarchy != 0)
          && pcity->client.buy_cost > 0);
}

/**
   Change the production of a given city.  Return the request ID.
 */
int city_sell_improvement(struct city *pcity, Impr_type_id sell_id)
{
  return dsend_packet_city_sell(&client.conn, pcity->id, sell_id);
}

/**
   Buy the current production item in a given city.  Return the request ID.
 */
int city_buy_production(struct city *pcity)
{
  return dsend_packet_city_buy(&client.conn, pcity->id);
}

/**
   Change a specialist in the given city.  Return the request ID.
 */
int city_change_specialist(struct city *pcity, Specialist_type_id from,
                           Specialist_type_id to)
{
  return dsend_packet_city_change_specialist(&client.conn, pcity->id, from,
                                             to);
}

/**
   Tell the server to rename the city.  Return the request ID.
 */
int city_rename(struct city *pcity, const char *name)
{
  return dsend_packet_city_rename(&client.conn, pcity->id, name);
}
