// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "bitvector.h"
#include "fcintl.h"

// common
#include "fc_types.h"
#include "name_translation.h" // struct name_translation
#include "requirements.h"     // struct requirement_vector

// Qt
class QString;
#include <QtContainerFwd> // QVector<QString>

struct ai_type;

/* The largest distance a ruleset can allow a unit to paradrop.
 *
 * Remember to make sure that the field type of PACKET_RULESET_UNIT's
 * paratroopers_range field can transfer the new maximum if you increase
 * it.
 *
 * The top value is reserved in case a future Freeciv21 version wants to
 * implement "no maximum range". It could be used to signal that the unit
 * can paradrop anywhere. Note that the value below it is high enough to
 * give the same effect on all maps inside the current size limits.
 * (No map side can be larger than MAP_MAX_LINEAR_SIZE)
 */
#define UNIT_MAX_PARADROP_RANGE (65535 - 1)

#define UCF_LAST_USER_FLAG UCF_USER_FLAG_12
#define MAX_NUM_USER_UCLASS_FLAGS (UCF_LAST_USER_FLAG - UCF_USER_FLAG_1 + 1)

// Used in savegame processing and clients.
#define SPECENUM_NAME unit_move_type
#define SPECENUM_VALUE0 UMT_LAND
#define SPECENUM_VALUE0NAME "Land"
#define SPECENUM_VALUE1 UMT_SEA
#define SPECENUM_VALUE1NAME "Sea"
#define SPECENUM_VALUE2 UMT_BOTH
#define SPECENUM_VALUE2NAME "Both"
#include "specenum_gen.h"

enum hut_behavior { HUT_NORMAL, HUT_NOTHING, HUT_FRIGHTEN };

enum move_level { MOVE_NONE, MOVE_PARTIAL, MOVE_FULL };

struct extra_type_list;
struct unit_class_list;

struct unit_class {
  Unit_Class_id item_number;
  struct name_translation name;
  bool ruledit_disabled;
  enum unit_move_type move_type;
  int min_speed;   // Minimum speed after damage and effects
  int hp_loss_pct; /* Percentage of hitpoints lost each turn not in city or
                      airbase */
  int non_native_def_pct;
  enum hut_behavior hut_behavior;
  bv_unit_class_flags flags;

  QVector<QString> *helptext;

  struct {
    enum move_level land_move;
    enum move_level sea_move;
  } adv;

  struct {
    struct extra_type_list *refuel_bases;
    struct extra_type_list *native_tile_extras;
    struct extra_type_list *bonus_roads;
    struct unit_class_list *subset_movers;
  } cache;
};

struct combat_bonus {
  enum unit_type_flag_id flag;
  enum combat_bonus_type type;
  int value;

  // Not listed in the help text.
  bool quiet;
};

// get 'struct combat_bonus_list' and related functions:
#define SPECLIST_TAG combat_bonus
#define SPECLIST_TYPE struct combat_bonus
#include "speclist.h"

#define combat_bonus_list_iterate(bonuslist, pbonus)                        \
  TYPED_LIST_ITERATE(struct combat_bonus, bonuslist, pbonus)
#define combat_bonus_list_iterate_end LIST_ITERATE_END

BV_DEFINE(bv_unit_types, U_LAST);

struct veteran_level {
  struct name_translation name; /* level/rank name */
  int power_fact; /* combat/work speed/diplomatic power factor (in %) */
  int move_bonus;
  int base_raise_chance; // server only
  int work_raise_chance; // server only
};

struct veteran_system {
  int levels;

  struct veteran_level *definitions;
};

struct unit_type {
  Unit_type_id item_number;
  struct name_translation name;
  bool ruledit_disabled; /* Does not really exist - hole in improvments array
                          */
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char sound_move[MAX_LEN_NAME];
  char sound_move_alt[MAX_LEN_NAME];
  char sound_fight[MAX_LEN_NAME];
  char sound_fight_alt[MAX_LEN_NAME];
  int build_cost; // Use wrappers to access this.
  int pop_cost;   /* number of workers the unit contains (e.g., settlers,
                     engineers)*/
  int attack_strength;
  int defense_strength;
  int move_rate;
  int unknown_move_cost; // See utype_unknown_move_cost().

  struct advance *require_advance; // may be nullptr
  struct requirement_vector build_reqs;

  int vision_radius_sq;
  int transport_capacity;
  int hp;
  int firepower;
  struct combat_bonus_list *bonuses;

#define U_NOT_OBSOLETED (nullptr)
  const struct unit_type *obsoleted_by;
  const struct unit_type *converted_to;
  int convert_time;
  int fuel;

  bv_unit_type_flags flags;
  bv_unit_type_roles roles;

  int happy_cost; // unhappy people in home city
  int upkeep[O_LAST];

  // Only valid for ACTION_PARADROP
  int paratroopers_range;

  // Additional values for the expanded veteran system
  struct veteran_system *veteran;

  // Values for bombardment
  int bombard_rate;

  // Values for founding cities
  int city_size;

  int city_slots;

  struct unit_class *uclass;

  bv_unit_classes cargo;

  // Can attack these classes even if they are otherwise "Unreachable"
  bv_unit_classes targets;
  /* Can load into these class transports at any location,
   * even if they are otherwise "Unreachable". */
  bv_unit_classes embarks;
  /* Can unload from these class transports at any location,
   * even if they are otherwise "Unreachable". */
  bv_unit_classes disembarks;

  enum vision_layer vlayer;

  QVector<QString> *helptext;

  struct {
    bool igwall;
    bool worker;
  } adv;

  struct {
    int max_defense_mp_pct; /* Value 0 here does not guarantee that unit
                             * never has CBONUS_DEFENSE_MULTIPLIER, it
                             * merely means that there's no POSITIVE one */
    int defense_mp_bonuses_pct[U_LAST];
  } cache;

  // Used to upgrade the ruleset format version.
  struct {
    int paratroopers_mr_req;
    int paratroopers_mr_sub;
  } rscompat_cache;

  void *ais[FREECIV_AI_MOD_LAST];
};

// General unit and unit type (matched) routines
Unit_type_id utype_count();
Unit_type_id utype_index(const struct unit_type *punittype);
Unit_type_id utype_number(const struct unit_type *punittype);

const struct unit_type *unit_type_get(const struct unit *punit);
struct unit_type *utype_by_number(const Unit_type_id id);

struct unit_type *unit_type_by_rule_name(const char *name);
struct unit_type *unit_type_by_translated_name(const char *name);

const char *unit_rule_name(const struct unit *punit);
const char *utype_rule_name(const struct unit_type *punittype);

const char *unit_name_translation(const struct unit *punit);
const char *utype_name_translation(const struct unit_type *punittype);

const char *utype_values_string(const struct unit_type *punittype);
const char *utype_values_translation(const struct unit_type *punittype);

// General unit type flag and role routines
bool unit_has_type_flag(const struct unit *punit,
                        enum unit_type_flag_id flag);

/**************************************************************************
  Return whether the given unit type has the flag.
**************************************************************************/
static inline bool utype_has_flag(const struct unit_type *punittype,
                                  int flag)
{
  return BV_ISSET(punittype->flags, flag);
}

bool unit_has_type_role(const struct unit *punit, enum unit_role_id role);
bool utype_has_role(const struct unit_type *punittype, int role);

void user_unit_type_flags_init();
void set_user_unit_type_flag_name(enum unit_type_flag_id id,
                                  const char *name, const char *helptxt);
const char *unit_type_flag_helptxt(enum unit_type_flag_id id);

bool unit_can_take_over(const struct unit *punit);
bool utype_can_take_over(const struct unit_type *punittype);

bool utype_can_freely_load(const struct unit_type *pcargotype,
                           const struct unit_type *ptranstype);
bool utype_can_freely_unload(const struct unit_type *pcargotype,
                             const struct unit_type *ptranstype);

bool utype_may_act_at_all(const struct unit_type *putype);
bool utype_can_do_action(const struct unit_type *putype,
                         const action_id act_id);
bool utype_can_do_action_result(const struct unit_type *putype,
                                enum action_result result);
bool utype_acts_hostile(const struct unit_type *putype);

bool can_unit_act_when_ustate_is(const struct unit_type *punit_type,
                                 const enum ustate_prop prop,
                                 const bool is_there);
bool utype_can_do_act_when_ustate(const struct unit_type *punit_type,
                                  const action_id act_id,
                                  const enum ustate_prop prop,
                                  const bool is_there);

bool utype_can_do_act_if_tgt_citytile(const struct unit_type *punit_type,
                                      const action_id act_id,
                                      const enum citytile_type prop,
                                      const bool is_there);

bool can_utype_do_act_if_tgt_diplrel(const struct unit_type *punit_type,
                                     const action_id act_id, const int prop,
                                     const bool is_there);

bool utype_may_act_move_frags(const struct unit_type *punit_type,
                              const action_id act_id,
                              const int move_fragments);

bool utype_may_act_tgt_city_tile(const struct unit_type *punit_type,
                                 const action_id act_id,
                                 const enum citytile_type prop,
                                 const bool is_there);

bool utype_is_consumed_by_action(const struct action *paction,
                                 const struct unit_type *utype);

bool utype_is_consumed_by_action_result(enum action_result result,
                                        const struct unit_type *utype);

bool utype_is_moved_to_tgt_by_action(const struct action *paction,
                                     const struct unit_type *utype);

bool utype_is_unmoved_by_action(const struct action *paction,
                                const struct unit_type *utype);

bool utype_pays_for_regular_move_to_tgt(const struct action *paction,
                                        const struct unit_type *utype);

int utype_pays_mp_for_action_base(const struct action *paction,
                                  const struct unit_type *putype);

int utype_pays_mp_for_action_estimate(const struct action *paction,
                                      const struct unit_type *putype,
                                      const struct player *act_player,
                                      const struct tile *act_tile,
                                      const struct tile *tgt_tile);

// Functions to operate on various flag and roles.
typedef bool (*role_unit_callback)(struct unit_type *ptype, void *data);

void role_unit_precalcs();
void role_unit_precalcs_free();
int num_role_units(int role);
struct unit_type *
role_units_iterate_backwards(int role, role_unit_callback cb, void *data);
struct unit_type *get_role_unit(int role, int role_index);
struct unit_type *best_role_unit(const struct city *pcity, int role);
struct unit_type *best_role_unit_for_player(const struct player *pplayer,
                                            int role);
struct unit_type *first_role_unit_for_player(const struct player *pplayer,
                                             int role);
bool role_units_translations(QString &astr, int flag, bool alts);

// General unit class routines
Unit_Class_id uclass_count();
Unit_Class_id uclass_number(const struct unit_class *pclass);
/* Optimised to be identical to uclass_number: the implementation
 * unittype.c is also semantically correct. */
#define uclass_index(_c_) (_c_)->item_number
#ifndef uclass_index
Unit_Class_id uclass_index(const struct unit_class *pclass);
#endif // uclass_index

struct unit_class *unit_class_get(const struct unit *punit);
struct unit_class *uclass_by_number(const Unit_Class_id id);
#define utype_class(_t_) (_t_)->uclass
#ifndef utype_class
struct unit_class *utype_class(const struct unit_type *punittype);
#endif // utype_class

struct unit_class *unit_class_by_rule_name(const char *s);

const char *uclass_rule_name(const struct unit_class *pclass);
const char *uclass_name_translation(const struct unit_class *pclass);

/**************************************************************************
  Return whether the given unit class has the flag.
**************************************************************************/
static inline bool uclass_has_flag(const struct unit_class *punitclass,
                                   enum unit_class_flag_id flag)
{
  return BV_ISSET(punitclass->flags, flag);
}

void user_unit_class_flags_init();
void set_user_unit_class_flag_name(enum unit_class_flag_id id,
                                   const char *name, const char *helptxt);
const char *unit_class_flag_helptxt(enum unit_class_flag_id id);

// Ancillary routines
int unit_build_shield_cost(const struct city *pcity,
                           const struct unit *punit);
int utype_build_shield_cost(const struct city *pcity,
                            const struct unit_type *punittype);
int utype_build_shield_cost_base(const struct unit_type *punittype);
int unit_build_shield_cost_base(const struct unit *punit);

int utype_buy_gold_cost(const struct city *pcity,
                        const struct unit_type *punittype,
                        int shields_in_stock);

const struct veteran_system *
utype_veteran_system(const struct unit_type *punittype);
int utype_veteran_levels(const struct unit_type *punittype);
const struct veteran_level *
utype_veteran_level(const struct unit_type *punittype, int level);
const char *utype_veteran_name_translation(const struct unit_type *punittype,
                                           int level);
bool utype_veteran_has_power_bonus(const struct unit_type *punittype);

struct veteran_system *veteran_system_new(int count);
void veteran_system_destroy(struct veteran_system *vsystem);
void veteran_system_definition(struct veteran_system *vsystem, int level,
                               const char *vlist_name, int vlist_power,
                               int vlist_move, int vlist_raise,
                               int vlist_wraise);

int unit_pop_value(const struct unit *punit);
int utype_pop_value(const struct unit_type *punittype);

enum unit_move_type utype_move_type(const struct unit_type *punittype);
void set_unit_move_type(struct unit_class *puclass);

// player related unit functions
int utype_upkeep_cost(const struct unit_type *ut, struct player *pplayer,
                      Output_type_id otype);
int utype_happy_cost(const struct unit_type *ut,
                     const struct player *pplayer);

const struct unit_type *
can_upgrade_unittype(const struct player *pplayer,
                     const struct unit_type *punittype);
int unit_upgrade_price(const struct player *pplayer,
                       const struct unit_type *from,
                       const struct unit_type *to);

bool utype_player_already_has_this_unique(const struct player *pplayer,
                                          const struct unit_type *putype);

bool can_player_build_unit_direct(const struct player *p,
                                  const struct unit_type *punittype);
bool can_player_build_unit_later(const struct player *p,
                                 const struct unit_type *punittype);
bool can_player_build_unit_now(const struct player *p,
                               const struct unit_type *punittype);

#define utype_fuel(ptype) (ptype)->fuel

bool utype_is_cityfounder(const struct unit_type *utype);

// Initialization and iteration
void unit_types_init();
void unit_types_free();
void unit_type_flags_free();
void unit_class_flags_free();

struct unit_type *unit_type_array_first();
const struct unit_type *unit_type_array_last();

#define unit_type_iterate(_p)                                               \
  {                                                                         \
    struct unit_type *_p = unit_type_array_first();                         \
    if (nullptr != _p) {                                                    \
      for (; _p <= unit_type_array_last(); _p++) {

#define unit_type_iterate_end                                               \
  }                                                                         \
  }                                                                         \
  }

#define unit_type_re_active_iterate(_p)                                     \
  unit_type_iterate(_p)                                                     \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define unit_type_re_active_iterate_end                                     \
  }                                                                         \
  }                                                                         \
  unit_type_iterate_end;

void *utype_ai_data(const struct unit_type *ptype, const struct ai_type *ai);
void utype_set_ai_data(struct unit_type *ptype, const struct ai_type *ai,
                       void *data);

void unit_type_action_cache_set(struct unit_type *ptype);
void unit_type_action_cache_init();

// Initialization and iteration
void unit_classes_init();
void unit_classes_free();

void set_unit_class_caches(struct unit_class *pclass);
void set_unit_type_caches(struct unit_type *ptype);

struct unit_class *unit_class_array_first();
const struct unit_class *unit_class_array_last();

#define unit_class_iterate(_p)                                              \
  {                                                                         \
    struct unit_class *_p = unit_class_array_first();                       \
    if (nullptr != _p) {                                                    \
      for (; _p <= unit_class_array_last(); _p++) {

#define unit_class_iterate_end                                              \
  }                                                                         \
  }                                                                         \
  }

#define unit_class_re_active_iterate(_p)                                    \
  unit_class_iterate(_p)                                                    \
  {                                                                         \
    if (!_p->ruledit_disabled) {

#define unit_class_re_active_iterate_end                                    \
  }                                                                         \
  }                                                                         \
  unit_class_iterate_end;

#define SPECLIST_TAG unit_class
#define SPECLIST_TYPE struct unit_class
#include "speclist.h"

#define unit_class_list_iterate(uclass_list, pclass)                        \
  TYPED_LIST_ITERATE(struct unit_class, uclass_list, pclass)
#define unit_class_list_iterate_end LIST_ITERATE_END

#define SPECLIST_TAG unit_type
#define SPECLIST_TYPE struct unit_type
#include "speclist.h"

#define unit_type_list_iterate(utype_list, ptype)                           \
  TYPED_LIST_ITERATE(struct unit_type, utype_list, ptype)
#define unit_type_list_iterate_end LIST_ITERATE_END
