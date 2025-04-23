// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "city.h"
#include "fc_types.h"
#include "name_translation.h"
#include "requirements.h"

// Used in the network protocol.
#define SPECENUM_NAME disaster_effect_id
#define SPECENUM_VALUE0 DE_DESTROY_BUILDING
#define SPECENUM_VALUE0NAME "DestroyBuilding"
#define SPECENUM_VALUE1 DE_REDUCE_POP
#define SPECENUM_VALUE1NAME "ReducePopulation"
#define SPECENUM_VALUE2 DE_EMPTY_FOODSTOCK
#define SPECENUM_VALUE2NAME "EmptyFoodStock"
#define SPECENUM_VALUE3 DE_EMPTY_PRODSTOCK
#define SPECENUM_VALUE3NAME "EmptyProdStock"
#define SPECENUM_VALUE4 DE_POLLUTION
#define SPECENUM_VALUE4NAME "Pollution"
#define SPECENUM_VALUE5 DE_FALLOUT
#define SPECENUM_VALUE5NAME "Fallout"
#define SPECENUM_VALUE6 DE_REDUCE_DESTROY
#define SPECENUM_VALUE6NAME "ReducePopDestroy"
#define SPECENUM_COUNT DE_COUNT
#define SPECENUM_BITVECTOR bv_disaster_effects
#include "specenum_gen.h"

#define DISASTER_BASE_RARITY 1000000

struct disaster_type {
  int id;
  struct name_translation name;

  struct requirement_vector reqs;

  /* Final probability for each city each turn is
   * this frequency * game.info.disasters frequency setting /
   * DISASTER_BASE_RARITY */
  int frequency;

  bv_disaster_effects effects;
};

// Initialization and iteration
void disaster_types_init();
void disaster_types_free();

Disaster_type_id disaster_index(const struct disaster_type *pdis);
Disaster_type_id disaster_number(const struct disaster_type *pdis);

struct disaster_type *disaster_by_number(Disaster_type_id id);

const char *disaster_name_translation(struct disaster_type *pdis);
const char *disaster_rule_name(struct disaster_type *pdis);

bool disaster_has_effect(const struct disaster_type *pdis,
                         enum disaster_effect_id effect);

bool can_disaster_happen(const struct disaster_type *pdis,
                         const struct city *pcity);

#define disaster_type_iterate(_p)                                           \
  {                                                                         \
    int _i_;                                                                \
    for (_i_ = 0; _i_ < game.control.num_disaster_types; _i_++) {           \
      struct disaster_type *_p = disaster_by_number(_i_);

#define disaster_type_iterate_end                                           \
  }                                                                         \
  }
