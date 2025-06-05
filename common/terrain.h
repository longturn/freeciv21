// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "fcintl.h"

// common
#include "fc_types.h"
#include "name_translation.h" // struct name_translation
#include "unittype.h"

// Qt
class QString;
#include <QtContainerFwd> // QStringList = QList<QString>

// std
#include <string> // std::string

struct base_type;
struct rgbcolor;

// Used in the network protocol.
enum special_river_move {
  RMV_NORMAL = 0,
  RMV_FAST_STRICT = 1,
  RMV_FAST_RELAXED = 2,
  RMV_FAST_ALWAYS = 3,
};

// ===

struct resource_type {
  std::string rule_name;
  char id_old_save; // Single-character identifier used in old savegames.
#define RESOURCE_NULL_IDENTIFIER '\0'
#define RESOURCE_NONE_IDENTIFIER ' '

  int output[O_LAST]; // Amount added by this resource.

  struct extra_type *self;
};

// ===

#define T_NONE (nullptr)    // A special flag meaning no terrain type.
#define T_UNKNOWN (nullptr) // An unknown terrain.

// The first terrain value.
#define T_FIRST 0

// Used in the network protocol.
#define SPECENUM_NAME terrain_class
#define SPECENUM_VALUE0 TC_LAND
// TRANS: terrain class: used adjectivally
#define SPECENUM_VALUE0NAME N_("Land")
#define SPECENUM_VALUE1 TC_OCEAN
// TRANS: terrain class: used adjectivally
#define SPECENUM_VALUE1NAME N_("Oceanic")
#define SPECENUM_COUNT TC_COUNT
#include "specenum_gen.h"

/* Types of alterations available to terrain.
 * This enum is only used in the effects system; the relevant information
 * is encoded in other members of the terrain structure.
 *
 * Used in the network protocol. */
#define SPECENUM_NAME terrain_alteration
// Can build irrigation without changing terrain
#define SPECENUM_VALUE0 TA_CAN_IRRIGATE
/* TRANS: this and following strings may rarely be presented to the player
 * in ruleset help text, to denote the set of terrains which can be altered
 * in a particular way */
#define SPECENUM_VALUE0NAME N_("CanIrrigate")
// Can build mine without changing terrain
#define SPECENUM_VALUE1 TA_CAN_MINE
#define SPECENUM_VALUE1NAME N_("CanMine")
/* Can build roads and/or railroads */
#define SPECENUM_VALUE2 TA_CAN_ROAD
#define SPECENUM_VALUE2NAME N_("CanRoad")
#define SPECENUM_COUNT TA_COUNT
#include "specenum_gen.h"

#define MAX_NUM_USER_TER_FLAGS (TER_USER_LAST - TER_USER_1 + 1)

#define SPECENUM_NAME mapgen_terrain_property
#define SPECENUM_VALUE0 MG_MOUNTAINOUS
#define SPECENUM_VALUE0NAME "mountainous"
#define SPECENUM_VALUE1 MG_GREEN
#define SPECENUM_VALUE1NAME "green"
#define SPECENUM_VALUE2 MG_FOLIAGE
#define SPECENUM_VALUE2NAME "foliage"
#define SPECENUM_VALUE3 MG_TROPICAL
#define SPECENUM_VALUE3NAME "tropical"
#define SPECENUM_VALUE4 MG_TEMPERATE
#define SPECENUM_VALUE4NAME "temperate"
#define SPECENUM_VALUE5 MG_COLD
#define SPECENUM_VALUE5NAME "cold"
#define SPECENUM_VALUE6 MG_FROZEN
#define SPECENUM_VALUE6NAME "frozen"
#define SPECENUM_VALUE7 MG_WET
#define SPECENUM_VALUE7NAME "wet"
#define SPECENUM_VALUE8 MG_DRY
#define SPECENUM_VALUE8NAME "dry"
#define SPECENUM_VALUE9 MG_OCEAN_DEPTH
#define SPECENUM_VALUE9NAME "ocean_depth"
#define SPECENUM_COUNT MG_COUNT
#include "specenum_gen.h"

/*
 * This struct gives data about each terrain type.  There are many ways
 * it could be extended.
 */
struct terrain {
  int item_number;
  struct name_translation name;
  bool ruledit_disabled; // Does not really exist - hole in terrain array
  char graphic_str[MAX_LEN_NAME]; // add tile_ prefix
  char graphic_alt[MAX_LEN_NAME];

  char identifier;      // Single-character identifier used in games saved.
  char identifier_load; /* Single-character identifier that was used in the
                         * savegame loaded. */

#define TERRAIN_UNKNOWN_IDENTIFIER 'u'

  enum terrain_class tclass;

  int movement_cost; // whole MP, not scaled by SINGLE_MOVE
  int defense_bonus; // % defense bonus - defaults to zero

  int output[O_LAST];

  struct extra_type **resources; // nullptr-terminated

  int road_output_incr_pct[O_LAST];
  int base_time;
  int road_time;

  int cultivate_time;

  int plant_time;

  struct terrain *irrigation_result;
  int irrigation_food_incr;
  int irrigation_time;

  struct terrain *mining_result;
  int mining_shield_incr;
  int mining_time;

  int placing_time;

  struct terrain *transform_result;
  int transform_time;
  int clean_pollution_time;
  int clean_fallout_time;
  int pillage_time;

  const struct unit_type *animal;

  // May be nullptr if the transformation is impossible.
  struct terrain *warmer_wetter_result, *warmer_drier_result;
  struct terrain *cooler_wetter_result, *cooler_drier_result;

  /* These are special properties of the terrain used by mapgen.  If a tile
   * has a property, then the value gives the weighted amount of tiles that
   * will be assigned this terrain.
   *
   * For instance if mountains have 70 and hills have 30 of MG_MOUNTAINOUS
   * then 70% of 'mountainous' tiles will be given mountains.
   *
   * Ocean_depth is different.  Instead of a percentage, the depth of the
   * tile in the range 0 (never chosen) to 100 (deepest) is used.
   */
  int property[MG_COUNT];
#define TERRAIN_OCEAN_DEPTH_MINIMUM (1)
#define TERRAIN_OCEAN_DEPTH_MAXIMUM (100)

  bv_unit_classes native_to;

  bv_terrain_flags flags;

  struct rgbcolor *rgb;

  QVector<QString> *helptext;
};

// General terrain accessor functions.
Terrain_type_id terrain_count();
Terrain_type_id terrain_index(const struct terrain *pterrain);
Terrain_type_id terrain_number(const struct terrain *pterrain);

struct terrain *terrain_by_number(const Terrain_type_id type);

struct terrain *terrain_by_rule_name(const char *name);
struct terrain *terrain_by_translated_name(const char *name);

char terrain_identifier(const struct terrain *pterrain);
const char *terrain_rule_name(const struct terrain *pterrain);
const char *terrain_name_translation(const struct terrain *pterrain);

// Functions to operate on a terrain flag.
#define terrain_has_flag(terr, flag) BV_ISSET((terr)->flags, flag)

bool is_terrain_flag_card_near(const struct tile *ptile,
                               enum terrain_flag_id flag);
bool is_terrain_flag_near_tile(const struct tile *ptile,
                               enum terrain_flag_id flag);
int count_terrain_flag_near_tile(const struct tile *ptile,
                                 bool cardinal_only, bool percentage,
                                 enum terrain_flag_id flag);
void user_terrain_flags_init();
void user_terrain_flags_free();
void set_user_terrain_flag_name(enum terrain_flag_id id, const char *name,
                                const char *helptxt);
const char *terrain_flag_helptxt(enum terrain_flag_id id);

// Terrain-specific functions.
#define is_ocean(pterrain)                                                  \
  ((pterrain) != T_UNKNOWN                                                  \
   && terrain_type_terrain_class(pterrain) == TC_OCEAN)
#define is_ocean_tile(ptile) is_ocean(tile_terrain(ptile))

bool terrain_has_resource(const struct terrain *pterrain,
                          const struct extra_type *presource);

// Functions to operate on a general terrain type.
bool is_terrain_card_near(const struct tile *ptile,
                          const struct terrain *pterrain, bool check_self);
bool is_terrain_near_tile(const struct tile *ptile,
                          const struct terrain *pterrain, bool check_self);
int count_terrain_property_near_tile(const struct tile *ptile,
                                     bool cardinal_only, bool percentage,
                                     enum mapgen_terrain_property prop);

struct resource_type *resource_type_init(struct extra_type *pextra);
void resource_types_free();

// Special helper functions
const QStringList get_infrastructure_texts(bv_extras extras);
struct extra_type *get_preferred_pillage(bv_extras extras);

int terrain_extra_build_time(const struct terrain *pterrain,
                             enum unit_activity activity,
                             const struct extra_type *tgt);
int terrain_extra_removal_time(const struct terrain *pterrain,
                               enum unit_activity activity,
                               const struct extra_type *tgt);

// Functions to operate on a terrain class.
const char *terrain_class_name_translation(enum terrain_class tclass);

enum terrain_class
terrain_type_terrain_class(const struct terrain *pterrain);
bool is_terrain_class_card_near(const struct tile *ptile,
                                enum terrain_class tclass);
bool is_terrain_class_near_tile(const struct tile *ptile,
                                enum terrain_class tclass);
int count_terrain_class_near_tile(const struct tile *ptile,
                                  bool cardinal_only, bool percentage,
                                  enum terrain_class tclass);

// Functions to deal with possible terrain alterations.
bool terrain_can_support_alteration(const struct terrain *pterrain,
                                    enum terrain_alteration talter);

// Initialization and iteration
void terrains_init();
void terrains_free();

struct terrain *terrain_array_first();
const struct terrain *terrain_array_last();

#define terrain_type_iterate(_p)                                            \
  {                                                                         \
    struct terrain *_p = terrain_array_first();                             \
    if (nullptr != _p) {                                                    \
      for (; _p <= terrain_array_last(); _p++) {

#define terrain_type_iterate_end                                            \
  }                                                                         \
  }                                                                         \
  }

#define terrain_re_active_iterate(_p)                                       \
  terrain_type_iterate(_p)                                                  \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define terrain_re_active_iterate_end                                       \
  }                                                                         \
  }                                                                         \
  terrain_type_iterate_end;
