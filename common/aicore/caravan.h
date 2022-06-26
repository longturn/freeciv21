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

enum foreign_trade_limit {
  FTL_NATIONAL_ONLY,
  FTL_ALLIED,
  FTL_PEACEFUL,
  FTL_NONWAR
};

/**
 * An advisor for using caravans optimally.
 * The parameter gives what we're optimizing for; use init_default if you
 * don't have anything better to set it to.
 * The optimization does not take into account other caravans in transit.
 * It also knows nothing about moving caravans except what pathfinding will
 * tell it -- ferries, for instance, aren't handled here. Set
 * ignore_transit_time to work around this.
 */

/**
 * The result of one trade route, accoring to the parameter below.
 * A null destination implies that this is not a real result.
 */
struct caravan_result {
  const struct city *src;
  const struct city *dest;
  int arrival_time;

  double value;
  bool help_wonder;
};

struct caravan_parameter {
  /*
   * How many turns to consider when optimizing.
   */
  int horizon;

  /*
   * Discount factor (that is, 1 / interest rate).
   * The worth of a trade route is equivalent to:
   * sum_{i=0}^{horizon} value in i turns * discount^i
   * The discount should be in (0..1].
   * A discount of 1 means future earnings are worth the same as immediate
   * earnings.  A discount of .95 means a dollar next turn is worth only 95
   * cents today; or, in other words, 95 cents in the bank today would yield
   * a dollar next turn.
   */
  double discount;

  /*
   * What to consider:
   * - the immediate windfall when the caravan reaches destination
   * - the trade
   * - the reduction in cost of helping build a wonder
   */
  bool consider_windfall;
  bool consider_trade;
  bool consider_wonders;

  /*
   * A new trade route may break old routes.
   * Account for the loss of old routes.
   */
  bool account_for_broken_routes;

  /*
   * Allow trading with allies and peaceful neighbours.
   * BUG: currently we only consider allies.
   */
  enum foreign_trade_limit allow_foreign_trade;

  /*
   * Normally, we'd want to compute the time it takes to establish the
   * trade route.
   * There are two reasons to ignore the transit time:
   * (1) it may be infinite (i.e. requires crossing an ocean)
   * (2) it's slow to compute it.
   */
  bool ignore_transit_time;

  /*
   * The 'effect of trade' can be computed exactly by an expensive
   * calculation which will take into account buildings, tax rates, etc; or
   * by a cheap calculation that just says that each trade is worth one
   * science or gold.
   */
  bool convert_trade;

  /*
   * This callback, if non-null, is called whenever a trade route
   * is evaluated.  One intended usage is for collecting all the
   * trade routes into a sorted list.
   * Note that the result must be copied to be stored.
   */
  void (*callback)(const struct caravan_result *result, void *data);
  void *callback_data;
};

void caravan_parameter_init_default(struct caravan_parameter *parameter);
void caravan_parameter_init_from_unit(struct caravan_parameter *parameter,
                                      const struct unit *caravan);

void caravan_result_init_zero(struct caravan_result *result);
int caravan_result_compare(const struct caravan_result *a,
                           const struct caravan_result *b);

void caravan_find_best_destination(const struct unit *caravan,
                                   const struct caravan_parameter *parameter,
                                   struct caravan_result *result,
                                   bool omniscient);
