/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

// utility
#include "fcintl.h"
#include "log.h"
#include "shared.h" // ARRAY_SIZE
#include "support.h"

// common
#include "game.h"
#include "victory.h"

#include "improvement.h"

/**
All the city improvements:
Use improvement_by_number(id) to access the array.
The improvement_types array is now setup in:
   server/ruleset.c (for the server)
   client/packhand.c (for the client)
 */
static struct impr_type improvement_types[B_LAST];

/**
   Initialize building structures.
 */
void improvements_init()
{
  int i;

  /* Can't use improvement_iterate() or improvement_by_number() here
   * because num_impr_types isn't known yet. */
  for (i = 0; i < ARRAY_SIZE(improvement_types); i++) {
    struct impr_type *p = &improvement_types[i];

    p->item_number = i;
    requirement_vector_init(&p->reqs);
    requirement_vector_init(&p->obsolete_by);
    p->ruledit_disabled = false;
  }
}

/**
   Frees the memory associated with this improvement.
 */
static void improvement_free(struct impr_type *p)
{
  delete p->helptext;
  p->helptext = nullptr;
  requirement_vector_free(&p->reqs);
  requirement_vector_free(&p->obsolete_by);
}

/**
  Frees the memory associated with all improvements.
 */
void improvements_free()
{
  improvement_iterate(p) { improvement_free(p); }
  improvement_iterate_end;
}

/**
   Cache features of the improvement
 */
void improvement_feature_cache_init()
{
  improvement_iterate(pimprove)
  {
    pimprove->allows_units = false;
    unit_type_iterate(putype)
    {
      if (requirement_needs_improvement(pimprove, &putype->build_reqs)) {
        pimprove->allows_units = true;
        break;
      }
    }
    unit_type_iterate_end;

    pimprove->allows_extras = false;
    extra_type_iterate(pextra)
    {
      if (requirement_needs_improvement(pimprove, &pextra->reqs)) {
        pimprove->allows_extras = true;
        break;
      }
    }
    extra_type_iterate_end;

    pimprove->prevents_disaster = false;
    disaster_type_iterate(pdis)
    {
      if (!requirement_fulfilled_by_improvement(pimprove, &pdis->reqs)) {
        pimprove->prevents_disaster = true;
        break;
      }
    }
    disaster_type_iterate_end;

    pimprove->protects_vs_actions = false;
    action_enablers_iterate(act)
    {
      if (!requirement_fulfilled_by_improvement(pimprove,
                                                &act->target_reqs)) {
        pimprove->protects_vs_actions = true;
        break;
      }
    }
    action_enablers_iterate_end;
  }
  improvement_iterate_end;
}

/**
   Return the first item of improvements.
 */
struct impr_type *improvement_array_first()
{
  if (game.control.num_impr_types > 0) {
    return improvement_types;
  }
  return nullptr;
}

/**
   Return the last item of improvements.
 */
const struct impr_type *improvement_array_last()
{
  if (game.control.num_impr_types > 0) {
    return &improvement_types[game.control.num_impr_types - 1];
  }
  return nullptr;
}

/**
   Return the number of improvements.
 */
Impr_type_id improvement_count() { return game.control.num_impr_types; }

/**
   Return the improvement index.

   Currently same as improvement_number(), paired with improvement_count()
   indicates use as an array index.
 */
Impr_type_id improvement_index(const struct impr_type *pimprove)
{
  fc_assert_ret_val(nullptr != pimprove, 0);
  return pimprove - improvement_types;
}

/**
   Return the improvement index.
 */
Impr_type_id improvement_number(const struct impr_type *pimprove)
{
  fc_assert_ret_val(nullptr != pimprove, 0);
  return pimprove->item_number;
}

/**
   Returns the improvement type for the given index/ID.  Returns nullptr for
   an out-of-range index.
 */
struct impr_type *improvement_by_number(const Impr_type_id id)
{
  if (id < 0 || id >= improvement_count()) {
    return nullptr;
  }
  return &improvement_types[id];
}

/**
   Returns pointer when the improvement_type "exists" in this game,
   returns nullptr otherwise.

   An improvement_type doesn't exist for any of:
    - the improvement_type has been flagged as removed by setting its
      tech_req to A_LAST; [was not in current 2007-07-27]
    - it is a space part, and the spacerace is not enabled.
 */
const struct impr_type *valid_improvement(const struct impr_type *pimprove)
{
  if (nullptr == pimprove) {
    return nullptr;
  }

  if (!victory_enabled(VC_SPACERACE)
      && (building_has_effect(pimprove, EFT_SS_STRUCTURAL)
          || building_has_effect(pimprove, EFT_SS_COMPONENT)
          || building_has_effect(pimprove, EFT_SS_MODULE))) {
    // This assumes that space parts don't have any other effects.
    return nullptr;
  }

  return pimprove;
}

/**
   Return the (translated) name of the given improvement.
   You don't have to free the return pointer.
 */
const char *improvement_name_translation(const struct impr_type *pimprove)
{
  return name_translation_get(&pimprove->name);
}

/**
   Return the (untranslated) rule name of the improvement.
   You don't have to free the return pointer.
 */
const char *improvement_rule_name(const struct impr_type *pimprove)
{
  return rule_name_get(&pimprove->name);
}

/**
   Returns the base number of shields it takes to build this improvement.
   This one does not take city specific bonuses in to account.
 */
int impr_base_build_shield_cost(const struct impr_type *pimprove)
{
  int base = pimprove->build_cost;

  return MAX(base * game.info.shieldbox / 100, 1);
}

/**
   Returns estimate of the number of shields it takes to build this
 improvement. pplayer and ptile can be nullptr, but that might reduce quality
 of the estimate.
 */
int impr_estimate_build_shield_cost(const struct player *pplayer,
                                    const struct tile *ptile,
                                    const struct impr_type *pimprove)
{
  int base = pimprove->build_cost
             * (100
                + get_target_bonus_effects(nullptr, pplayer, nullptr,
                                           nullptr, pimprove, ptile, nullptr,
                                           nullptr, nullptr, nullptr,
                                           nullptr, EFT_IMPR_BUILD_COST_PCT))
             / 100;

  return MAX(base * game.info.shieldbox / 100, 1);
}

/**
   Returns the number of shields it takes to build this improvement.
 */
int impr_build_shield_cost(const struct city *pcity,
                           const struct impr_type *pimprove)
{
  int base =
      pimprove->build_cost
      * (100 + get_building_bonus(pcity, pimprove, EFT_IMPR_BUILD_COST_PCT))
      / 100;

  return MAX(base * game.info.shieldbox / 100, 1);
}

/**
   Returns the amount of gold it takes to rush this improvement.
 */
int impr_buy_gold_cost(const struct city *pcity,
                       const struct impr_type *pimprove,
                       int shields_in_stock)
{
  int cost = 0;
  const int missing =
      impr_build_shield_cost(pcity, pimprove) - shields_in_stock;

  if (improvement_has_flag(pimprove, IF_GOLD)) {
    // Can't buy capitalization.
    return 0;
  }

  if (missing > 0) {
    cost = 2 * missing;
  }

  if (shields_in_stock == 0) {
    cost *= 2;
  }

  cost = cost
         * (100 + get_building_bonus(pcity, pimprove, EFT_IMPR_BUY_COST_PCT))
         / 100;

  return cost;
}

/**
   Returns the amount of gold received when this improvement is sold.
 */
int impr_sell_gold(const struct impr_type *pimprove)
{
  return MAX(pimprove->build_cost * game.info.shieldbox / 100, 1);
}

/**
   Returns whether improvement is some kind of wonder. Both great wonders
   and small wonders count.
 */
bool is_wonder(const struct impr_type *pimprove)
{
  return (is_great_wonder(pimprove) || is_small_wonder(pimprove));
}

/**
   Does a linear search of improvement_types[].name.translated
   Returns nullptr when none match.
 */
struct impr_type *improvement_by_translated_name(const char *name)
{
  improvement_iterate(pimprove)
  {
    if (0 == strcmp(improvement_name_translation(pimprove), name)) {
      return pimprove;
    }
  }
  improvement_iterate_end;

  return nullptr;
}

/**
   Does a linear search of improvement_types[].name.vernacular
   Returns nullptr when none match.
 */
struct impr_type *improvement_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  improvement_iterate(pimprove)
  {
    if (0 == fc_strcasecmp(improvement_rule_name(pimprove), qname)) {
      return pimprove;
    }
  }
  improvement_iterate_end;

  return nullptr;
}

/**
   Return TRUE if the impr has this flag otherwise FALSE
 */
bool improvement_has_flag(const struct impr_type *pimprove,
                          enum impr_flag_id flag)
{
  fc_assert_ret_val(impr_flag_id_is_valid(flag), false);
  return BV_ISSET(pimprove->flags, flag);
}

/**
   Return TRUE if the improvement should be visible to others without spying
 */
bool is_improvement_visible(const struct impr_type *pimprove)
{
  return (is_wonder(pimprove)
          || improvement_has_flag(pimprove, IF_VISIBLE_BY_OTHERS));
}

/**
   Return TRUE if the improvement can ever go obsolete.
   Can be used for buildings or wonders.
 */
bool can_improvement_go_obsolete(const struct impr_type *pimprove)
{
  return requirement_vector_size(&pimprove->obsolete_by) > 0;
}

/**
   Returns TRUE if the improvement or wonder is obsolete
 */
bool improvement_obsolete(const struct player *pplayer,
                          const struct impr_type *pimprove,
                          const struct city *pcity)
{
  struct tile *ptile = nullptr;

  if (pcity != nullptr) {
    ptile = city_tile(pcity);
  }

  requirement_vector_iterate(&pimprove->obsolete_by, preq)
  {
    if (is_req_active(pplayer, nullptr, pcity, pimprove, ptile, nullptr,
                      nullptr, nullptr, nullptr, nullptr, preq,
                      RPT_CERTAIN)) {
      return true;
    }
  }
  requirement_vector_iterate_end;

  return false;
}

/**
   Returns TRUE iff improvement provides units buildable in city
 */
static bool impr_provides_buildable_units(const struct city *pcity,
                                          const struct impr_type *pimprove)
{
  // Fast check
  if (!pimprove->allows_units) {
    return false;
  }

  unit_type_iterate(ut)
  {
    if (requirement_needs_improvement(pimprove, &ut->build_reqs)
        && can_city_build_unit_now(pcity, ut)) {
      return true;
    }
  }
  unit_type_iterate_end;

  return false;
}

/**
   Returns TRUE iff improvement provides extras buildable in city
 */
static bool impr_provides_buildable_extras(const struct city *pcity,
                                           const struct impr_type *pimprove)
{
  // Fast check
  if (!pimprove->allows_extras) {
    return false;
  }

  extra_type_iterate(pextra)
  {
    if (requirement_needs_improvement(pimprove, &pextra->reqs)) {
      city_tile_iterate(city_map_radius_sq_get(pcity), city_tile(pcity),
                        ptile)
      {
        if (player_can_build_extra(pextra, city_owner(pcity), ptile)) {
          return true;
        }
      }
      city_tile_iterate_end;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Returns TRUE iff improvement prevents a disaster in city
 */
static bool impr_prevents_disaster(const struct city *pcity,
                                   const struct impr_type *pimprove)
{
  // Fast check
  if (!pimprove->prevents_disaster) {
    return false;
  }

  disaster_type_iterate(pdis)
  {
    if (!requirement_fulfilled_by_improvement(pimprove, &pdis->reqs)
        && !can_disaster_happen(pdis, pcity)) {
      return true;
    }
  }
  disaster_type_iterate_end;

  return false;
}

/**
   Returns TRUE iff improvement protects against an action on the city
   FIXME: This is prone to false positives: for example, if one requires
          a special tech or unit to perform an action, and no other player
          has or can gain that tech or unit, protection is still claimed.
 */
static bool impr_protects_vs_actions(const struct city *pcity,
                                     const struct impr_type *pimprove)
{
  // Fast check
  if (!pimprove->protects_vs_actions) {
    return false;
  }

  action_enablers_iterate(act)
  {
    if (!requirement_fulfilled_by_improvement(pimprove, &act->target_reqs)
        && !is_action_possible_on_city(act->action, nullptr, pcity)) {
      return true;
    }
  }
  action_enablers_iterate_end;

  return false;
}

/**
   Check if an improvement has side effects for a city.  Side effects
   are any benefits that accrue that are not tracked by the effects
   system.

   Note that this function will always return FALSE if the improvement does
   not currently provide a benefit to the city (for example, if the
 improvement has not yet been built, or another city benefits from the
 improvement in this city (i.e. Wonders)).
 */
static bool improvement_has_side_effects(const struct city *pcity,
                                         const struct impr_type *pimprove)
{
  /* FIXME: There should probably also be a test as to whether
   *        the improvement *enables* an action (somewhere else),
   *        but this is hard to determine at city scope. */

  return (impr_provides_buildable_units(pcity, pimprove)
          || impr_provides_buildable_extras(pcity, pimprove)
          || impr_prevents_disaster(pcity, pimprove)
          || impr_protects_vs_actions(pcity, pimprove));
}

/**
   Returns TRUE iff improvement provides some effect (good or bad).
 */
static bool improvement_has_effects(const struct city *pcity,
                                    const struct impr_type *pimprove)
{
  struct universal source = {.value = {.building = pimprove},
                             .kind = VUT_IMPROVEMENT};
  struct effect_list *plist = get_req_source_effects(&source);

  if (!plist || improvement_obsolete(city_owner(pcity), pimprove, pcity)) {
    return false;
  }

  effect_list_iterate(plist, peffect)
  {
    if (0
        != get_potential_improvement_bonus(pimprove, pcity, peffect->type,
                                           RPT_CERTAIN)) {
      return true;
    }
  }
  effect_list_iterate_end;

  return false;
}

/**
   Returns TRUE if an improvement in a city is productive, in some way.

   Note that unproductive improvements may become productive later, if
   appropriate conditions are met (e.g. a special building that isn't
   producing units under the current government might under another).
 */
bool is_improvement_productive(const struct city *pcity,
                               const struct impr_type *pimprove)
{
  return (!improvement_obsolete(city_owner(pcity), pimprove, pcity)
          && (improvement_has_flag(pimprove, IF_GOLD)
              || improvement_has_side_effects(pcity, pimprove)
              || improvement_has_effects(pcity, pimprove)));
}

/**
   Returns TRUE if an improvement in a city is redundant, that is, the
   city wouldn't lose anything by losing the improvement, or gain anything
   by building it. This means:
    - all of its effects (if any) are provided by other means, or it's
      obsolete (and thus assumed to have no effect); and
    - it's not enabling the city to build some kind of units; and
    - it's not Coinage (IF_GOLD).
   (Note that it's not impossible that this improvement could become useful
   if circumstances changed, say if a new government enabled the building
   of its special units.)
 */
bool is_improvement_redundant(const struct city *pcity,
                              const struct impr_type *pimprove)
{
  // A capitalization production is never redundant.
  if (improvement_has_flag(pimprove, IF_GOLD)) {
    return false;
  }

  // If an improvement has side effects, don't claim it's redundant.
  if (improvement_has_side_effects(pcity, pimprove)) {
    return false;
  }

  /* Otherwise, it's redundant if its effects are available by other means,
   * or if it's an obsolete wonder (great or small). */
  return is_building_replaced(pcity, pimprove, RPT_CERTAIN)
         || improvement_obsolete(city_owner(pcity), pimprove, pcity);
}

/**
    Whether player can build given building somewhere, ignoring whether it
    is obsolete.
 */
bool can_player_build_improvement_direct(const struct player *p,
                                         const struct impr_type *pimprove)
{
  bool space_part = false;

  if (!valid_improvement(pimprove)) {
    return false;
  }

  requirement_vector_iterate(&pimprove->reqs, preq)
  {
    if (preq->range >= REQ_RANGE_PLAYER
        && !is_req_active(p, nullptr, nullptr, nullptr, nullptr, nullptr,
                          nullptr, nullptr, nullptr, nullptr, preq,
                          RPT_CERTAIN)) {
      return false;
    }
  }
  requirement_vector_iterate_end;

  /* Check for space part construction.  This assumes that space parts have
   * no other effects. */
  if (building_has_effect(pimprove, EFT_SS_STRUCTURAL)) {
    space_part = true;
    if (p->spaceship.structurals >= NUM_SS_STRUCTURALS) {
      return false;
    }
  }
  if (building_has_effect(pimprove, EFT_SS_COMPONENT)) {
    space_part = true;
    if (p->spaceship.components >= NUM_SS_COMPONENTS) {
      return false;
    }
  }
  if (building_has_effect(pimprove, EFT_SS_MODULE)) {
    space_part = true;
    if (p->spaceship.modules >= NUM_SS_MODULES) {
      return false;
    }
  }
  if (space_part
      && (get_player_bonus(p, EFT_ENABLE_SPACE) <= 0
          || p->spaceship.state >= SSHIP_LAUNCHED)) {
    return false;
  }

  if (is_great_wonder(pimprove)) {
    // Can't build wonder if already built
    if (!great_wonder_is_available(pimprove)) {
      return false;
    }
  }

  return true;
}

/**
   Whether player can build given building somewhere immediately.
   Returns FALSE if building is obsolete.
 */
bool can_player_build_improvement_now(const struct player *p,
                                      struct impr_type *pimprove)
{
  if (!can_player_build_improvement_direct(p, pimprove)) {
    return false;
  }
  if (improvement_obsolete(p, pimprove, nullptr)) {
    return false;
  }
  return true;
}

/**
   Whether player can _eventually_ build given building somewhere -- i.e.,
   returns TRUE if building is available with current tech OR will be
   available with future tech. Returns FALSE if building is obsolete.
 */
bool can_player_build_improvement_later(const struct player *p,
                                        const struct impr_type *pimprove)
{
  if (!valid_improvement(pimprove)) {
    return false;
  }
  if (improvement_obsolete(p, pimprove, nullptr)) {
    return false;
  }
  if (is_great_wonder(pimprove) && !great_wonder_is_available(pimprove)) {
    // Can't build wonder if already built
    return false;
  }

  /* Check for requirements that aren't met and that are unchanging (so
   * they can never be met). */
  requirement_vector_iterate(&pimprove->reqs, preq)
  {
    if (preq->range >= REQ_RANGE_PLAYER && is_req_unchanging(preq)
        && !is_req_active(p, nullptr, nullptr, nullptr, nullptr, nullptr,
                          nullptr, nullptr, nullptr, nullptr, preq,
                          RPT_POSSIBLE)) {
      return false;
    }
  }
  requirement_vector_iterate_end;
  /* FIXME: should check some "unchanging" reqs here - like if there's
   * a nation requirement, we can go ahead and check it now. */

  return true;
}

/**
   Is this building a great wonder?
 */
bool is_great_wonder(const struct impr_type *pimprove)
{
  return (pimprove->genus == IG_GREAT_WONDER);
}

/**
   Is this building a small wonder?
 */
bool is_small_wonder(const struct impr_type *pimprove)
{
  return (pimprove->genus == IG_SMALL_WONDER);
}

/**
   Is this building a regular improvement?
 */
bool is_improvement(const struct impr_type *pimprove)
{
  return (pimprove->genus == IG_IMPROVEMENT);
}

/**
   Returns TRUE if this is a "special" improvement. For example, spaceship
   parts and coinage in the default ruleset are considered special.
 */
bool is_special_improvement(const struct impr_type *pimprove)
{
  return pimprove->genus == IG_SPECIAL;
}

/**
   Build a wonder in the city.
 */
void wonder_built(const struct city *pcity, const struct impr_type *pimprove)
{
  struct player *pplayer;
  int windex = improvement_number(pimprove);

  fc_assert_ret(nullptr != pcity);
  fc_assert_ret(is_wonder(pimprove));

  pplayer = city_owner(pcity);
  pplayer->wonders[windex] = pcity->id;

  /* Build turn is set with wonder_set_build_turn() only when
   * actually building it in-game. But this is called also when
   * loading savegames etc., so initialize to something known. */
  pplayer->wonder_build_turn[windex] = -1;

  if (is_great_wonder(pimprove)) {
    game.info.great_wonder_owners[windex] = player_number(pplayer);
  }
}

/**
   Remove a wonder from a city and destroy it if it's a great wonder.  To
   transfer a great wonder, use great_wonder_transfer.
 */
void wonder_destroyed(const struct city *pcity,
                      const struct impr_type *pimprove)
{
  struct player *pplayer;
  int windex = improvement_number(pimprove);

  fc_assert_ret(nullptr != pcity);
  fc_assert_ret(is_wonder(pimprove));

  pplayer = city_owner(pcity);
  fc_assert_ret(pplayer->wonders[windex] == pcity->id);
  pplayer->wonders[windex] = WONDER_LOST;

  if (is_great_wonder(pimprove)) {
    fc_assert_ret(game.info.great_wonder_owners[windex]
                  == player_number(pplayer));
    game.info.great_wonder_owners[windex] = WONDER_DESTROYED;
  }
}

/**
   Returns whether the player has lost this wonder after having owned it
   (small or great).
 */
bool wonder_is_lost(const struct player *pplayer,
                    const struct impr_type *pimprove)
{
  fc_assert_ret_val(nullptr != pplayer, false);
  fc_assert_ret_val(is_wonder(pimprove), false);

  return pplayer->wonders[improvement_index(pimprove)] == WONDER_LOST;
}

/**
   Returns whether the player is currently in possession of this wonder
   (small or great)  and it hasn't been just built this turn.
 */
bool wonder_is_built(const struct player *pplayer,
                     const struct impr_type *pimprove)
{
  int windex = improvement_index(pimprove);

  fc_assert_ret_val(nullptr != pplayer, false);
  fc_assert_ret_val(is_wonder(pimprove), false);

  /* New city turn: Wonders don't take effect until the next
   * turn after building */

  if (!WONDER_BUILT(pplayer->wonders[windex])) {
    return false;
  }

  return (pplayer->wonder_build_turn[windex] != game.info.turn);
}

/**
   Get the world city with this wonder (small or great).  This doesn't
   always succeed on the client side, and even when it does, it may
   return an "invisible" city whose members are unexpectedly nullptr;
   take care.
 */
struct city *city_from_wonder(const struct player *pplayer,
                              const struct impr_type *pimprove)
{
  int city_id = pplayer->wonders[improvement_index(pimprove)];

  fc_assert_ret_val(nullptr != pplayer, nullptr);
  fc_assert_ret_val(is_wonder(pimprove), nullptr);

  if (!WONDER_BUILT(city_id)) {
    return nullptr;
  }

#ifdef FREECIV_DEBUG
  if (is_server()) {
    // On client side, this info is not always known.
    struct city *pcity = player_city_by_number(pplayer, city_id);

    if (nullptr == pcity) {
      qCritical("Player %s (nb %d) has outdated wonder info for "
                "%s (nb %d), it points to city nb %d.",
                player_name(pplayer), player_number(pplayer),
                improvement_rule_name(pimprove),
                improvement_number(pimprove), city_id);
    } else if (!city_has_building(pcity, pimprove)) {
      qCritical("Player %s (nb %d) has outdated wonder info for "
                "%s (nb %d), the city %s (nb %d) doesn't have this wonder.",
                player_name(pplayer), player_number(pplayer),
                improvement_rule_name(pimprove),
                improvement_number(pimprove), city_name_get(pcity),
                pcity->id);
      return nullptr;
    }

    return pcity;
  }
#endif // FREECIV_DEBUG

  return player_city_by_number(pplayer, city_id);
}

/**
   Returns whether this wonder is currently built.
 */
bool great_wonder_is_built(const struct impr_type *pimprove)
{
  int windex = improvement_index(pimprove);
  int owner;
  fc_assert_ret_val(is_great_wonder(pimprove), false);

  owner = game.info.great_wonder_owners[windex];
  /* call wonder_is_built() to check the build turn */
  return (WONDER_OWNED(owner)
          && wonder_is_built(player_by_number(owner), pimprove));
}

/**
   Returns whether this wonder has been destroyed.
 */
bool great_wonder_is_destroyed(const struct impr_type *pimprove)
{
  fc_assert_ret_val(is_great_wonder(pimprove), false);

  return (WONDER_DESTROYED
          == game.info.great_wonder_owners[improvement_index(pimprove)]);
}

/**
   Returns whether this wonder can be currently built.
 */
bool great_wonder_is_available(const struct impr_type *pimprove)
{
  fc_assert_ret_val(is_great_wonder(pimprove), false);

  return (WONDER_NOT_OWNED
          == game.info.great_wonder_owners[improvement_index(pimprove)]);
}

/**
   Get the world city with this great wonder.  This doesn't always success
   on the client side.
 */
struct city *city_from_great_wonder(const struct impr_type *pimprove)
{
  int player_id = game.info.great_wonder_owners[improvement_index(pimprove)];

  fc_assert_ret_val(is_great_wonder(pimprove), nullptr);

  if (WONDER_OWNED(player_id)) {
#ifdef FREECIV_DEBUG
    const struct player *pplayer = player_by_number(player_id);
    struct city *pcity = city_from_wonder(pplayer, pimprove);

    if (is_server() && nullptr == pcity) {
      qCritical("Game has outdated wonder info for %s (nb %d), "
                "the player %s (nb %d) doesn't have this wonder.",
                improvement_rule_name(pimprove),
                improvement_number(pimprove), player_name(pplayer),
                player_number(pplayer));
    }

    return pcity;
#else
    return city_from_wonder(player_by_number(player_id), pimprove);
#endif // FREECIV_DEBUG
  } else {
    return nullptr;
  }
}

/**
   Get the player owning this small wonder.  This doesn't always success on
   the client side.
 */
struct player *great_wonder_owner(const struct impr_type *pimprove)
{
  int player_id = game.info.great_wonder_owners[improvement_index(pimprove)];

  fc_assert_ret_val(is_great_wonder(pimprove), nullptr);

  if (WONDER_OWNED(player_id)) {
    return player_by_number(player_id);
  } else {
    return nullptr;
  }
}

/**
   Get the player city with this small wonder.
 */
struct city *city_from_small_wonder(const struct player *pplayer,
                                    const struct impr_type *pimprove)
{
  fc_assert_ret_val(is_small_wonder(pimprove), nullptr);

  if (nullptr == pplayer) {
    return nullptr; // Used in some places in the client.
  } else {
    return city_from_wonder(pplayer, pimprove);
  }
}

/**
   Return TRUE iff the improvement can be sold.
 */
bool can_sell_building(const struct impr_type *pimprove)
{
  return (valid_improvement(pimprove) && is_improvement(pimprove));
}

/**
   Return TRUE iff the city can sell the given improvement.
 */
bool can_city_sell_building(const struct city *pcity,
                            const struct impr_type *pimprove)
{
  return (city_has_building(pcity, pimprove) && is_improvement(pimprove));
}

/**
   Return TRUE iff the player can sell the given improvement from city.
   If pimprove is nullptr, returns iff city could sell some building type
   (this does not check if such building is in this city)
 */
enum test_result
test_player_sell_building_now(struct player *pplayer, struct city *pcity,
                              const struct impr_type *pimprove)
{
  // Check if player can sell anything from this city
  if (pcity->owner != pplayer) {
    return TR_OTHER_FAILURE;
  }

  if (pcity->did_sell) {
    return TR_ALREADY_SOLD;
  }

  // Check if particular building can be solt
  if (pimprove != nullptr && !can_city_sell_building(pcity, pimprove)) {
    return TR_OTHER_FAILURE;
  }

  return TR_SUCCESS;
}

/**
   Try to find a sensible replacement building, based on other buildings
   that may have caused this one to become obsolete.
 */
const struct impr_type *
improvement_replacement(const struct impr_type *pimprove)
{
  requirement_vector_iterate(&pimprove->obsolete_by, pobs)
  {
    if (pobs->source.kind == VUT_IMPROVEMENT && pobs->present) {
      return pobs->source.value.building;
    }
  }
  requirement_vector_iterate_end;

  return nullptr;
}
