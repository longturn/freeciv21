// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"
#include "multipliers.h"  // struct multiplier
#include "requirements.h" // struct requirement_vector

// Qt
class QString;

// std
#include <cstddef> // size_t

/* An effect is provided by a source.  If the source is present, and the
 * other conditions (described below) are met, the effect will be active.
 * Note the difference between effect and effect_type. */
struct effect {
  enum effect_type type;

  // pointer to multipliers (nullptr means that this effect has no multiplier
  struct multiplier *multiplier;

  /* The "value" of the effect.  The meaning of this varies between
   * effects.  When get_xxx_bonus() is called the value of all applicable
   * effects will be summed up. */
  int value;

  /* An effect can have multiple requirements.  The effect will only be
   * active if all of these requirement are met. */
  struct requirement_vector reqs;
};

// An effect_list is a list of effects.
#define SPECLIST_TAG effect
#define SPECLIST_TYPE struct effect
#include "speclist.h"
#define effect_list_iterate(effect_list, peffect)                           \
  TYPED_LIST_ITERATE(struct effect, effect_list, peffect)
#define effect_list_iterate_end LIST_ITERATE_END

struct effect *effect_new(enum effect_type type, int value,
                          struct multiplier *pmul);
struct effect *effect_copy(struct effect *old);
void effect_req_append(struct effect *peffect, struct requirement req);
void get_effect_req_text(const struct effect *peffect, char *buf,
                         size_t buf_len);
QString get_effect_list_req_text(const struct effect_list *plist);
QString effect_type_unit_text(effect_type type, int value);

// ruleset cache creation and communication functions
struct packet_ruleset_effect;

void ruleset_cache_init();
void ruleset_cache_free();
void recv_ruleset_effect(const struct packet_ruleset_effect *packet);
void send_ruleset_cache(struct conn_list *dest);

int effect_cumulative_max(enum effect_type type, struct universal *for_uni);
int effect_cumulative_min(enum effect_type type, struct universal *for_uni);

int effect_value_from_universals(enum effect_type type,
                                 struct universal *unis, size_t n_unis);

bool is_building_replaced(const struct city *pcity,
                          const struct impr_type *pimprove,
                          const enum req_problem_type prob_type);

// functions to know the bonuses a certain effect is granting
int get_world_bonus(enum effect_type effect_type);
int get_player_bonus(const struct player *plr, enum effect_type effect_type);
int get_city_bonus(const struct city *pcity, enum effect_type effect_type,
                   enum vision_layer vlayer = V_COUNT);
int get_city_specialist_output_bonus(const struct city *pcity,
                                     const struct specialist *pspecialist,
                                     const struct output_type *poutput,
                                     enum effect_type effect_type);
int get_city_tile_output_bonus(const struct city *pcity,
                               const struct tile *ptile,
                               const struct output_type *poutput,
                               enum effect_type effect_type);
int get_tile_output_bonus(const struct city *pcity, const struct tile *ptile,
                          const struct output_type *poutput,
                          enum effect_type effect_type);
int get_player_output_bonus(const struct player *pplayer,
                            const struct output_type *poutput,
                            enum effect_type effect_type);
int get_player_intel_bonus(const struct player *pplayer,
                           const struct player *pother,
                           enum national_intelligence nintel,
                           enum effect_type effect_type);
int get_city_output_bonus(const struct city *pcity,
                          const struct output_type *poutput,
                          enum effect_type effect_type);
int get_building_bonus(const struct city *pcity,
                       const struct impr_type *building,
                       enum effect_type effect_type);
int get_unittype_bonus(const struct player *pplayer,
                       const struct tile *ptile, // pcity is implied
                       const struct unit_type *punittype,
                       enum effect_type effect_type,
                       enum vision_layer vision_layer = V_COUNT);
int get_unit_bonus(const struct unit *punit, enum effect_type effect_type);

// miscellaneous auxiliary effects functions
struct effect_list *get_req_source_effects(struct universal *psource);

int get_player_bonus_effects(struct effect_list *plist,
                             const struct player *pplayer,
                             enum effect_type effect_type);
int get_city_bonus_effects(struct effect_list *plist,
                           const struct city *pcity,
                           const struct output_type *poutput,
                           enum effect_type effect_type);

int get_target_bonus_effects(
    struct effect_list *plist, const struct player *target_player,
    const struct player *other_player, const struct city *target_city,
    const struct impr_type *target_building, const struct tile *target_tile,
    const struct unit *target_unit, const struct unit_type *target_unittype,
    const struct output_type *target_output,
    const struct specialist *target_specialist,
    const struct action *target_action, enum effect_type effect_type,
    enum vision_layer vision_layer = V_COUNT,
    enum national_intelligence nintel = NI_COUNT);
int get_target_bonus(const struct req_context *target_context,
                     const struct req_context *other_context,
                     enum effect_type effect_type);
int get_target_bonus_effects(struct effect_list *plist,
                             const struct req_context *target_context,
                             const struct req_context *other_context,
                             enum effect_type effect_type);

bool building_has_effect(const struct impr_type *pimprove,
                         enum effect_type effect_type);
int get_current_construction_bonus(const struct city *pcity,
                                   enum effect_type effect_type,
                                   const enum req_problem_type prob_type);
int get_potential_improvement_bonus(const struct impr_type *pimprove,
                                    const struct city *pcity,
                                    enum effect_type effect_type,
                                    const enum req_problem_type prob_type);

const effect_list *get_effects();
struct effect_list *get_effects(enum effect_type effect_type);

typedef bool (*iec_cb)(struct effect *, void *data);
bool iterate_effect_cache(iec_cb cb, void *data);
