// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "fcintl.h"

// common
#include "fc_types.h"
#include "requirements.h" // struct requirement_vector

// Qt
class QString;

/* A battle is against a defender that tries to stop the action where the
 * defender is in danger. A dice roll without a defender risking anything,
 * like the roll controlled by EFT_ACTION_ODDS_PCT, isn't a battle. */
#define SPECENUM_NAME action_battle_kind
#define SPECENUM_VALUE0 ABK_NONE
#define SPECENUM_VALUE0NAME N_("no battle")
#define SPECENUM_VALUE1 ABK_STANDARD
#define SPECENUM_VALUE1NAME N_("battle")
#define SPECENUM_VALUE2 ABK_DIPLOMATIC
#define SPECENUM_VALUE2NAME N_("diplomatic battle")
#define SPECENUM_COUNT ABK_COUNT
#include "specenum_gen.h"

// Describes how a unit sucessfully performing an action will move it.
#define SPECENUM_NAME moves_actor_kind
#define SPECENUM_VALUE0 MAK_STAYS
#define SPECENUM_VALUE0NAME N_("stays")
#define SPECENUM_VALUE1 MAK_REGULAR
#define SPECENUM_VALUE1NAME N_("regular")
#define SPECENUM_VALUE2 MAK_TELEPORT
#define SPECENUM_VALUE2NAME N_("teleport")
#define SPECENUM_VALUE3 MAK_ESCAPE
#define SPECENUM_VALUE3NAME N_("escape")
#define SPECENUM_VALUE4 MAK_FORCED
#define SPECENUM_VALUE4NAME N_("forced")
#define SPECENUM_VALUE5 MAK_UNREPRESENTABLE
#define SPECENUM_VALUE5NAME N_("unrepresentable")
#include "specenum_gen.h"

// Who ordered the action to be performed?
#define SPECENUM_NAME action_requester
// The player ordered it directly.
#define SPECENUM_VALUE0 ACT_REQ_PLAYER
#define SPECENUM_VALUE0NAME N_("the player")
// The game it self because the rules requires it.
#define SPECENUM_VALUE1 ACT_REQ_RULES
#define SPECENUM_VALUE1NAME N_("the game rules")
// A server side autonomous agent working for the player.
#define SPECENUM_VALUE2 ACT_REQ_SS_AGENT
#define SPECENUM_VALUE2NAME N_("a server agent")
// Number of action requesters.
#define SPECENUM_COUNT ACT_REQ_COUNT
#include "specenum_gen.h"

/* The last action distance value that is interpreted as an actual
 * distance rather than as a signal value.
 *
 * It is specified literally rather than referring to MAP_DISTANCE_MAX
 * because Freeciv-web's MAP_DISTANCE_MAX differs from the regular Freeciv
 * server's MAP_DISTANCE_MAX. A static assertion in actions.c makes sure
 * that it can cover the whole map. */
#define ACTION_DISTANCE_LAST_NON_SIGNAL 128016
// No action max distance to target limit.
#define ACTION_DISTANCE_UNLIMITED (ACTION_DISTANCE_LAST_NON_SIGNAL + 1)
// No action max distance can be bigger than this.
#define ACTION_DISTANCE_MAX ACTION_DISTANCE_UNLIMITED

// Action target complexity
#define SPECENUM_NAME act_tgt_compl
/* The action's target is just the primary target. (Just the tile, unit,
 * city, etc). */
#define SPECENUM_VALUE0 ACT_TGT_COMPL_SIMPLE
#define SPECENUM_VALUE0NAME N_("simple")
/* The action's target is complex because its target is the primary target
 * and a sub target. (Examples: Tile + Extra and City + Building.) The
 * player is able to specify details about this action but the server will
 * fill in missing details so a client can choose to not specify the sub
 * target. */
#define SPECENUM_VALUE1 ACT_TGT_COMPL_FLEXIBLE
#define SPECENUM_VALUE1NAME N_("flexible")
/* The action's target is complex because its target is the primary target
 * and a sub target. (Examples: Tile + Extra and City + Building.) The
 * player is required to specify details about this action because the
 * server won't fill inn the missing details when unspecified. A client must
 * therefore specify the sub target of this action. */
#define SPECENUM_VALUE2 ACT_TGT_COMPL_MANDATORY
#define SPECENUM_VALUE2NAME N_("mandatory")
#include "specenum_gen.h"

struct action {
  action_id id;

  enum action_result result;

  enum action_actor_kind actor_kind;
  enum action_target_kind target_kind;
  enum action_sub_target_kind sub_target_kind;

  // Sub target policy.
  enum act_tgt_compl target_complexity;

  /* Limits on the distance on the map between the actor and the target.
   * The action is legal iff the distance is min_distance, max_distance or
   * a value in between. */
  int min_distance, max_distance;

  // The name of the action shown in the UI
  char ui_name[MAX_LEN_NAME];

  /* Suppress automatic help text generation about what enables and/or
   * disables this action. */
  bool quiet;

  /* Actions that blocks this action. The action will be illegal if any
   * bloking action is legal. */
  bv_actions blocked_by;

  /* Successfully performing this action will always consume the actor.
   * Don't set this for actions that consumes the unit in some cases
   * (depending on luck, the presence of a flag, etc) but not in other
   * cases. */
  bool actor_consuming_always;

  union {
    struct {
      /* A unit's ability to perform this action will pop up the action
       * selection dialog before the player asks for it only in exceptional
       * cases.
       *
       * The motivation for setting rare_pop_up is to minimize player
       * annoyance and mistakes. Getting a pop up every time a unit moves is
       * annoying. An unexpected offer to do something that in many cases is
       * destructive can lead the player's muscle memory to perform the
       * wrong action. */
      bool rare_pop_up;

      // The unitwaittime setting blocks this action when done too soon.
      bool unitwaittime_controlled;

      /* How successfully performing the specified action always will move
       * the actor unit of the specified type. */
      enum moves_actor_kind moves_actor;
    } is_unit;
  } actor;
};

struct action_enabler {
  bool disabled;
  action_id action;
  struct requirement_vector actor_reqs;
  struct requirement_vector target_reqs;
};

#define enabler_get_action(_enabler_) action_by_number(_enabler_->action)

#define SPECLIST_TAG action_enabler
#define SPECLIST_TYPE struct action_enabler
#include "speclist.h"
#define action_enabler_list_iterate(action_enabler_list, aenabler)          \
  TYPED_LIST_ITERATE(struct action_enabler, action_enabler_list, aenabler)
#define action_enabler_list_iterate_end LIST_ITERATE_END

#define action_iterate(_act_)                                               \
  {                                                                         \
    action_id _act_;                                                        \
    for (_act_ = 0; _act_ < NUM_ACTIONS; _act_++) {

#define action_iterate_end                                                  \
  }                                                                         \
  }

#define action_by_result_iterate(_paction_, _act_id_, _result_)             \
  {                                                                         \
    action_iterate(_act_id_)                                                \
    {                                                                       \
      struct action *_paction_ = action_by_number(_act_id_);                \
      if (!action_has_result(_paction_, _result_)) {                        \
        continue;                                                           \
      }

#define action_by_result_iterate_end                                        \
  }                                                                         \
  action_iterate_end;                                                       \
  }

#define action_list_iterate(_act_list_, _act_id_)                           \
  {                                                                         \
    int _pos_;                                                              \
                                                                            \
    for (_pos_ = 0; _pos_ < NUM_ACTIONS; _pos_++) {                         \
      const action_id _act_id_ = _act_list_[_pos_];                         \
                                                                            \
      if (_act_id_ == ACTION_NONE) {                                        \
        /* No more actions in this list. */                                 \
        break;                                                              \
      }

#define action_list_iterate_end                                             \
  }                                                                         \
  }

#define action_enablers_iterate(_enabler_)                                  \
  {                                                                         \
    action_iterate(_act_)                                                   \
    {                                                                       \
      action_enabler_list_iterate(action_enablers_for_action(_act_),        \
                                  _enabler_)                                \
      {

#define action_enablers_iterate_end                                         \
  }                                                                         \
  action_enabler_list_iterate_end;                                          \
  }                                                                         \
  action_iterate_end;                                                       \
  }

/* An Action Auto Performer rule makes an actor try to perform an action
 * without being ordered to do so by the player controlling it.
 * - the first auto performer that matches the cause and fulfills the reqs
 *   is selected.
 * - the actions listed by the selected auto performer is tried in order
 *   until an action is successful, all actions have been tried or the
 *   actor disappears.
 * - if no action inside the selected auto performer is legal no action is
 *   performed. The system won't try to select another auto performer.
 */
struct action_auto_perf {
  // The reason for trying to auto perform an action.
  enum action_auto_perf_cause cause;

  /* Must be fulfilled if the game should try to force an action from this
   * action auto performer. */
  struct requirement_vector reqs;

  /* Auto perform the first legal action in this list.
   * The list is terminated by ACTION_NONE. */
  action_id alternatives[MAX_NUM_ACTIONS];
};

#define action_auto_perf_iterate(_act_perf_)                                \
  {                                                                         \
    int _ap_num_;                                                           \
                                                                            \
    for (_ap_num_ = 0;                                                      \
         _ap_num_ < MAX_NUM_ACTION_AUTO_PERFORMERS                          \
         && (action_auto_perf_by_number(_ap_num_)->cause != AAPC_COUNT);    \
         _ap_num_++) {                                                      \
      const struct action_auto_perf *_act_perf_ =                           \
          action_auto_perf_by_number(_ap_num_);

#define action_auto_perf_iterate_end                                        \
  }                                                                         \
  }

#define action_auto_perf_by_cause_iterate(_cause_, _act_perf_)              \
  action_auto_perf_iterate(_act_perf_)                                      \
  {                                                                         \
    if (_act_perf_->cause != _cause_) {                                     \
      continue;                                                             \
    }

#define action_auto_perf_by_cause_iterate_end                               \
  }                                                                         \
  action_auto_perf_iterate_end

#define action_auto_perf_actions_iterate(_autoperf_, _act_id_)              \
  action_list_iterate(_autoperf_->alternatives, _act_id_)

#define action_auto_perf_actions_iterate_end action_list_iterate_end

/* Hard coded location of action auto performers. Used for conversion while
 * action auto performers aren't directly exposed to the ruleset. */
#define ACTION_AUTO_UPKEEP_FOOD 0
#define ACTION_AUTO_UPKEEP_GOLD 1
#define ACTION_AUTO_UPKEEP_SHIELD 2
#define ACTION_AUTO_MOVED_ADJ 3

// Initialization
void actions_init();
void actions_rs_pre_san_gen();
void actions_free();

bool actions_are_ready();

bool action_id_exists(const action_id act_id);

struct action *action_by_number(action_id act_id);
struct action *action_by_rule_name(const char *name);

enum action_actor_kind action_get_actor_kind(const struct action *paction);
#define action_id_get_actor_kind(act_id)                                    \
  action_get_actor_kind(action_by_number(act_id))
enum action_target_kind action_get_target_kind(const struct action *paction);
#define action_id_get_target_kind(act_id)                                   \
  action_get_target_kind(action_by_number(act_id))
enum action_sub_target_kind
action_get_sub_target_kind(const struct action *paction);
#define action_id_get_sub_target_kind(act_id)                               \
  action_get_sub_target_kind(action_by_number(act_id))

enum action_battle_kind action_get_battle_kind(const struct action *pact);

int action_number(const struct action *action);

bool action_has_result(const struct action *paction,
                       enum action_result result);
#define action_has_result_safe(paction, result)                             \
  (paction && action_has_result(paction, result))
#define action_id_has_result_safe(act_id, result)                           \
  (action_by_number(act_id)                                                 \
   && action_has_result(action_by_number(act_id), result))

bool action_has_complex_target(const struct action *paction);
#define action_id_has_complex_target(act_id)                                \
  action_has_complex_target(action_by_number(act_id))
bool action_requires_details(const struct action *paction);
#define action_id_requires_details(act_id)                                  \
  action_requires_details(action_by_number(act_id))

int action_get_act_time(const struct action *paction,
                        const struct unit *actor_unit,
                        const struct tile *tgt_tile,
                        const struct extra_type *tgt_extra);
#define action_id_get_act_time(act_id, actor_unit, tgt_tile, tgt_extra)     \
  action_get_act_time(action_by_number(act_id), actor_unit, tgt_tile,       \
                      tgt_extra)

bool action_creates_extra(const struct action *paction,
                          const struct extra_type *pextra);
bool action_removes_extra(const struct action *paction,
                          const struct extra_type *pextra);

bool action_id_is_rare_pop_up(action_id act_id);

bool action_distance_accepted(const struct action *action,
                              const int distance);
#define action_id_distance_accepted(act_id, distance)                       \
  action_distance_accepted(action_by_number(act_id), distance)

bool action_distance_inside_max(const struct action *action,
                                const int distance);
#define action_id_distance_inside_max(act_id, distance)                     \
  action_distance_inside_max(action_by_number(act_id), distance)

bool action_would_be_blocked_by(const struct action *blocked,
                                const struct action *blocker);
#define action_id_would_be_blocked_by(blocked_id, blocker_id)               \
  action_would_be_blocked_by(action_by_number(blocked_id),                  \
                             action_by_number(blocker_id))

int action_get_role(const struct action *paction);
#define action_id_get_role(act_id) action_get_role(action_by_number(act_id))

enum unit_activity action_get_activity(const struct action *paction);
#define action_id_get_activity(act_id)                                      \
  action_get_activity(action_by_number(act_id))

const char *action_rule_name(const struct action *action);
const char *action_id_rule_name(action_id act_id);

const QString action_name_translation(const struct action *action);
const QString action_id_name_translation(action_id act_id);
const QString action_prepare_ui_name(action_id act_id, const char *mnemonic,
                                     const struct act_prob prob,
                                     const QString &custom);

const char *action_ui_name_ruleset_var_name(int act);
const char *action_ui_name_default(int act);

const char *action_min_range_ruleset_var_name(int act);
int action_min_range_default(int act);
const char *action_max_range_ruleset_var_name(int act);
int action_max_range_default(int act);

const char *action_target_kind_ruleset_var_name(int act);
const char *action_actor_consuming_always_ruleset_var_name(action_id act);

struct action_enabler_list *action_enablers_for_action(action_id action);

struct action_enabler *action_enabler_new();
void action_enabler_free(struct action_enabler *enabler);
struct action_enabler *
action_enabler_copy(const struct action_enabler *original);
void action_enabler_add(struct action_enabler *enabler);
bool action_enabler_remove(struct action_enabler *enabler);

struct req_vec_problem *
action_enabler_suggest_repair_oblig(const struct action_enabler *enabler);
struct req_vec_problem *
action_enabler_suggest_repair(const struct action_enabler *enabler);
struct req_vec_problem *
action_enabler_suggest_improvement(const struct action_enabler *enabler);

req_vec_num_in_item
action_enabler_vector_number(const void *enabler,
                             const struct requirement_vector *vec);
struct requirement_vector *
action_enabler_vector_by_number(const void *enabler,
                                req_vec_num_in_item vec);
const char *action_enabler_vector_by_number_name(req_vec_num_in_item vec);

struct action *action_is_blocked_by(const action_id act_id,
                                    const struct unit *actor_unit,
                                    const struct tile *target_tile,
                                    const struct city *target_city,
                                    const struct unit *target_unit);

bool is_action_enabled_unit_on_city(const action_id wanted_action,
                                    const struct unit *actor_unit,
                                    const struct city *target_city);

bool is_action_enabled_unit_on_unit(const action_id wanted_action,
                                    const struct unit *actor_unit,
                                    const struct unit *target_unit);

bool is_action_enabled_unit_on_units(const action_id wanted_action,
                                     const struct unit *actor_unit,
                                     const struct tile *target_tile);

bool is_action_enabled_unit_on_tile(const action_id wanted_action,
                                    const struct unit *actor_unit,
                                    const struct tile *target_tile,
                                    const struct extra_type *target_extra);

bool is_action_enabled_unit_on_self(const action_id wanted_action,
                                    const struct unit *actor_unit);

struct act_prob action_prob_vs_city(const struct unit *actor,
                                    const action_id act_id,
                                    const struct city *victim);

struct act_prob action_prob_vs_unit(const struct unit *actor,
                                    const action_id act_id,
                                    const struct unit *victim);

struct act_prob action_prob_vs_units(const struct unit *actor,
                                     const action_id act_id,
                                     const struct tile *victims);

struct act_prob action_prob_vs_tile(const struct unit *actor,
                                    const action_id act_id,
                                    const struct tile *victims,
                                    const struct extra_type *target_extra);

struct act_prob action_prob_self(const struct unit *actor,
                                 const action_id act_id);

struct act_prob action_prob_unit_vs_tgt(const struct action *paction,
                                        const struct unit *act_unit,
                                        const struct city *tgt_city,
                                        const struct unit *tgt_unit,
                                        const struct tile *tgt_tile,
                                        const struct extra_type *sub_tgt);

struct act_prob action_speculate_unit_on_city(action_id act_id,
                                              const struct unit *actor,
                                              const struct city *actor_home,
                                              const struct tile *actor_tile,
                                              bool omniscient_cheat,
                                              const struct city *target);

struct act_prob action_speculate_unit_on_units(action_id act_id,
                                               const struct unit *actor,
                                               const struct city *actor_home,
                                               const struct tile *actor_tile,
                                               bool omniscient_cheat,
                                               const struct tile *target);

struct act_prob action_speculate_unit_on_tile(
    action_id act_id, const struct unit *actor,
    const struct city *actor_home, const struct tile *actor_tile,
    bool omniscient_cheat, const struct tile *target_tile,
    const struct extra_type *target_extra);

struct act_prob action_speculate_unit_on_self(action_id act_id,
                                              const struct unit *actor,
                                              const struct city *actor_home,
                                              const struct tile *actor_tile,
                                              bool omniscient_cheat);

bool action_prob_possible(const struct act_prob probability);

bool are_action_probabilitys_equal(const struct act_prob *ap1,
                                   const struct act_prob *ap2);

int action_prob_cmp_pessimist(const struct act_prob ap1,
                              const struct act_prob ap2);

double action_prob_to_0_to_1_pessimist(const struct act_prob ap);

struct act_prob action_prob_fall_back(const struct act_prob *ap1,
                                      const struct act_prob *ap2);

const QString action_prob_explain(const struct act_prob prob);

struct act_prob action_prob_new_impossible();
struct act_prob action_prob_new_not_relevant();
struct act_prob action_prob_new_not_impl();
struct act_prob action_prob_new_unknown();
struct act_prob action_prob_new_certain();

/* Special action probability values. Documented in fc_types.h's
 * definition of struct act_prob. */
#define ACTPROB_IMPOSSIBLE action_prob_new_impossible()
#define ACTPROB_CERTAIN action_prob_new_certain()
#define ACTPROB_NA action_prob_new_not_relevant()
#define ACTPROB_NOT_IMPLEMENTED action_prob_new_not_impl()
#define ACTPROB_NOT_KNOWN action_prob_new_unknown()

// ACTION_ODDS_PCT_DICE_ROLL_NA must be above 100%.
#define ACTION_ODDS_PCT_DICE_ROLL_NA 110
int action_dice_roll_initial_odds(const struct action *paction);
int action_dice_roll_odds(const struct player *act_player,
                          const struct unit *act_unit,
                          const struct city *tgt_city,
                          const struct player *tgt_player,
                          const struct action *paction);

bool action_actor_utype_hard_reqs_ok(enum action_result result,
                                     const struct unit_type *actor_unittype);

// Reasoning about actions
bool action_univs_not_blocking(const struct action *paction,
                               struct universal *actor_uni,
                               struct universal *target_uni);
#define action_id_univs_not_blocking(act_id, act_uni, tgt_uni)              \
  action_univs_not_blocking(action_by_number(act_id), act_uni, tgt_uni)

bool action_immune_government(struct government *gov, action_id act);

bool is_action_possible_on_city(action_id act_id,
                                const struct player *actor_player,
                                const struct city *target_city);

bool is_action_possible_on_unit(action_id act_id, const unit *target_unit);

bool action_maybe_possible_actor_unit(const action_id wanted_action,
                                      const struct unit *actor_unit);

bool action_mp_full_makes_legal(const struct unit *actor,
                                const action_id act_id);

// Action lists
void action_list_end(action_id *act_list, int size);
void action_list_add_all_by_result(action_id *act_list, int *position,
                                   enum action_result result);

// Action auto performers
const struct action_auto_perf *action_auto_perf_by_number(const int num);
struct action_auto_perf *action_auto_perf_slot_number(const int num);

// Find targets
struct city *action_tgt_city(struct unit *actor, struct tile *target_tile,
                             bool accept_all_actions);

struct unit *action_tgt_unit(struct unit *actor, struct tile *target_tile,
                             bool accept_all_actions);

struct tile *action_tgt_tile(struct unit *actor, struct tile *target_tile,
                             const struct extra_type *target_extra,
                             bool accept_all_actions);

struct extra_type *action_tgt_tile_extra(const struct unit *actor,
                                         const struct tile *target_tile,
                                         bool accept_all_actions);
