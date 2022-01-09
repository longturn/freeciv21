/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cctype>
#include <cstring>

// utility
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "shared.h" // ARRAY_SIZE
#include "support.h"

// common
#include "city.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "map.h"
#include "multipliers.h"
#include "packets.h"
#include "player.h"
#include "tech.h"

#include "effects.h"

static bool initialized = false;

/**************************************************************************
  The code creates a ruleset cache on ruleset load. This constant cache
  is used to speed up effects queries.  After the cache is created it is
  not modified again (though it may later be freed).

  Since the cache is constant, the server only needs to send effects data to
  the client upon connect. It also means that an AI can do fast searches in
  the effects space by trying the possible combinations of addition or
  removal of buildings with the effects it cares about.


  To know how much a target is being affected, simply use the convenience
  functions:

  * get_player_bonus
  * get_city_bonus
  * get_city_tile_bonus
  * get_building_bonus

  These functions require as arguments the target and the effect type to be
  queried.

  Effect sources are unique and at a well known place in the
  data structures.  This allows the above queries to be fast:
    - Look up the possible sources for the effect (O(1) lookup)
    - For each source, find out if it is present (O(1) lookup per source).
  The first is commonly called the "ruleset cache" and is stored statically
  in this file.  The second is the "sources cache" and is stored all over.

  Any type of effect range and "survives" is possible if we have a sources
  cache for that combination.  For instance
    - There is a sources cache of all existing buildings in a city; thus any
      building effect in a city can have city range.
    - There is a sources cache of all wonders in the world; thus any wonder
      effect can have world range.
    - There is a sources cache of all wonders for each player; thus any
      wonder effect can have player range.
    - There is a sources cache of all wonders ever built; thus any wonder
      effect that survives can have world range.
  However there is no sources cache for many of the possible sources.  For
  instance non-unique buildings do not have a world-range sources cahce, so
  you can't have a non-wonder building have a world-ranged effect.

  The sources caches could easily be extended by generalizing it to a set
  of arrays
    game.buildings[], pplayer->buildings[],
    pisland->builidngs[], pcity->buildings[]
  which would store the number of buildings of that type present by game,
  player, island (continent) or city.  This would allow non-surviving effects
  to come from any building at any range.  However to allow surviving effects
  a second set of arrays would be needed.  This should enable basic support
  for small wonders and satellites.

  No matter which sources caches are present, we should always know where
  to look for a source and so the lookups will always be fast even as the
  number of possible sources increases.
**************************************************************************/

/**************************************************************************
  Ruleset cache. The cache is created during ruleset loading and the data
  is organized to enable fast queries.
**************************************************************************/
static struct {
  // A single list containing every effect.
  struct effect_list *tracker;

  /* This array provides a full list of the effects of this type
   * (It's not really a cache, it's the real data.) */
  struct effect_list *effects[EFT_COUNT];

  struct {
    // This cache shows for each building, which effects it provides.
    struct effect_list *buildings[B_LAST];
    // Same for governments
    struct effect_list *govs[G_LAST];
    // ...advances...
    struct effect_list *advances[A_LAST];
  } reqs;
} ruleset_cache;

/**
   Get a list of all effects.
 */
const effect_list *get_effects() { return ruleset_cache.tracker; }

/**
   Get a list of effects of this type.
 */
struct effect_list *get_effects(enum effect_type effect_type)
{
  return ruleset_cache.effects[effect_type];
}

/**
   Get a list of effects with this requirement source.

   Note: currently only buildings and governments are supported.
 */
struct effect_list *get_req_source_effects(struct universal *psource)
{
  int type, value;

  universal_extraction(psource, &type, &value);

  switch (type) {
  case VUT_GOVERNMENT:
    if (value >= 0 && value < government_count()) {
      return ruleset_cache.reqs.govs[value];
    } else {
      return NULL;
    }
  case VUT_IMPROVEMENT:
    if (value >= 0 && value < improvement_count()) {
      return ruleset_cache.reqs.buildings[value];
    } else {
      return NULL;
    }
  case VUT_ADVANCE:
    if (value >= 0 && value < advance_count()) {
      return ruleset_cache.reqs.advances[value];
    } else {
      return NULL;
    }
  default:
    return NULL;
  }
}

/**
   Add effect to ruleset cache.
 */
struct effect *effect_new(enum effect_type type, int value,
                          struct multiplier *pmul)
{
  // Create the effect.
  auto *peffect = new effect;
  peffect->type = type;
  peffect->value = value;
  peffect->multiplier = pmul;

  requirement_vector_init(&peffect->reqs);

  // Now add the effect to the ruleset cache.
  effect_list_append(ruleset_cache.tracker, peffect);
  effect_list_append(get_effects(type), peffect);

  return peffect;
}

/**
   Return new copy of the effect
 */
struct effect *effect_copy(struct effect *old)
{
  struct effect *new_eff =
      effect_new(old->type, old->value, old->multiplier);

  requirement_vector_iterate(&old->reqs, preq)
  {
    effect_req_append(new_eff, *preq);
  }
  requirement_vector_iterate_end;

  return new_eff;
}

/**
   Append requirement to effect.
 */
void effect_req_append(struct effect *peffect, struct requirement req)
{
  struct effect_list *eff_list = get_req_source_effects(&req.source);

  requirement_vector_append(&peffect->reqs, req);

  if (eff_list) {
    effect_list_append(eff_list, peffect);
  }
}

/**
   Initialize the ruleset cache.  The ruleset cache should be empty
   before this is done (so if it's previously been initialized, it needs
   to be freed (see ruleset_cache_free) before it can be reused).
 */
void ruleset_cache_init()
{
  int i;

  initialized = true;

  ruleset_cache.tracker = effect_list_new();

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.effects); i++) {
    ruleset_cache.effects[i] = effect_list_new();
  }
  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.buildings); i++) {
    ruleset_cache.reqs.buildings[i] = effect_list_new();
  }
  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.govs); i++) {
    ruleset_cache.reqs.govs[i] = effect_list_new();
  }
  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.advances); i++) {
    ruleset_cache.reqs.advances[i] = effect_list_new();
  }
}

/**
   Free the ruleset cache.  This should be called at the end of the game or
   when the client disconnects from the server.  See ruleset_cache_init.
 */
void ruleset_cache_free()
{
  int i;
  struct effect_list *tracker_list = ruleset_cache.tracker;

  if (tracker_list) {
    effect_list_iterate(tracker_list, peffect)
    {
      requirement_vector_free(&peffect->reqs);
      delete peffect;
    }
    effect_list_iterate_end;
    effect_list_destroy(tracker_list);
    ruleset_cache.tracker = NULL;
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.effects); i++) {
    struct effect_list *plist = ruleset_cache.effects[i];

    if (plist) {
      effect_list_destroy(plist);
      ruleset_cache.effects[i] = NULL;
    }
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.buildings); i++) {
    struct effect_list *plist = ruleset_cache.reqs.buildings[i];

    if (plist) {
      effect_list_destroy(plist);
      ruleset_cache.reqs.buildings[i] = NULL;
    }
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.govs); i++) {
    struct effect_list *plist = ruleset_cache.reqs.govs[i];

    if (plist) {
      effect_list_destroy(plist);
      ruleset_cache.reqs.govs[i] = NULL;
    }
  }

  for (i = 0; i < ARRAY_SIZE(ruleset_cache.reqs.advances); i++) {
    struct effect_list *plist = ruleset_cache.reqs.advances[i];

    if (plist) {
      effect_list_destroy(plist);
      ruleset_cache.reqs.advances[i] = NULL;
    }
  }

  initialized = false;
}

/**
   Get the maximum effect value in this ruleset for the universal
   (that is, the sum of all positive effects clauses that apply specifically
   to this universal -- this can be an overestimate in the case of
   mutually exclusive effects).
   for_uni can be NULL to get max effect value ignoring requirements.
 */
int effect_cumulative_max(enum effect_type type, struct universal *for_uni)
{
  struct effect_list *plist = ruleset_cache.tracker;
  int value = 0;

  if (plist) {
    effect_list_iterate(plist, peffect)
    {
      if (peffect->type == type && peffect->value > 0) {
        if (for_uni == NULL
            || universal_fulfills_requirements(false, &(peffect->reqs),
                                               for_uni)) {
          value += peffect->value;
        }
      }
    }
    effect_list_iterate_end;
  }

  return value;
}

/**
   Get the minimum effect value in this ruleset for the universal
   (that is, the sum of all negative effects clauses that apply specifically
   to this universal -- this can be an overestimate in the case of
   mutually exclusive effects).
   for_uni can be NULL to get min effect value ignoring requirements.
 */
int effect_cumulative_min(enum effect_type type, struct universal *for_uni)
{
  struct effect_list *plist = ruleset_cache.tracker;
  int value = 0;

  if (plist) {
    effect_list_iterate(plist, peffect)
    {
      if (peffect->type == type && peffect->value < 0) {
        if (for_uni == NULL
            || universal_fulfills_requirements(false, &(peffect->reqs),
                                               for_uni)) {
          value += peffect->value;
        }
      }
    }
    effect_list_iterate_end;
  }

  return value;
}

/**
   Return the base value of a given effect that can always be expected from
   just the sources in the list, independent of other factors.
   Adds up all the effects that rely only on these universals; effects that
   have extra conditions are ignored. In effect, 'unis' is a template
   against which effects are matched.
   The first universal in the list is special: effects must have a
   condition that specifically applies to that source to be included
   (may be a superset requirement, e.g. ExtraFlag for VUT_EXTRA source).
 */
int effect_value_from_universals(enum effect_type type,
                                 struct universal *unis, size_t n_unis)
{
  int value = 0;
  struct effect_list *el = get_effects(type);

  effect_list_iterate(el, peffect)
  {
    bool effect_applies = true;
    bool first_source_mentioned = false;

    if (peffect->multiplier) {
      /* Discount any effects with multipliers; we are looking for constant
       * effects */
      continue;
    }

    requirement_vector_iterate(&(peffect->reqs), preq)
    {
      int i;
      bool req_mentioned_a_source = false;

      for (i = 0; effect_applies && i < n_unis; i++) {
        switch (universal_fulfills_requirement(preq, &(unis[i]))) {
        case ITF_NOT_APPLICABLE:
          /* This req not applicable to this source (but might be relevant
           * to another source in our template). Keep looking. */
          break;
        case ITF_NO:
          req_mentioned_a_source = true; // this req matched this source
          if (preq->present) {
            // Requirement contradicts template. Effect doesn't apply.
            effect_applies = false;
          } // but negative req doesn't count for first_source_mentioned
          break;
        case ITF_YES:
          req_mentioned_a_source = true; // this req matched this source
          if (preq->present) {
            if (i == 0) {
              first_source_mentioned = true;
            }
            // keep looking
          } else /* !present */ {
            // Requirement contradicts template. Effect doesn't apply.
            effect_applies = false;
          }
          break;
        }
      }
      if (!req_mentioned_a_source) {
        /* This requirement isn't relevant to any source in our template,
         * so it's an extra condition and the effect should be ignored. */
        effect_applies = false;
      }
      if (!effect_applies) {
        // Already known to be irrelevant, bail out early
        break;
      }
    }
    requirement_vector_iterate_end;

    if (!first_source_mentioned) {
      /* First source not positively mentioned anywhere in requirements,
       * so ignore this effect */
      continue;
    }
    if (effect_applies) {
      value += peffect->value;
    }
  }
  effect_list_iterate_end;

  return value;
}

/**
   Receives a new effect.  This is called by the client when the packet
   arrives.
 */
void recv_ruleset_effect(const struct packet_ruleset_effect *packet)
{
  struct effect *peffect;
  struct multiplier *pmul;
  int i;

  pmul = packet->has_multiplier ? multiplier_by_number(packet->multiplier)
                                : NULL;
  peffect = effect_new(packet->effect_type, packet->effect_value, pmul);

  for (i = 0; i < packet->reqs_count; i++) {
    effect_req_append(peffect, packet->reqs[i]);
  }
  fc_assert(peffect->reqs.size == packet->reqs_count);
}

/**
   Send the ruleset cache data over the network.
 */
void send_ruleset_cache(struct conn_list *dest)
{
  effect_list_iterate(ruleset_cache.tracker, peffect)
  {
    struct packet_ruleset_effect effect_packet;
    int counter;

    effect_packet.effect_type = peffect->type;
    effect_packet.effect_value = peffect->value;
    if (peffect->multiplier) {
      effect_packet.has_multiplier = true;
      effect_packet.multiplier = multiplier_number(peffect->multiplier);
    } else {
      effect_packet.has_multiplier = false;
      effect_packet.multiplier = 0; // arbitrary
    }

    counter = 0;
    requirement_vector_iterate(&(peffect->reqs), req)
    {
      effect_packet.reqs[counter++] = *req;
    }
    requirement_vector_iterate_end;
    effect_packet.reqs_count = counter;

    lsend_packet_ruleset_effect(dest, &effect_packet);
  }
  effect_list_iterate_end;
}

/**
   Returns TRUE if the building has any effect bonuses of the given type.

   Note that this function returns a boolean rather than an integer value
   giving the exact bonus.  Finding the exact bonus requires knowing the
   effect range and may take longer.  This function should only be used
   in situations where the range doesn't matter.
 */
bool building_has_effect(const struct impr_type *pimprove,
                         enum effect_type effect_type)
{
  struct universal source = {
      // just to bamboozle the annoying compiler warning
      .value = {.building =
                    improvement_by_number(improvement_number(pimprove))},
      .kind = VUT_IMPROVEMENT};
  struct effect_list *plist = get_req_source_effects(&source);

  if (!plist) {
    return false;
  }

  effect_list_iterate(plist, peffect)
  {
    if (peffect->type == effect_type) {
      return true;
    }
  }
  effect_list_iterate_end;

  return false;
}

/**
   Return TRUE iff any of the disabling requirements for this effect are
   active, which would prevent it from taking effect.
   (Assumes that any requirement specified in the ruleset with a negative
   sense is an impediment.)
 */
static bool is_effect_prevented(
    const struct player *target_player, const struct player *other_player,
    const struct city *target_city, const struct impr_type *target_building,
    const struct tile *target_tile, const struct unit *target_unit,
    const struct unit_type *target_unittype,
    const struct output_type *target_output,
    const struct specialist *target_specialist, const struct effect *peffect,
    const enum req_problem_type prob_type)
{
  requirement_vector_iterate(&peffect->reqs, preq)
  {
    /* Only check present=FALSE requirements; these will return _FALSE_
     * from is_req_active() if met, and need reversed prob_type */
    if (!preq->present
        && !is_req_active(target_player, other_player, target_city,
                          target_building, target_tile, target_unit,
                          target_unittype, target_output, target_specialist,
                          NULL, preq, REVERSED_RPT(prob_type))) {
      return true;
    }
  }
  requirement_vector_iterate_end;

  return false;
}

/**
   Returns TRUE if a building is replaced. To be replaced, all its effects
   must be made redundant by groups that it is in.
   prob_type CERTAIN or POSSIBLE is answer to function name.
 */
bool is_building_replaced(const struct city *pcity,
                          const struct impr_type *pimprove,
                          const enum req_problem_type prob_type)
{
  struct effect_list *plist;
  struct universal source = {.value = {.building = pimprove},
                             .kind = VUT_IMPROVEMENT};

  plist = get_req_source_effects(&source);

  // A building with no effects and no flags is always redundant!
  if (!plist) {
    return true;
  }

  effect_list_iterate(plist, peffect)
  {
    /* We use TARGET_BUILDING as the lowest common denominator.  Note that
     * the building is its own target - but whether this is actually
     * checked depends on the range of the effect. */
    /* Prob_type is not reversed here. disabled is equal to replaced, not
     * reverse */
    if (!is_effect_prevented(city_owner(pcity), NULL, pcity, pimprove, NULL,
                             NULL, NULL, NULL, NULL, peffect, prob_type)) {
      return false;
    }
  }
  effect_list_iterate_end;

  return true;
}

/**
   Returns the effect bonus of a given type for any target.

   target gives the type of the target
   (player,city,building,tile) give the exact target
   effect_type gives the effect type to be considered

   Returns the effect sources of this type _currently active_.

   The returned vector must be freed (building_vector_free) when the caller
   is done with it.
 */
int get_target_bonus_effects(
    struct effect_list *plist, const struct player *target_player,
    const struct player *other_player, const struct city *target_city,
    const struct impr_type *target_building, const struct tile *target_tile,
    const struct unit *target_unit, const struct unit_type *target_unittype,
    const struct output_type *target_output,
    const struct specialist *target_specialist,
    const struct action *target_action, enum effect_type effect_type)
{
  int bonus = 0;

  // Loop over all effects of this type.
  effect_list_iterate(get_effects(effect_type), peffect)
  {
    // For each effect, see if it is active.
    if (are_reqs_active(target_player, other_player, target_city,
                        target_building, target_tile, target_unit,
                        target_unittype, target_output, target_specialist,
                        target_action, &peffect->reqs, RPT_CERTAIN)) {
      /* This code will add value of effect. If there's multiplier for
       * effect and target_player aren't null, then value is multiplied
       * by player's multiplier factor. */
      if (peffect->multiplier) {
        if (target_player) {
          bonus += (peffect->value
                    * player_multiplier_effect_value(target_player,
                                                     peffect->multiplier))
                   / 100;
        }
      } else {
        bonus += peffect->value;
      }

      if (plist) {
        effect_list_append(plist, peffect);
      }
    }
  }
  effect_list_iterate_end;

  return bonus;
}

/**
   Returns the effect bonus for the whole world.
 */
int get_world_bonus(enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  return get_target_bonus_effects(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, effect_type);
}

/**
   Returns the effect bonus for a player.
 */
int get_player_bonus(const struct player *pplayer,
                     enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  return get_target_bonus_effects(NULL, pplayer, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, effect_type);
}

/**
   Returns the effect bonus at a city.
 */
int get_city_bonus(const struct city *pcity, enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  return get_target_bonus_effects(NULL, city_owner(pcity), NULL, pcity, NULL,
                                  city_tile(pcity), NULL, NULL, NULL, NULL,
                                  NULL, effect_type);
}

/**
   Returns the effect bonus of a specialist in a city.
 */
int get_city_specialist_output_bonus(const struct city *pcity,
                                     const struct specialist *pspecialist,
                                     const struct output_type *poutput,
                                     enum effect_type effect_type)
{
  fc_assert_ret_val(pcity != NULL, 0);
  fc_assert_ret_val(pspecialist != NULL, 0);
  fc_assert_ret_val(poutput != NULL, 0);
  return get_target_bonus_effects(NULL, city_owner(pcity), NULL, pcity, NULL,
                                  NULL, NULL, NULL, poutput, pspecialist,
                                  NULL, effect_type);
}

/**
   Returns the effect bonus at a city tile.
   pcity must be supplied.

   FIXME: this is now used both for tile bonuses, tile-output bonuses,
   and city-output bonuses.  Thus ptile or poutput may be NULL for
   certain callers.  This could be changed by adding 2 new functions to
   the interface but they'd be almost identical and their likely names
   would conflict with functions already in city.c.
   It's also very similar to get_tile_output_bonus(); it should be
   called when the city is mandatory.
 */
int get_city_tile_output_bonus(const struct city *pcity,
                               const struct tile *ptile,
                               const struct output_type *poutput,
                               enum effect_type effect_type)
{
  fc_assert_ret_val(pcity != NULL, 0);
  return get_target_bonus_effects(NULL, city_owner(pcity), NULL, pcity, NULL,
                                  ptile, NULL, NULL, poutput, NULL, NULL,
                                  effect_type);
}

/**
   Returns the effect bonus at a tile for given output type (or NULL for
   output-type-independent bonus).
   If pcity is supplied, it's the bonus for that particular city, otherwise
   it's the player/city-independent bonus (and any city on the tile is
   ignored).
 */
int get_tile_output_bonus(const struct city *pcity, const struct tile *ptile,
                          const struct output_type *poutput,
                          enum effect_type effect_type)
{
  const struct player *pplayer = pcity ? city_owner(pcity) : NULL;

  return get_target_bonus_effects(NULL, pplayer, NULL, pcity, NULL, ptile,
                                  NULL, NULL, poutput, NULL, NULL,
                                  effect_type);
}

/**
   Returns the player effect bonus of an output.
 */
int get_player_output_bonus(const struct player *pplayer,
                            const struct output_type *poutput,
                            enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(pplayer != NULL, 0);
  fc_assert_ret_val(poutput != NULL, 0);
  fc_assert_ret_val(effect_type != EFT_COUNT, 0);
  return get_target_bonus_effects(NULL, pplayer, NULL, NULL, NULL, NULL,
                                  NULL, NULL, poutput, NULL, NULL,
                                  effect_type);
}

/**
   Returns the player effect bonus of an output.
 */
int get_city_output_bonus(const struct city *pcity,
                          const struct output_type *poutput,
                          enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(pcity != NULL, 0);
  fc_assert_ret_val(poutput != NULL, 0);
  fc_assert_ret_val(effect_type != EFT_COUNT, 0);
  return get_target_bonus_effects(NULL, city_owner(pcity), NULL, pcity, NULL,
                                  NULL, NULL, NULL, poutput, NULL, NULL,
                                  effect_type);
}

/**
   Returns the effect bonus at a building.
 */
int get_building_bonus(const struct city *pcity,
                       const struct impr_type *building,
                       enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(NULL != pcity && NULL != building, 0);
  return get_target_bonus_effects(NULL, city_owner(pcity), NULL, pcity,
                                  building, NULL, NULL, NULL, NULL, NULL,
                                  NULL, effect_type);
}

/**
   Returns the effect bonus that applies at a tile for a given unittype.

   For instance with EFT_DEFEND_BONUS the attacker's unittype and the
   defending tile should be passed in.  Slightly counter-intuitive!
   See doc/README.effects to see how the unittype applies for each effect
   here.
 */
int get_unittype_bonus(const struct player *pplayer,
                       const struct tile *ptile,
                       const struct unit_type *punittype,
                       enum effect_type effect_type)
{
  struct city *pcity;

  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(punittype != NULL, 0);

  if (ptile != NULL) {
    pcity = tile_city(ptile);
  } else {
    pcity = NULL;
  }

  return get_target_bonus_effects(NULL, pplayer, NULL, pcity, NULL, ptile,
                                  NULL, punittype, NULL, NULL, NULL,
                                  effect_type);
}

/**
   Returns the effect bonus at a unit
 */
int get_unit_bonus(const struct unit *punit, enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(punit != NULL, 0);
  return get_target_bonus_effects(
      NULL, unit_owner(punit), NULL,
      unit_tile(punit) ? tile_city(unit_tile(punit)) : NULL, NULL,
      unit_tile(punit), punit, unit_type_get(punit), NULL, NULL, NULL,
      effect_type);
}

/**
   Returns the effect bonus at a tile
 */
int get_tile_bonus(const struct tile *ptile, const struct unit *punit,
                   enum effect_type etype)
{
  struct player *pplayer = NULL;
  const struct unit_type *utype = NULL;

  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(ptile != NULL, 0);

  if (punit != NULL) {
    pplayer = unit_owner(punit);
    utype = unit_type_get(punit);
  }

  return get_target_bonus_effects(NULL, pplayer, NULL, tile_city(ptile),
                                  NULL, ptile, punit, utype, NULL, NULL,
                                  NULL, etype);
}

/**
   Returns the effect sources of this type _currently active_ at the player.

   The returned vector must be freed (building_vector_free) when the caller
   is done with it.
 */
int get_player_bonus_effects(struct effect_list *plist,
                             const struct player *pplayer,
                             enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(pplayer != NULL, 0);
  return get_target_bonus_effects(plist, pplayer, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, effect_type);
}

/**
   Returns the effect sources of this type _currently active_ at the city.

   The returned vector must be freed (building_vector_free) when the caller
   is done with it.
 */
int get_city_bonus_effects(struct effect_list *plist,
                           const struct city *pcity,
                           const struct output_type *poutput,
                           enum effect_type effect_type)
{
  if (!initialized) {
    return 0;
  }

  fc_assert_ret_val(pcity != NULL, 0);
  return get_target_bonus_effects(plist, city_owner(pcity), NULL, pcity,
                                  NULL, NULL, NULL, NULL, poutput, NULL,
                                  NULL, effect_type);
}

/**
   Returns the effect bonus the currently-in-construction-item will provide.

   Note this is not called get_current_production_bonus because that would
   be confused with EFT_PROD_BONUS.

   Problem type tells if we need to be CERTAIN about bonus before counting
   it or is POSSIBLE bonus enough.
 */
int get_current_construction_bonus(const struct city *pcity,
                                   enum effect_type effect_type,
                                   const enum req_problem_type prob_type)
{
  if (!initialized) {
    return 0;
  }

  if (VUT_IMPROVEMENT == pcity->production.kind) {
    return get_potential_improvement_bonus(pcity->production.value.building,
                                           pcity, effect_type, prob_type);
  }
  return 0;
}

/**
   Returns the effect bonus the improvement would or does provide if present.

   Problem type tells if we need to be CERTAIN about bonus before counting
   it or is POSSIBLE bonus enough.
 */
int get_potential_improvement_bonus(const struct impr_type *pimprove,
                                    const struct city *pcity,
                                    enum effect_type effect_type,
                                    const enum req_problem_type prob_type)
{
  struct universal source = {.value = {.building = pimprove},
                             .kind = VUT_IMPROVEMENT};
  struct effect_list *plist = get_req_source_effects(&source);

  if (plist) {
    int power = 0;

    effect_list_iterate(plist, peffect)
    {
      bool present = true;
      bool useful = true;

      if (peffect->type != effect_type) {
        continue;
      }

      requirement_vector_iterate(&peffect->reqs, preq)
      {
        if (VUT_IMPROVEMENT == preq->source.kind
            && preq->source.value.building == pimprove) {
          present = preq->present;
          continue;
        }

        if (!is_req_active(city_owner(pcity), NULL, pcity, pimprove, NULL,
                           NULL, NULL, NULL, NULL, NULL, preq, prob_type)) {
          useful = false;
          break;
        }
      }
      requirement_vector_iterate_end;

      if (useful) {
        if (present) {
          power += peffect->value;
        } else {
          power -= peffect->value;
        }
      }
    }
    effect_list_iterate_end;

    return power;
  }
  return 0;
}

/**
   Make user-friendly text for the source.  The text is put into a user
   buffer.
 */
void get_effect_req_text(const struct effect *peffect, char *buf,
                         size_t buf_len)
{
  buf[0] = '\0';

  if (peffect->multiplier) {
    fc_strlcat(buf, multiplier_name_translation(peffect->multiplier),
               buf_len);
  }

  requirement_vector_iterate(&peffect->reqs, preq)
  {
    if (buf[0] != '\0') {
      fc_strlcat(buf, Q_("?req-list-separator:+"), buf_len);
    }
    if (!preq->present) {
      fc_strlcat(buf, Q_("?req-list-separator:not "), buf_len);
    }

    universal_name_translation(&preq->source, buf + qstrlen(buf),
                               buf_len - qstrlen(buf));
  }
  requirement_vector_iterate_end;
}

/**
   Make user-friendly text for an effect list. The text is put into a user
   astring.
 */
QString get_effect_list_req_text(const struct effect_list *plist)
{
  QVector<QString> psv;
  char req_text[512];

  effect_list_iterate(plist, peffect)
  {
    get_effect_req_text(peffect, req_text, sizeof(req_text));
    psv.append(req_text);
  }
  effect_list_iterate_end;
  return strvec_to_and_list(psv);
}

/**
 * Returns a string describing an effect value as interpreted in the context
 * of an effect_type, typically as "value [unit]" for some unit.
 */
QString effect_type_unit_text(effect_type type, int value)
{
  switch (type) {
  case EFT_TECH_PARASITE:
    return QString(PL_("%1 player", "%1 players", value)).arg(value);
  case EFT_AIRLIFT:
  case EFT_UPGRADE_UNIT:
  case EFT_MARTIAL_LAW_MAX:
    // TRANS: Units like warriors and settlers, not miles and kg
    return QString(PL_("%1 unit", "%1 units", value)).arg(value);
  case EFT_OUTPUT_BONUS:
  case EFT_OUTPUT_BONUS_2:
  case EFT_OUTPUT_PER_TILE:
  case EFT_OUTPUT_WASTE_PCT:
  case EFT_GROWTH_FOOD:
  case EFT_HEALTH_PCT:
  case EFT_POLLU_POP_PCT:
  case EFT_POLLU_POP_PCT_2:
  case EFT_POLLU_PROD_PCT:
  case EFT_INCITE_COST_PCT:
  case EFT_SPY_RESISTANT:
  case EFT_VETERAN_COMBAT:
  case EFT_HP_REGEN:
  case EFT_TRADEROUTE_PCT:
  case EFT_DEFEND_BONUS:
  case EFT_CIVIL_WAR_CHANCE:
  case EFT_MIGRATION_PCT:
  case EFT_MAX_RATES:
  case EFT_OUTPUT_WASTE:
  case EFT_UPGRADE_PRICE_PCT:
  case EFT_SHIELD2GOLD_FACTOR:
  case EFT_IMPR_BUILD_COST_PCT:
  case EFT_IMPR_BUY_COST_PCT:
  case EFT_NUKE_IMPROVEMENT_PCT:
  case EFT_UNIT_BUILD_COST_PCT:
  case EFT_UNIT_BUY_COST_PCT:
  case EFT_ENEMY_CITIZEN_UNHAPPY_PCT:
  case EFT_IRRIGATION_PCT:
  case EFT_MINING_PCT:
  case EFT_OUTPUT_TILE_PUNISH_PCT:
  case EFT_UNIT_BRIBE_COST_PCT:
  case EFT_RETIRE_PCT:
  case EFT_ACTION_ODDS_PCT:
  case EFT_OUTPUT_WASTE_BY_REL_DISTANCE: // Effectively % but not really
                                         // useful
  case EFT_SABOTEUR_RESISTANT:
  case EFT_ATTACK_BONUS:
  case EFT_CONQUEST_TECH_PCT:
  case EFT_FORTIFY_DEFENSE_BONUS:
  case EFT_MAPS_STOLEN_PCT:
  case EFT_UNIT_SHIELD_VALUE_PCT:
    // TRANS: per cent
    return QString(PL_("%1%", "%1%", value)).arg(value);
  case EFT_MAX_STOLEN_GOLD_PM:
  case EFT_THIEFS_SHARE_PM:
    // TRANS: per mille
    return QString(PL_("%1‰", "%1‰", value)).arg(value);
  case EFT_FORCE_CONTENT:
  case EFT_MAKE_CONTENT:
  case EFT_MAKE_CONTENT_MIL:
  case EFT_MAKE_HAPPY:
  case EFT_SIZE_ADJ:
  case EFT_MARTIAL_LAW_EACH:
  case EFT_CITY_UNHAPPY_SIZE:
    return QString(PL_("%1 citizen", "%1 citizens", value)).arg(value);
  case EFT_GIVE_IMM_TECH:
    return QString(PL_("%1 tech", "%1 techs", value)).arg(value);
  case EFT_MOVE_BONUS:
  case EFT_ILLEGAL_ACTION_MOVE_COST:
  case EFT_ACTION_SUCCESS_MOVE_COST:
  case EFT_ACTION_SUCCESS_TARGET_MOVE_COST:
    // TRANS: Movement points
    return QString(PL_("%1 MP", "%1 MP", value)).arg(value);
  case EFT_UNIT_RECOVER:
  case EFT_ILLEGAL_ACTION_HP_COST:
    // TRANS: Health/hit points
    return QString(PL_("%1 HP", "%1 HP", value)).arg(value);
  case EFT_UPKEEP_FREE:
    return QString(PL_("%1 building", "%1 buildings", value)).arg(value);
  case EFT_VETERAN_BUILD:
    // TRANS: Veteran levels
    return QString(PL_("%1 level", "%1 levels", value)).arg(value);
  case EFT_CITY_VISION_RADIUS_SQ:
  case EFT_UNIT_VISION_RADIUS_SQ:
  case EFT_CITY_RADIUS_SQ:
    // TRANS: sq. = squared, as in sq. meter
    return QString(PL_("%1 sq. tile", "%1 sq. tiles", value)).arg(value);
  case EFT_GAIN_AI_LOVE:
    // TRANS: "AI love" is what makes the AI friendly
    return QString(PL_("%1 love", "%1 love", value)).arg(value);
  case EFT_TURN_YEARS:
    // TRANS: Year/turn
    return QString(PL_("%1 yr/turn", "%1 yr/turn", value)).arg(value);
  case EFT_EMPIRE_SIZE_BASE:
  case EFT_EMPIRE_SIZE_STEP:
    return QString(PL_("%1 city", "%1 cities", value)).arg(value);
  case EFT_REVOLUTION_UNHAPPINESS:
    return QString(PL_("%1 turn", "%1 turns", value)).arg(value);
  case EFT_TRADE_REVENUE_BONUS: {
    // Complicated arithmetics... and this effect is multiplicative
    int factor = 100 * std::pow(2.0, value / 1000.);
    return QString(PL_("%1%", "%1%", factor)).arg(factor);
  }
  case EFT_OUTPUT_WASTE_BY_DISTANCE:
    return QString(PL_("%1%/tile", "%1%/tile", value)).arg(value);
  case EFT_CITY_BUILD_SLOTS:
    return QString(PL_("%1 slot", "%1 slots", value)).arg(value);
  case EFT_MAX_TRADE_ROUTES:
    // TRANS: Trade routes
    return QString(PL_("%1 route", "%1 routes", value)).arg(value);
  case EFT_COMBAT_ROUNDS:
    // TRANS: Combat rounds
    return QString(PL_("%1 round", "%1 rounds", value)).arg(value);
  case EFT_PERFORMANCE:
  case EFT_HISTORY:
  case EFT_NATION_PERFORMANCE:
  case EFT_NATION_HISTORY:
    return QString(PL_("%1 culture point", "%1 culture points", value))
        .arg(value);
  case EFT_TURN_FRAGMENTS:
    return QString(PL_("%1 fragment", "%1 fragments", value)).arg(value);
  case EFT_UNIT_SLOTS:
    // TRANS: Unit slots
    return QString(PL_("%1 slot", "%1 slots", value)).arg(value);
  case EFT_INFRA_POINTS:
    return QString(PL_("%1 infrastructure point", "%1 infrastructure points",
                       value))
        .arg(value);
  case EFT_ANY_GOVERNMENT:
  case EFT_CAPITAL_CITY:
  case EFT_ENABLE_NUKE:
  case EFT_ENABLE_SPACE:
  case EFT_SPECIALIST_OUTPUT: // FIXME should use the name of the
                              // OutputType...
  case EFT_OUTPUT_ADD_TILE:
  case EFT_OUTPUT_INC_TILE:
  case EFT_HAVE_EMBASSIES:
  case EFT_MAKE_CONTENT_MIL_PER: // FIXME ?
  case EFT_NO_ANARCHY:
  case EFT_NUKE_PROOF:
  case EFT_REVEAL_CITIES:
  case EFT_REVEAL_MAP:
  case EFT_SIZE_UNLIMIT:
  case EFT_SS_STRUCTURAL:
  case EFT_SS_COMPONENT:
  case EFT_SS_MODULE:
  case EFT_UNIT_NO_LOSE_POP:
  case EFT_TECH_UPKEEP_FREE:
  case EFT_NO_UNHAPPY:
  case EFT_SLOW_DOWN_TIMELINE:
  case EFT_RAPTURE_GROW:
  case EFT_HAS_SENATE:
  case EFT_INSPIRE_PARTISANS:
  case EFT_HAPPINESS_TO_GOLD:
  case EFT_FANATICS:
  case EFT_NO_DIPLOMACY:
  case EFT_UNHAPPY_FACTOR:
  case EFT_UPKEEP_FACTOR:
  case EFT_UNIT_UPKEEP_FREE_PER_CITY:
  case EFT_OUTPUT_PENALTY_TILE:
  case EFT_OUTPUT_INC_TILE_CELEBRATE:
  case EFT_VISIBLE_WALLS:
  case EFT_TECH_COST_FACTOR:
  case EFT_TILE_WORKABLE:
  case EFT_CITY_IMAGE:
  case EFT_GOV_CENTER:
  case EFT_NOT_TECH_SOURCE:
  case EFT_VICTORY:
  case EFT_HAVE_CONTACTS:
  case EFT_CASUS_BELLI_CAUGHT:
  case EFT_CASUS_BELLI_SUCCESS:
  case EFT_BORDER_VISION:
  case EFT_STEALINGS_IGNORE:
  case EFT_CASUS_BELLI_COMPLETE:
  case EFT_COUNT:
    return QStringLiteral("%1").arg(value);
  }

  fc_assert_ret_val(false, QString());
}

/**
   Iterate through all the effects in cache, and call callback for each.
   This is currently not very generic implementation, as we have only one
 user; ruleset sanity checking. If any callback returns FALSE, there is no
   further checking and this will return FALSE.
 */
bool iterate_effect_cache(iec_cb cb, void *data)
{
  fc_assert_ret_val(cb != NULL, false);

  effect_list_iterate(ruleset_cache.tracker, peffect)
  {
    if (!cb(peffect, data)) {
      return false;
    }
  }
  effect_list_iterate_end;

  return true;
}
