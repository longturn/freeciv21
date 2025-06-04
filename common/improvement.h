// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"
#include "name_translation.h" // struct name_translation
#include "requirements.h"     // struct requirement_vector

// Qt
class QString;
#include <QtContainerFwd> // QVector<QString>

// Type of improvement. (Read from buildings.ruleset file.)
struct impr_type {
  Impr_type_id item_number;
  struct name_translation name;
  bool ruledit_disabled; /* Does not really exist - hole in improvements
                            array */
  char graphic_str[MAX_LEN_NAME]; // city icon of improv.
  char graphic_alt[MAX_LEN_NAME]; // city icon of improv.
  struct requirement_vector reqs;
  struct requirement_vector obsolete_by;
  int build_cost; // Use wrappers to access this.
  int upkeep;
  int sabotage;             // Base chance of diplomat sabotage succeeding.
  enum impr_genus_id genus; // genus; e.g. GreatWonder
  bv_impr_flags flags;
  QVector<QString> *helptext;
  char soundtag[MAX_LEN_NAME];
  char soundtag_alt[MAX_LEN_NAME];

  // Cache
  bool allows_units;
  bool allows_extras;
  bool prevents_disaster;
  bool protects_vs_actions;
};

// General improvement accessor functions.
Impr_type_id improvement_count();
Impr_type_id improvement_index(const struct impr_type *pimprove);
Impr_type_id improvement_number(const struct impr_type *pimprove);

struct impr_type *improvement_by_number(const Impr_type_id id);

const struct impr_type *valid_improvement(const struct impr_type *pimprove);

struct impr_type *improvement_by_rule_name(const char *name);
struct impr_type *improvement_by_translated_name(const char *name);

const char *improvement_rule_name(const struct impr_type *pimprove);
const char *improvement_name_translation(const struct impr_type *pimprove);

// General improvement flag accessor routines
bool improvement_has_flag(const struct impr_type *pimprove,
                          enum impr_flag_id flag);

// Ancillary routines
int impr_build_shield_cost(const struct city *pcity,
                           const struct impr_type *pimprove);
int impr_base_build_shield_cost(const struct impr_type *pimprove);
int impr_estimate_build_shield_cost(const struct player *pplayer,
                                    const struct tile *ptile,
                                    const struct impr_type *pimprove);
int impr_buy_gold_cost(const struct city *pcity,
                       const struct impr_type *pimprove,
                       int shields_in_stock);
int impr_sell_gold(const struct impr_type *pimprove);

bool is_improvement_visible(const struct impr_type *pimprove);

bool is_great_wonder(const struct impr_type *pimprove);
bool is_small_wonder(const struct impr_type *pimprove);
bool is_wonder(const struct impr_type *pimprove);
bool is_improvement(const struct impr_type *pimprove);
bool is_special_improvement(const struct impr_type *pimprove);

bool can_improvement_go_obsolete(const struct impr_type *pimprove);

bool can_sell_building(const struct impr_type *pimprove);
bool can_city_sell_building(const struct city *pcity,
                            const struct impr_type *pimprove);
enum test_result
test_player_sell_building_now(struct player *pplayer, const city *pcity,
                              const struct impr_type *pimprove);

const struct impr_type *
improvement_replacement(const struct impr_type *pimprove);

// Macros for struct packet_game_info::great_wonder_owners[].
#define WONDER_DESTROYED                                                    \
  (MAX_NUM_PLAYER_SLOTS + 1) /* Used as player id.                          \
                              */
#define WONDER_NOT_OWNED                                                    \
  (MAX_NUM_PLAYER_SLOTS + 2) /* Used as player id.                          \
                              */
#define WONDER_OWNED(player_id) ((player_id) < MAX_NUM_PLAYER_SLOTS)

// Macros for struct player::wonders[].
#define WONDER_LOST (-1)   // Used as city id.
#define WONDER_NOT_BUILT 0 // Used as city id.
#define WONDER_BUILT(city_id) ((city_id) > 0)

void wonder_built(const struct city *pcity,
                  const struct impr_type *pimprove);
void wonder_destroyed(const struct city *pcity,
                      const struct impr_type *pimprove);

bool wonder_is_lost(const struct player *pplayer,
                    const struct impr_type *pimprove);
bool wonder_is_built(const struct player *pplayer,
                     const struct impr_type *pimprove);
struct city *city_from_wonder(const struct player *pplayer,
                              const struct impr_type *pimprove);

bool great_wonder_is_built(const struct impr_type *pimprove);
bool great_wonder_is_destroyed(const struct impr_type *pimprove);
bool great_wonder_is_available(const struct impr_type *pimprove);
struct city *city_from_great_wonder(const struct impr_type *pimprove);
struct player *great_wonder_owner(const struct impr_type *pimprove);

struct city *city_from_small_wonder(const struct player *pplayer,
                                    const struct impr_type *pimprove);

// player related improvement functions
bool improvement_obsolete(const struct player *pplayer,
                          const struct impr_type *pimprove,
                          const struct city *pcity);
bool is_improvement_productive(const struct city *pcity,
                               const struct impr_type *pimprove);
bool is_improvement_redundant(const struct city *pcity,
                              const struct impr_type *pimprove);

bool can_player_build_improvement_direct(const struct player *p,
                                         const struct impr_type *pimprove);
bool can_player_build_improvement_later(const struct player *p,
                                        const struct impr_type *pimprove);
bool can_player_build_improvement_now(const struct player *p,
                                      struct impr_type *pimprove);

// Initialization and iteration
void improvements_init();
void improvements_free();

void improvement_feature_cache_init();

struct impr_type *improvement_array_first();
const struct impr_type *improvement_array_last();

#define improvement_iterate(_p)                                             \
  {                                                                         \
    struct impr_type *_p = improvement_array_first();                       \
    if (nullptr != _p) {                                                    \
      for (; _p <= improvement_array_last(); _p++) {

#define improvement_iterate_end                                             \
  }                                                                         \
  }                                                                         \
  }

#define improvement_re_active_iterate(_p)                                   \
  improvement_iterate(_p)                                                   \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define improvement_re_active_iterate_end                                   \
  }                                                                         \
  }                                                                         \
  improvement_iterate_end;
