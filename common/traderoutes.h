// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "support.h"

// common
#include "fc_types.h"
#include "name_translation.h" // struct name_translation
#include "requirements.h"     // struct requirement_vector

// Qt
#include <QtContainerFwd> // QVector<QString>

struct city;
struct city_list;
struct player;

enum trade_route_type {
  TRT_NATIONAL = 0,
  TRT_NATIONAL_IC = 1, // Intercontinental
  TRT_IN = 2,
  TRT_IN_IC = 3, // International intercontinental
  TRT_ALLY = 4,
  TRT_ALLY_IC = 5,
  TRT_ENEMY = 6,
  TRT_ENEMY_IC = 7,
  TRT_TEAM = 8,
  TRT_TEAM_IC = 9,
  TRT_LAST = 10
};

struct trade_route_settings {
  int trade_pct;
  enum traderoute_illegal_cancelling cancelling;
  enum traderoute_bonus_type bonus_type;
};

struct goods_type;

struct trade_route {
  int partner;
  int value;
  enum route_direction dir;
  struct goods_type *goods;
};

// get 'struct trade_route_list' and related functions:
#define SPECLIST_TAG trade_route
#define SPECLIST_TYPE struct trade_route
#include "speclist.h"

#define trade_route_list_iterate(trade_route_list, proute)                  \
  TYPED_LIST_ITERATE(struct trade_route, trade_route_list, proute)
#define trade_route_list_iterate_end LIST_ITERATE_END

int max_trade_routes(const struct city *pcity);
enum trade_route_type cities_trade_route_type(const struct city *pcity1,
                                              const struct city *pcity2);
int trade_route_type_trade_pct(enum trade_route_type type);

void trade_route_types_init();
const char *trade_route_type_name(enum trade_route_type type);
enum trade_route_type trade_route_type_by_name(const char *name);

const char *
traderoute_cancelling_type_name(enum traderoute_illegal_cancelling type);
enum traderoute_illegal_cancelling
traderoute_cancelling_type_by_name(const char *name);

struct trade_route_settings *
trade_route_settings_by_type(enum trade_route_type type);

bool can_cities_trade(const struct city *pc1, const struct city *pc2);
bool can_establish_trade_route(const struct city *pc1,
                               const struct city *pc2);
bool have_cities_trade_route(const struct city *pc1, const struct city *pc2);
int trade_base_between_cities(const struct city *pc1,
                              const struct city *pc2);
int trade_from_route(const struct city *pc1, const struct trade_route *route,
                     int base);
int city_num_trade_routes(const struct city *pcity);
int get_caravan_enter_city_trade_bonus(const struct city *pc1,
                                       const struct city *pc2,
                                       const player *seen_as,
                                       struct goods_type *pgood,
                                       const bool establish_trade);
int city_trade_removable(const struct city *pcity,
                         struct trade_route_list *would_remove);

#define trade_routes_iterate(c, proute)                                     \
  do {                                                                      \
    trade_route_list_iterate(c->routes, proute)                             \
    {

#define trade_routes_iterate_end                                            \
  }                                                                         \
  trade_route_list_iterate_end;                                             \
  }                                                                         \
  while (false)

#define trade_routes_iterate_safe(c, proute)                                \
  {                                                                         \
    int _routes##_size = trade_route_list_size(c->routes);                  \
    if (_routes##_size > 0) {                                               \
      struct trade_route *_routes##_saved[_routes##_size];                  \
      int _routes##_index = 0;                                              \
      trade_routes_iterate(c, _proute)                                      \
      {                                                                     \
        _routes##_saved[_routes##_index++] = _proute;                       \
      }                                                                     \
      trade_routes_iterate_end;                                             \
      for (_routes##_index = 0; _routes##_index < _routes##_size;           \
           _routes##_index++) {                                             \
        struct trade_route *proute = _routes##_saved[_routes##_index];

#define trade_routes_iterate_safe_end                                       \
  }                                                                         \
  }                                                                         \
  }

#define trade_partners_iterate(c, p)                                        \
  do {                                                                      \
    trade_routes_iterate(c, _proute_)                                       \
    {                                                                       \
      struct city *p = game_city_by_number(_proute_->partner);              \
      /* Maybe we haven't yet recieved info about this city */              \
      if (!p) {                                                             \
        continue;                                                           \
      }

#define trade_partners_iterate_end                                          \
  }                                                                         \
  trade_routes_iterate_end;                                                 \
  }                                                                         \
  while (false)                                                             \
    ;

struct goods_type {
  int id;
  struct name_translation name;
  bool ruledit_disabled; // Does not really exist - hole in goods array

  struct requirement_vector reqs;

  int from_pct;
  int to_pct;
  int onetime_pct;

  bv_goods_flags flags;

  QVector<QString> *helptext;
};

void goods_init();
void goods_free();

Goods_type_id goods_index(const struct goods_type *pgood);
Goods_type_id goods_number(const struct goods_type *pgood);

struct goods_type *goods_by_number(Goods_type_id id);

const char *goods_name_translation(struct goods_type *pgood);
const char *goods_rule_name(struct goods_type *pgood);
struct goods_type *goods_by_rule_name(const char *name);
struct goods_type *goods_by_translated_name(const char *name);

bool goods_has_flag(const struct goods_type *pgood, enum goods_flag_id flag);

bool goods_can_be_provided(struct city *pcity, struct goods_type *pgood,
                           struct unit *punit);
struct goods_type *goods_from_city_to_unit(struct city *src,
                                           struct unit *punit);
bool city_receives_goods(const struct city *pcity,
                         const struct goods_type *pgood);

#define goods_type_iterate(_p)                                              \
  {                                                                         \
    int _i_;                                                                \
    for (_i_ = 0; _i_ < game.control.num_goods_types; _i_++) {              \
      struct goods_type *_p = goods_by_number(_i_);

#define goods_type_iterate_end                                              \
  }                                                                         \
  }

#define goods_type_re_active_iterate(_p)                                    \
  goods_type_iterate(_p)                                                    \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define goods_type_re_active_iterate_end                                    \
  }                                                                         \
  }                                                                         \
  goods_type_iterate_end;
