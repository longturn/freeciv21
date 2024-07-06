/*
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
 */

// utility
#include "fcintl.h"
#include "log.h"
#include "rand.h"

// common
#include "city.h"
#include "effects.h"
#include "game.h"
#include "map.h"
#include "tile.h"
#include "unittype.h"

#include "traderoutes.h"

const char *trade_route_type_names[] = {
    "National", "NationalIC", "IN",      "INIC", "Ally",
    "AllyIC",   "Enemy",      "EnemyIC", "Team", "TeamIC"};

const char *traderoute_cancelling_type_names[] = {"Active", "Inactive",
                                                  "Cancel"};

struct trade_route_settings trtss[TRT_LAST];

static struct goods_type goods[MAX_GOODS_TYPES];

/**
   Return current maximum number of trade routes city can have.
 */
int max_trade_routes(const struct city *pcity)
{
  int eft = get_city_bonus(pcity, EFT_MAX_TRADE_ROUTES);

  return CLIP(0, eft, MAX_TRADE_ROUTES);
}

/**
   What is type of the traderoute between two cities.
 */
enum trade_route_type cities_trade_route_type(const struct city *pcity1,
                                              const struct city *pcity2)
{
  struct player *plr1 = city_owner(pcity1);
  struct player *plr2 = city_owner(pcity2);

  if (plr1 != plr2) {
    struct player_diplstate *ds = player_diplstate_get(plr1, plr2);

    if (city_tile(pcity1)->continent != city_tile(pcity2)->continent) {
      switch (ds->type) {
      case DS_ALLIANCE:
        return TRT_ALLY_IC;
      case DS_WAR:
        return TRT_ENEMY_IC;
      case DS_TEAM:
        return TRT_TEAM_IC;
      case DS_ARMISTICE:
      case DS_CEASEFIRE:
      case DS_PEACE:
      case DS_NO_CONTACT:
        return TRT_IN_IC;
      case DS_LAST:
        fc_assert(ds->type != DS_LAST);
        return TRT_IN_IC;
      }
      fc_assert(false);

      return TRT_IN_IC;
    } else {
      switch (ds->type) {
      case DS_ALLIANCE:
        return TRT_ALLY;
      case DS_WAR:
        return TRT_ENEMY;
      case DS_TEAM:
        return TRT_TEAM;
      case DS_ARMISTICE:
      case DS_CEASEFIRE:
      case DS_PEACE:
      case DS_NO_CONTACT:
        return TRT_IN;
      case DS_LAST:
        fc_assert(ds->type != DS_LAST);
        return TRT_IN;
      }
      fc_assert(false);

      return TRT_IN;
    }
  } else {
    if (city_tile(pcity1)->continent != city_tile(pcity2)->continent) {
      return TRT_NATIONAL_IC;
    } else {
      return TRT_NATIONAL;
    }
  }

  return TRT_LAST;
}

/**
   Return percentage bonus for trade route type.
 */
int trade_route_type_trade_pct(enum trade_route_type type)
{
  if (type >= TRT_LAST) {
    return 0;
  }

  return trtss[type].trade_pct;
}

/**
   Initialize trade route types.
 */
void trade_route_types_init()
{
  for (int type = TRT_NATIONAL; type < TRT_LAST; type++) {
    struct trade_route_settings *set =
        trade_route_settings_by_type(trade_route_type(type));

    set->trade_pct = 100;
  }
}

/**
   Return human readable name of trade route type
 */
const char *trade_route_type_name(enum trade_route_type type)
{
  fc_assert_ret_val(type >= TRT_NATIONAL && type < TRT_LAST, nullptr);

  return trade_route_type_names[type];
}

/**
   Get trade route type by name.
 */
enum trade_route_type trade_route_type_by_name(const char *name)
{
  for (int type = TRT_NATIONAL; type < TRT_LAST; type++) {
    if (!fc_strcasecmp(trade_route_type_names[type], name)) {
      return trade_route_type(type);
    }
  }

  return TRT_LAST;
}

/**
   Return human readable name of traderoute cancelling type
 */
const char *
traderoute_cancelling_type_name(enum traderoute_illegal_cancelling type)
{
  fc_assert_ret_val(type >= TRI_ACTIVE && type < TRI_LAST, nullptr);

  return traderoute_cancelling_type_names[type];
}

/**
   Get traderoute cancelling type by name.
 */
enum traderoute_illegal_cancelling
traderoute_cancelling_type_by_name(const char *name)
{
  for (int type = TRI_ACTIVE; type < TRI_LAST; type++) {
    if (!fc_strcasecmp(traderoute_cancelling_type_names[type], name)) {
      return traderoute_illegal_cancelling(type);
    }
  }

  return TRI_LAST;
}

/**
   Get trade route settings related to type.
 */
struct trade_route_settings *
trade_route_settings_by_type(enum trade_route_type type)
{
  fc_assert_ret_val(type >= TRT_NATIONAL && type < TRT_LAST, nullptr);

  return &trtss[type];
}

/**
   Return TRUE iff the two cities are capable of trade; i.e., if a caravan
   from one city can enter the other to sell its goods.

   See also can_establish_trade_route().
 */
bool can_cities_trade(const struct city *pc1, const struct city *pc2)
{
  /* If you change the logic here, make sure to update the help in
   * helptext_unit(). */
  return (pc1 && pc2 && pc1 != pc2
          && (city_owner(pc1) != city_owner(pc2)
              || real_map_distance(pc1->tile, pc2->tile)
                     >= game.info.trademindist)
          && (trade_route_type_trade_pct(cities_trade_route_type(pc1, pc2))
              > 0));
}

/**
   Return the minimum value of the sum of trade routes which could be
   replaced by a new one. The target routes to be removed
   will be put into 'would_remove', if set.
 */
int city_trade_removable(const struct city *pcity,
                         struct trade_route_list *would_remove)
{
  struct trade_route *sorted[MAX_TRADE_ROUTES];
  int num, i, j;

  // Sort trade route values.
  num = 0;
  trade_routes_iterate(pcity, proute)
  {
    for (j = num; j > 0 && (proute->value < sorted[j - 1]->value); j--) {
      sorted[j] = sorted[j - 1];
    }
    sorted[j] = proute;
    num++;
  }
  trade_routes_iterate_end;

  // No trade routes at all.
  if (0 == num) {
    return 0;
  }

  // Adjust number of concerned trade routes.
  num += 1 - max_trade_routes(pcity);
  if (0 >= num) {
    num = 1;
  }

  // Return values.
  for (i = j = 0; i < num; i++) {
    j += sorted[i]->value;
    if (nullptr != would_remove) {
      trade_route_list_append(would_remove, sorted[i]);
    }
  }

  return j;
}

/**
   Returns TRUE iff the two cities can establish a trade route.  We look
   at the distance and ownership of the cities as well as their existing
   trade routes.  Should only be called if you already know that
   can_cities_trade().
 */
bool can_establish_trade_route(const struct city *pc1,
                               const struct city *pc2)
{
  int trade = -1;
  int maxpc1;
  int maxpc2;

  if (!pc1 || !pc2 || pc1 == pc2 || !can_cities_trade(pc1, pc2)
      || have_cities_trade_route(pc1, pc2)) {
    return false;
  }

  // First check if cities can have trade routes at all.
  maxpc1 = max_trade_routes(pc1);
  if (maxpc1 <= 0) {
    return false;
  }
  maxpc2 = max_trade_routes(pc2);
  if (maxpc2 <= 0) {
    return false;
  }

  if (city_num_trade_routes(pc1) >= maxpc1) {
    trade = trade_base_between_cities(pc1, pc2);
    // can we replace trade route?
    if (city_trade_removable(pc1, nullptr) >= trade) {
      return false;
    }
  }

  if (city_num_trade_routes(pc2) >= maxpc2) {
    if (trade == -1) {
      trade = trade_base_between_cities(pc1, pc2);
    }
    // can we replace trade route?
    if (city_trade_removable(pc2, nullptr) >= trade) {
      return false;
    }
  }

  return true;
}

/**
   Return the trade that exists between these cities, assuming they have a
   trade route.
 */
int trade_base_between_cities(const struct city *pc1, const struct city *pc2)
{
  int bonus = 0;

  if (nullptr == pc1 || nullptr == pc1->tile || nullptr == pc2
      || nullptr == pc2->tile) {
    return 0;
  }

  if (game.info.trade_revenue_style == TRS_CLASSIC) {
    // Classic Freeciv
    int real_dist = real_map_distance(pc1->tile, pc2->tile);
    int weighted_distance =
        ((100 - game.info.trade_world_rel_pct) * real_dist
         + game.info.trade_world_rel_pct
               * (real_dist * 40 / MAX(wld.map.xsize, wld.map.ysize)))
        / 100;

    bonus = weighted_distance + city_size_get(pc1) + city_size_get(pc2);
  } else if (game.info.trade_revenue_style == TRS_SIMPLE) {
    // Simple revenue style
    bonus =
        (pc1->citizen_base[O_TRADE] + pc2->citizen_base[O_TRADE] + 4) * 3;
  }

  bonus = bonus
          * trade_route_type_trade_pct(cities_trade_route_type(pc1, pc2))
          / 100;

  bonus /= 12;

  return bonus;
}

/**
   Get trade income specific to route's good.
 */
int trade_from_route(const struct city *pc1, const struct trade_route *route,
                     int base)
{
  Q_UNUSED(pc1)
  if (route->dir == RDIR_TO) {
    return base * route->goods->to_pct / 100;
  }

  return base * route->goods->from_pct / 100;
}

/**
   Return number of trade route city has
 */
int city_num_trade_routes(const struct city *pcity)
{
  return trade_route_list_size(pcity->routes);
}

/**
   Returns the maximum trade production of the tiles of the city.
 */
static int max_tile_trade(const struct city *pcity, const player *seen_as)
{
  int i, total = 0;
  int radius_sq = city_map_radius_sq_get(pcity);
  std::vector<int> tile_trade;
  tile_trade.resize(city_map_tiles(radius_sq));
  size_t size = 0;
  bool is_celebrating = base_city_celebrating(pcity);

  if (pcity->tile == nullptr) {
    return 0;
  }

  city_map_iterate(radius_sq, cindex, cx, cy)
  {
    struct tile *ptile = city_map_to_tile(pcity->tile, radius_sq, cx, cy);

    if (ptile == nullptr) {
      continue;
    }

    if (is_city_center_index(cindex)) {
      total += city_tile_output(pcity, ptile, is_celebrating, O_TRADE);
      continue;
    }

    if (!base_city_can_work_tile(seen_as ? seen_as : city_owner(pcity),
                                 pcity, ptile)) {
      continue;
    }

    tile_trade[size++] =
        city_tile_output(pcity, ptile, is_celebrating, O_TRADE);
  }
  city_map_iterate_end;

  std::sort(tile_trade.begin(), tile_trade.end());

  for (i = 0; i < pcity->size && i < size; i++) {
    total += tile_trade[i];
  }

  return total;
}

/**
   Returns the maximum trade production of a city.
 */
static int max_trade_prod(const struct city *pcity, const player *seen_as)
{
  // Trade tile base
  int trade_prod = max_tile_trade(pcity, seen_as);

  // Add trade routes values
  trade_partners_iterate(pcity, partner)
  {
    trade_prod += trade_base_between_cities(pcity, partner);
  }
  trade_partners_iterate_end;

  return trade_prod;
}

/**
   Returns the revenue trade bonus - you get this when establishing a
   trade route and also when you simply sell your trade goods at the
   new city.

   pgood can be nullptr for ignoring good's onetime_pct.

   pc2 can be nullptr for using an imaginary city at distance 10 with
   75% of pc1's trade (for dai_choose_trade_route()).
 */
int get_caravan_enter_city_trade_bonus(const struct city *pc1,
                                       const struct city *pc2,
                                       const player *seen_as,
                                       struct goods_type *pgood,
                                       const bool establish_trade)
{
  int rmd, trade2, max_trade_2;
  int tb = 0, bonus = 0;

  if (pc2) {
    rmd = real_map_distance(pc1->tile, pc2->tile);
    trade2 = pc2->surplus[O_TRADE];
    max_trade_2 = max_trade_prod(pc2, seen_as);
  } else {
    rmd = 10;
    trade2 = pc1->surplus[O_TRADE] * 0.75;
    max_trade_2 = max_trade_prod(pc1, seen_as) * 0.75;
  }

  if (game.info.caravan_bonus_style == CBS_CLASSIC) {
    tb = rmd + 10;
    tb = (tb * (pc1->surplus[O_TRADE] + trade2)) / 24;
  } else if (game.info.caravan_bonus_style == CBS_LOGARITHMIC) {
    // Logarithmic bonus
    tb = pow(log(rmd + 20 + max_trade_prod(pc1, seen_as) + max_trade_2) * 2,
             2);
  } else if (game.info.caravan_bonus_style == CBS_LINEAR) {
    // Linear bonus (like CLASSIC) but using max_trade_prod
    tb = rmd + 10;
    tb = (tb * (max_trade_prod(pc1, seen_as) + max_trade_2)) / 24;
  } else if (game.info.caravan_bonus_style == CBS_DISTANCE) {
    // Purely dependent on distance, ignore city trade
    tb = rmd + 10;
  }

  if (pgood != nullptr) {
    tb = tb * pgood->onetime_pct / 100;
  }

  // Trade_revenue_exponent (in milimes) bends the shape of the curve
  bonus = get_target_bonus_effects(
      nullptr, city_owner(pc1), pc2 ? city_owner(pc2) : nullptr, pc1,
      nullptr, city_tile(pc1), nullptr, nullptr, nullptr, nullptr,
      action_by_number(establish_trade ? ACTION_TRADE_ROUTE
                                       : ACTION_MARKETPLACE),
      EFT_TRADE_REVENUE_EXPONENT);
  tb = ceil(pow(static_cast<float>(tb),
                1.0f + static_cast<float>(bonus) / 1000.0));

  // Trade_revenue_bonus increases revenue by power of 2 in milimes
  bonus = get_target_bonus_effects(
      nullptr, city_owner(pc1), pc2 ? city_owner(pc2) : nullptr, pc1,
      nullptr, city_tile(pc1),
      /* TODO: Should unit requirements be
       * allowed so stuff like moves left and
       * unit type can modify the bonus? */
      nullptr, nullptr, nullptr, nullptr,
      /* Could be used to reduce the one time
       * bonus if no trade route is
       * established. */
      action_by_number(establish_trade ? ACTION_TRADE_ROUTE
                                       : ACTION_MARKETPLACE),
      EFT_TRADE_REVENUE_BONUS);

  // Be mercy full to players with small amounts. Round up.
  tb = ceil(static_cast<float>(tb)
            * pow(2.0, static_cast<double>(bonus) / 1000.0));

  return tb;
}

/**
   Check if cities have an established trade route.
 */
bool have_cities_trade_route(const struct city *pc1, const struct city *pc2)
{
  trade_partners_iterate(pc1, route_to)
  {
    if (route_to->id == pc2->id) {
      return true;
    }
  }
  trade_partners_iterate_end;

  return false;
}

/**
   Initialize goods structures.
 */
void goods_init()
{
  int i;

  for (i = 0; i < MAX_GOODS_TYPES; i++) {
    goods[i].id = i;

    requirement_vector_init(&(goods[i].reqs));
    goods[i].ruledit_disabled = false;
    goods[i].helptext = nullptr;
  }
}

/**
   Free the memory associated with goods
 */
void goods_free()
{
  int i;

  for (i = 0; i < MAX_GOODS_TYPES; i++) {
    requirement_vector_free(&(goods[i].reqs));
    delete[] goods[i].helptext;
    goods[i].helptext = nullptr;
  }
}

/**
   Return the goods id.
 */
Goods_type_id goods_number(const struct goods_type *pgood)
{
  fc_assert_ret_val(nullptr != pgood, 0);

  return pgood->id;
}

/**
   Return the goods index.

   Currently same as goods_number(), paired with goods_count()
   indicates use as an array index.
 */
Goods_type_id goods_index(const struct goods_type *pgood)
{
  fc_assert_ret_val(nullptr != pgood, 0);

  return pgood - goods;
}

/**
   Return goods type of given id.
 */
struct goods_type *goods_by_number(Goods_type_id id)
{
  fc_assert_ret_val(id >= 0 && id < game.control.num_goods_types, nullptr);

  return &goods[id];
}

/**
   Return translated name of this goods type.
 */
const char *goods_name_translation(struct goods_type *pgood)
{
  return name_translation_get(&pgood->name);
}

/**
   Return untranslated name of this goods type.
 */
const char *goods_rule_name(struct goods_type *pgood)
{
  return rule_name_get(&pgood->name);
}

/**
   Returns goods type matching rule name or nullptr if there is no goods type
   with such name.
 */
struct goods_type *goods_by_rule_name(const char *name)
{
  const char *qs;

  if (name == nullptr) {
    return nullptr;
  }

  qs = Qn_(name);

  goods_type_iterate(pgood)
  {
    if (!fc_strcasecmp(goods_rule_name(pgood), qs)) {
      return pgood;
    }
  }
  goods_type_iterate_end;

  return nullptr;
}

/*
   Returns goods type matching the translated name, or nullptr if there is no
   goods type with that name.
 */
struct goods_type *goods_by_translated_name(const char *name)
{
  goods_type_iterate(pgood)
  {
    if (0 == strcmp(goods_name_translation(pgood), name)) {
      return pgood;
    }
  }
  goods_type_iterate_end;

  return nullptr;
}

/**
   Check if goods has given flag
 */
bool goods_has_flag(const struct goods_type *pgood, enum goods_flag_id flag)
{
  return BV_ISSET(pgood->flags, flag);
}

/**
   Can the city provide goods.
 */
bool goods_can_be_provided(struct city *pcity, struct goods_type *pgood,
                           struct unit *punit)
{
  const struct unit_type *ptype;

  if (punit != nullptr) {
    ptype = unit_type_get(punit);
  } else {
    ptype = nullptr;
  }

  return are_reqs_active(city_owner(pcity), nullptr, pcity, nullptr,
                         city_tile(pcity), punit, ptype, nullptr, nullptr,
                         nullptr, &pgood->reqs, RPT_CERTAIN);
}

/**
   Does city receive goods
 */
bool city_receives_goods(const struct city *pcity,
                         const struct goods_type *pgood)
{
  trade_routes_iterate(pcity, proute)
  {
    if (proute->goods == pgood
        && (proute->dir == RDIR_TO || proute->dir == RDIR_BIDIRECTIONAL)) {
      return true;
    }
  }
  trade_routes_iterate_end;

  return false;
}

/**
   Return goods type for the new traderoute between given cities.
 */
struct goods_type *goods_from_city_to_unit(struct city *src,
                                           struct unit *punit)
{
  int i = 0;
  struct goods_type *potential[MAX_GOODS_TYPES];

  goods_type_iterate(pgood)
  {
    if (goods_can_be_provided(src, pgood, punit)) {
      potential[i++] = pgood;
    }
  }
  goods_type_iterate_end;

  if (i == 0) {
    return nullptr;
  }

  return potential[fc_rand(i)];
}
