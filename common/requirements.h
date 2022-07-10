/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

// common
#include "fc_types.h"

/* Range of requirements.
 * Used in the network protocol.
 * Order is important -- wider ranges should come later -- some code
 * assumes a total order, or tests for e.g. >= REQ_RANGE_PLAYER.
 * Ranges of similar types should be supersets, for example:
 *  - the set of Adjacent tiles contains the set of CAdjacent tiles,
 *    and both contain the center Local tile (a requirement on the local
 *    tile is also within Adjacent range);
 *  - World contains Alliance contains Player (a requirement we ourselves
 *    have is also within Alliance range). */
#define SPECENUM_NAME req_range
#define SPECENUM_VALUE0 REQ_RANGE_LOCAL
#define SPECENUM_VALUE0NAME "Local"
#define SPECENUM_VALUE1 REQ_RANGE_CADJACENT
#define SPECENUM_VALUE1NAME "CAdjacent"
#define SPECENUM_VALUE2 REQ_RANGE_ADJACENT
#define SPECENUM_VALUE2NAME "Adjacent"
#define SPECENUM_VALUE3 REQ_RANGE_CITY
#define SPECENUM_VALUE3NAME "City"
#define SPECENUM_VALUE4 REQ_RANGE_TRADEROUTE
#define SPECENUM_VALUE4NAME "Traderoute"
#define SPECENUM_VALUE5 REQ_RANGE_CONTINENT
#define SPECENUM_VALUE5NAME "Continent"
#define SPECENUM_VALUE6 REQ_RANGE_PLAYER
#define SPECENUM_VALUE6NAME "Player"
#define SPECENUM_VALUE7 REQ_RANGE_TEAM
#define SPECENUM_VALUE7NAME "Team"
#define SPECENUM_VALUE8 REQ_RANGE_ALLIANCE
#define SPECENUM_VALUE8NAME "Alliance"
#define SPECENUM_VALUE9 REQ_RANGE_WORLD
#define SPECENUM_VALUE9NAME "World"
#define SPECENUM_COUNT REQ_RANGE_COUNT // keep this last
#include "specenum_gen.h"

#define req_range_iterate(_range_)                                          \
  {                                                                         \
    enum req_range _range_;                                                 \
    for (_range_ = REQ_RANGE_LOCAL; _range_ < REQ_RANGE_COUNT;              \
         _range_ = (enum req_range)(_range_ + 1)) {

#define req_range_iterate_end                                               \
  }                                                                         \
  }

/* A requirement. This requirement is basically a conditional; it may or
 * may not be active on a target.  If it is active then something happens.
 * For instance units and buildings have requirements to be built, techs
 * have requirements to be researched, and effects have requirements to be
 * active.
 * Used in the network protocol. */
struct requirement {
  struct universal source; // requirement source
  enum req_range range;    // requirement range
  bool survives;           /* set if destroyed sources satisfy the req*/
  bool present;            // set if the requirement is to be present
  bool quiet;              // do not list this in helptext
};

#define SPECVEC_TAG requirement
#define SPECVEC_TYPE struct requirement
#include "specvec.h"
#define requirement_vector_iterate(req_vec, preq)                           \
  TYPED_VECTOR_ITERATE(struct requirement, req_vec, preq)
#define requirement_vector_iterate_end VECTOR_ITERATE_END

// General requirement functions.
struct requirement req_from_str(const char *type, const char *range,
                                bool survives, bool present, bool quiet,
                                const char *value);
QString req_to_fstring(const struct requirement *req);

void req_get_values(const struct requirement *req, int *type, int *range,
                    bool *survives, bool *present, bool *quiet, int *value);
struct requirement req_from_values(int type, int range, bool survives,
                                   bool present, bool quiet, int value);

bool are_requirements_equal(const struct requirement *req1,
                            const struct requirement *req2);

bool are_requirements_contradictions(const struct requirement *req1,
                                     const struct requirement *req2);

bool does_req_contradicts_reqs(const struct requirement *req,
                               const struct requirement_vector *vec);

bool is_req_active(
    const struct player *target_player, const struct player *other_player,
    const struct city *target_city, const struct impr_type *target_building,
    const struct tile *target_tile, const struct unit *target_unit,
    const struct unit_type *target_unittype,
    const struct output_type *target_output,
    const struct specialist *target_specialist,
    const struct action *target_action, const struct requirement *req,
    const enum req_problem_type prob_type,
    const enum vision_layer vision_layer = V_COUNT);
bool are_reqs_active(const struct player *target_player,
                     const struct player *other_player,
                     const struct city *target_city,
                     const struct impr_type *target_building,
                     const struct tile *target_tile,
                     const struct unit *target_unit,
                     const struct unit_type *target_unittype,
                     const struct output_type *target_output,
                     const struct specialist *target_specialist,
                     const struct action *target_action,
                     const struct requirement_vector *reqs,
                     const enum req_problem_type prob_type,
                     const enum vision_layer vision_layer = V_COUNT);

bool is_req_unchanging(const struct requirement *req);

bool is_req_in_vec(const struct requirement *req,
                   const struct requirement_vector *vec);

bool req_vec_wants_type(const struct requirement_vector *reqs,
                        enum universals_n kind);

/**
 * @brief req_vec_num_in_item a requirement vectors number in an item.
 *
 * A ruleset item can have more than one requirement vector. This numbers
 * them. Allows applying a change suggested based on one ruleset item to
 * another ruleset item of the same kind - say to a copy.
 */
typedef signed char req_vec_num_in_item;

/**
   Returns the requirement vector number of the specified requirement
   vector in the specified parent item or -1 if the vector doesn't belong to
   the parent item.
   @param parent_item the item that may own the vector.
   @param vec the requirement vector to number.
   @return the requirement vector number the vector has in the parent item.
 */
typedef req_vec_num_in_item (*requirement_vector_number)(
    const void *parent_item, const struct requirement_vector *vec);

/**
   Returns a writable pointer to the specified requirement vector in the
   specified parent item or nullptr if the parent item doesn't have a
   requirement vector with that requirement vector number.
   @param parent_item the item that should have the requirement vector.
   @param number the item's requirement vector number.
   @return a pointer to the specified requirement vector.
 */
typedef struct requirement_vector *(*requirement_vector_by_number)(
    const void *parent_item, req_vec_num_in_item number);

/**
   Returns the name of the specified requirement vector number in the
   parent item or nullptr if parent items of the kind this function is for
   don't have a requirement vector with that number.
   @param number the requirement vector to name
   @return the requirement vector name or nullptr.
 */
typedef const char *(*requirement_vector_namer)(req_vec_num_in_item number);

req_vec_num_in_item
req_vec_vector_number(const void *parent_item,
                      const struct requirement_vector *vec);

/* Interactive friendly requirement vector change suggestions and
 * reasoning. */
#define SPECENUM_NAME req_vec_change_operation
#define SPECENUM_VALUE0 RVCO_REMOVE
#define SPECENUM_VALUE0NAME N_("Remove")
#define SPECENUM_VALUE1 RVCO_APPEND
#define SPECENUM_VALUE1NAME N_("Append")
#define SPECENUM_COUNT RVCO_NOOP
#include "specenum_gen.h"

struct req_vec_change {
  enum req_vec_change_operation operation;
  struct requirement req;

  req_vec_num_in_item vector_number;
};

struct req_vec_problem {
  /* Can't use name_translation because it is MAX_LEN_NAME long and a
   * description may contain more than one name. */
  char description[500];
  char description_translated[500];

  int num_suggested_solutions;
  struct req_vec_change *suggested_solutions;
};

const char *req_vec_change_translation(const struct req_vec_change *change,
                                       const requirement_vector_namer namer);

bool req_vec_change_apply(const struct req_vec_change *modification,
                          requirement_vector_by_number getter,
                          const void *parent_item);

struct req_vec_problem *req_vec_problem_new(int num_suggested_solutions,
                                            const char *description, ...);
void req_vec_problem_free(struct req_vec_problem *issue);

struct req_vec_problem *
req_vec_get_first_contradiction(const struct requirement_vector *vec,
                                requirement_vector_number get_num,
                                const void *parent_item);

// General universal functions.
int universal_number(const struct universal *source);

struct universal universal_by_number(const enum universals_n kind,
                                     const int value);
struct universal universal_by_rule_name(const char *kind, const char *value);
void universal_value_from_str(struct universal *source, const char *value);
void universal_extraction(const struct universal *source, int *kind,
                          int *value);

bool are_universals_equal(const struct universal *psource1,
                          const struct universal *psource2);

const char *universal_rule_name(const struct universal *psource);
const char *universal_name_translation(const struct universal *psource,
                                       char *buf, size_t bufsz);
const char *universal_type_rule_name(const struct universal *psource);

int universal_build_shield_cost(const struct city *pcity,
                                const struct universal *target);

bool universal_replace_in_req_vec(struct requirement_vector *reqs,
                                  const struct universal *to_replace,
                                  const struct universal *replacement);

#define universal_is_mentioned_by_requirement(preq, psource)                \
  are_universals_equal(&preq->source, psource)
bool universal_is_mentioned_by_requirements(
    const struct requirement_vector *reqs, const struct universal *psource);

// An item contradicts, fulfills or is irrelevant to the requirement
enum req_item_found { ITF_NO, ITF_YES, ITF_NOT_APPLICABLE };

void universal_found_functions_init();
enum req_item_found
universal_fulfills_requirement(const struct requirement *preq,
                               const struct universal *source);
bool universal_fulfills_requirements(bool check_necessary,
                                     const struct requirement_vector *reqs,
                                     const struct universal *source);
bool sv_universal_fulfills_requirements(
    bool check_necessary, const struct requirement_vector *reqs,
    const struct universal source);
bool universal_is_relevant_to_requirement(const struct requirement *req,
                                          const struct universal *source);

#define universals_iterate(_univ_)                                          \
  {                                                                         \
    enum universals_n _univ_;                                               \
    for (_univ_ = VUT_NONE; _univ_ < VUT_COUNT;                             \
         _univ_ = (enum universals_n)(_univ_ + 1)) {

#define universals_iterate_end                                              \
  }                                                                         \
  }

/* Accessors to determine if a universal fulfills a requirement vector.
 * When adding an additional accessor, be sure to add the appropriate
 * item_found function in universal_found_functions_init(). */
// XXX Some versions of g++ can't cope with the struct literals
#define requirement_fulfilled_by_government(_gov_, _rqs_)                   \
  sv_universal_fulfills_requirements(                                       \
      false, (_rqs_),                                                       \
      (struct universal){.value = {.govern = (_gov_)},                      \
                         .kind = VUT_GOVERNMENT})
#define requirement_fulfilled_by_nation(_nat_, _rqs_)                       \
  sv_universal_fulfills_requirements(                                       \
      false, (_rqs_),                                                       \
      (struct universal){.value = {.nation = (_nat_)}, .kind = VUT_NATION})
#define requirement_fulfilled_by_improvement(_imp_, _rqs_)                  \
  sv_universal_fulfills_requirements(                                       \
      false, (_rqs_),                                                       \
      (struct universal){.value = {.building = (_imp_)},                    \
                         .kind = VUT_IMPROVEMENT})
#define requirement_fulfilled_by_terrain(_ter_, _rqs_)                      \
  sv_universal_fulfills_requirements(                                       \
      false, (_rqs_),                                                       \
      (struct universal){.value = {.terrain = (_ter_)},                     \
                         .kind = VUT_TERRAIN})
#define requirement_fulfilled_by_unit_class(_uc_, _rqs_)                    \
  sv_universal_fulfills_requirements(                                       \
      false, (_rqs_),                                                       \
      (struct universal){.value = {.uclass = (_uc_)}, .kind = VUT_UCLASS})
#define requirement_fulfilled_by_unit_type(_ut_, _rqs_)                     \
  sv_universal_fulfills_requirements(                                       \
      false, (_rqs_),                                                       \
      (struct universal){.value = {.utype = (_ut_)}, .kind = VUT_UTYPE})

#define requirement_needs_improvement(_imp_, _rqs_)                         \
  sv_universal_fulfills_requirements(                                       \
      true, (_rqs_),                                                        \
      (struct universal){.value = {.building = (_imp_)},                    \
                         .kind = VUT_IMPROVEMENT})

int requirement_kind_ereq(const int value, const enum req_range range,
                          const bool present, const int max_value);

#define requirement_diplrel_ereq(_id_, _range_, _present_)                  \
  requirement_kind_ereq(_id_, _range_, _present_, DRO_LAST)
