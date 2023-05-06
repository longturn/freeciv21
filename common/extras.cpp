/*
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
 */

// utility
#include "fcintl.h"
#include "rand.h"

// common
#include "base.h"
#include "game.h"
#include "map.h"
#include "research.h"
#include "road.h"

#include "extras.h"

static struct extra_type extras[MAX_EXTRA_TYPES];

static struct user_flag user_extra_flags[MAX_NUM_USER_EXTRA_FLAGS];

static struct extra_type_list *caused_by[EC_LAST];
static struct extra_type_list *removed_by[ERM_COUNT];
static struct extra_type_list *unit_hidden;

/**
   Initialize extras structures.
 */
void extras_init()
{
  int i;

  for (i = 0; i < EC_LAST; i++) {
    caused_by[i] = extra_type_list_new();
  }
  for (i = 0; i < ERM_COUNT; i++) {
    removed_by[i] = extra_type_list_new();
  }
  unit_hidden = extra_type_list_new();

  for (i = 0; i < MAX_EXTRA_TYPES; i++) {
    requirement_vector_init(&(extras[i].reqs));
    requirement_vector_init(&(extras[i].rmreqs));
    requirement_vector_init(&(extras[i].appearance_reqs));
    requirement_vector_init(&(extras[i].disappearance_reqs));
    extras[i].id = i;
    extras[i].hiders = nullptr;
    extras[i].bridged = nullptr;
    extras[i].data.special_idx = -1;
    extras[i].data.base = nullptr;
    extras[i].data.road = nullptr;
    extras[i].data.resource = nullptr;
    extras[i].causes = 0;
    extras[i].rmcauses = 0;
    extras[i].helptext = nullptr;
    extras[i].ruledit_disabled = false;
    extras[i].visibility_req = A_NONE;
  }
}

/**
   Free the memory associated with extras
 */
void extras_free()
{
  int i;

  base_types_free();
  road_types_free();
  resource_types_free();

  for (i = 0; i < game.control.num_extra_types; i++) {
    delete extras[i].data.base;
    delete extras[i].data.road;
    delete extras[i].data.resource;
    extras[i].data.base = nullptr;
    extras[i].data.road = nullptr;
    extras[i].data.resource = nullptr;
  }

  for (i = 0; i < EC_LAST; i++) {
    extra_type_list_destroy(caused_by[i]);
    caused_by[i] = nullptr;
  }

  for (i = 0; i < ERM_COUNT; i++) {
    extra_type_list_destroy(removed_by[i]);
    removed_by[i] = nullptr;
  }

  extra_type_list_destroy(unit_hidden);
  unit_hidden = nullptr;

  for (i = 0; i < MAX_EXTRA_TYPES; i++) {
    requirement_vector_free(&(extras[i].reqs));
    requirement_vector_free(&(extras[i].rmreqs));
    requirement_vector_free(&(extras[i].appearance_reqs));
    requirement_vector_free(&(extras[i].disappearance_reqs));
    delete extras[i].helptext;
    extras[i].helptext = nullptr;
  }

  extra_type_iterate(pextra)
  {
    if (pextra->hiders != nullptr) {
      extra_type_list_destroy(pextra->hiders);
      pextra->hiders = nullptr;
    }
    if (pextra->bridged != nullptr) {
      extra_type_list_destroy(pextra->bridged);
      pextra->bridged = nullptr;
    }
  }
  extra_type_iterate_end;
}

/**
   Return the number of extra_types.
 */
int extra_count() { return game.control.num_extra_types; }

/**
   Return the extra id.
 */
int extra_number(const struct extra_type *pextra)
{
  fc_assert_ret_val(nullptr != pextra, 0);

  return pextra->id;
}

#ifndef extra_index
/**
   Return the extra index.
 */
int extra_index(const struct extra_type *pextra)
{
  fc_assert_ret_val(nullptr != pextra, 0);

  return pextra - extras;
}
#endif // extra_index

/**
   Return extras type of given id.
 */
struct extra_type *extra_by_number(int id)
{
  fc_assert_ret_val(id >= 0 && id < MAX_EXTRA_TYPES, nullptr);

  return &extras[id];
}

/**
   Return the (translated) name of the extra type.
   You don't have to free the return pointer.
 */
const char *extra_name_translation(const struct extra_type *pextra)
{
  return name_translation_get(&pextra->name);
}

/**
   Return the (untranslated) rule name of the extra type.
   You don't have to free the return pointer.
 */
const char *extra_rule_name(const struct extra_type *pextra)
{
  return rule_name_get(&pextra->name);
}

/**
   Returns extra type matching rule name or nullptr if there is no extra type
   with such name.
 */
struct extra_type *extra_type_by_rule_name(const char *name)
{
  const char *qs;

  if (name == nullptr) {
    return nullptr;
  }

  qs = Qn_(name);

  extra_type_iterate(pextra)
  {
    if (!fc_strcasecmp(extra_rule_name(pextra), qs)) {
      return pextra;
    }
  }
  extra_type_iterate_end;

  return nullptr;
}

/**
   Returns extra type matching the translated name, or nullptr if there is no
   extra type with that name.
 */
struct extra_type *extra_type_by_translated_name(const char *name)
{
  extra_type_iterate(pextra)
  {
    if (0 == strcmp(extra_name_translation(pextra), name)) {
      return pextra;
    }
  }
  extra_type_iterate_end;

  return nullptr;
}

/**
   Returns extra type for given cause.
 */
struct extra_type_list *extra_type_list_by_cause(enum extra_cause cause)
{
  fc_assert(cause < EC_LAST);

  return caused_by[cause];
}

/**
   Returns extra types that hide units.
 */
struct extra_type_list *extra_type_list_of_unit_hiders()
{
  return unit_hidden;
}

/**
   Return random extra type for given cause that is native to the tile.
 */
struct extra_type *rand_extra_for_tile(struct tile *ptile,
                                       enum extra_cause cause,
                                       bool generated)
{
  struct extra_type_list *full_list = extra_type_list_by_cause(cause);
  struct extra_type_list *potential = extra_type_list_new();
  int options;
  struct extra_type *selected = nullptr;

  extra_type_list_iterate(full_list, pextra)
  {
    if ((!generated || pextra->generated)
        && is_native_tile_to_extra(pextra, ptile)) {
      extra_type_list_append(potential, pextra);
    }
  }
  extra_type_list_iterate_end;

  options = extra_type_list_size(potential);

  if (options > 0) {
    selected = extra_type_list_get(potential, fc_rand(options));
  }

  extra_type_list_destroy(potential);

  return selected;
}

/**
   Add extra type to list of extra caused by given cause.
 */
void extra_to_caused_by_list(struct extra_type *pextra,
                             enum extra_cause cause)
{
  fc_assert(cause < EC_LAST);

  extra_type_list_append(caused_by[cause], pextra);
}

/**
   Returns extra type for given rmcause.
 */
struct extra_type_list *
extra_type_list_by_rmcause(enum extra_rmcause rmcause)
{
  fc_assert(rmcause < ERM_COUNT);

  return removed_by[rmcause];
}

/**
   Add extra type to list of extra removed by given cause.
 */
void extra_to_removed_by_list(struct extra_type *pextra,
                              enum extra_rmcause rmcause)
{
  fc_assert(rmcause < ERM_COUNT);

  extra_type_list_append(removed_by[rmcause], pextra);
}

/**
   Is given cause one of the removal causes for the given extra?
 */
bool is_extra_removed_by(const struct extra_type *pextra,
                         enum extra_rmcause rmcause)
{
  return (pextra->rmcauses & (1 << rmcause));
}

/**
   Is there extra of the given type cardinally near tile?
   (Does not check ptile itself.)
 */
bool is_extra_card_near(const struct tile *ptile,
                        const struct extra_type *pextra)
{
  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    if (tile_has_extra(adjc_tile, pextra)) {
      return true;
    }
  }
  cardinal_adjc_iterate_end;

  return false;
}

/**
   Is there extra of the given type near tile?
   (Does not check ptile itself.)
 */
bool is_extra_near_tile(const struct tile *ptile,
                        const struct extra_type *pextra)
{
  adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    if (tile_has_extra(adjc_tile, pextra)) {
      return true;
    }
  }
  adjc_iterate_end;

  return false;
}

/**
   Tells if extra can build to tile if all other requirements are met.
 */
bool extra_can_be_built(const struct extra_type *pextra,
                        const struct tile *ptile)
{
  if (!pextra->buildable) {
    // Extra type not buildable
    return false;
  }

  if (tile_has_extra(ptile, pextra)) {
    // Extra exist already
    return false;
  }

  return true;
}

/**
   Tells if player can build extra to tile with suitable unit.
 */
bool can_build_extra_base(const struct extra_type *pextra,
                          const struct player *pplayer,
                          const struct tile *ptile)
{
  if (!extra_can_be_built(pextra, ptile)) {
    return false;
  }

  if (is_extra_caused_by(pextra, EC_BASE)) {
    if (tile_terrain(ptile)->base_time == 0) {
      return false;
    }
    if (tile_city(ptile) != nullptr
        && extra_base_get(pextra)->border_sq >= 0) {
      return false;
    }
  }

  if (is_extra_caused_by(pextra, EC_ROAD)
      && tile_terrain(ptile)->road_time == 0) {
    return false;
  }

  if (pplayer != nullptr
      && !player_knows_techs_with_flag(pplayer, TF_BRIDGE)) {
    /* Player does not know technology to bridge over extras that require it.
     */
    extra_type_list_iterate(pextra->bridged, pbridged)
    {
      if (tile_has_extra(ptile, pbridged)) {
        // Tile has extra that would require bridging over.
        return false;
      }
    }
    extra_type_list_iterate_end;
  }

  return true;
}

/**
   Tells if player can build extra to tile with suitable unit.
 */
bool player_can_build_extra(const struct extra_type *pextra,
                            const struct player *pplayer,
                            const struct tile *ptile)
{
  if (!can_build_extra_base(pextra, pplayer, ptile)) {
    return false;
  }

  return are_reqs_active(pplayer, tile_owner(ptile), nullptr, nullptr, ptile,
                         nullptr, nullptr, nullptr, nullptr, nullptr,
                         &pextra->reqs, RPT_POSSIBLE);
}

/**
   Tells if player can place extra on tile.
 */
bool player_can_place_extra(const struct extra_type *pextra,
                            const struct player *pplayer,
                            const struct tile *ptile)
{
  if (ptile->placing != nullptr) {
    // Already placing something
    return false;
  }

  if (tile_terrain(ptile)->placing_time <= 0) {
    // Can't place to this terrain
    return false;
  }

  if (game.info.borders != BORDERS_DISABLED) {
    if (tile_owner(ptile) != pplayer) {
      return false;
    }
  } else {
    struct city *pcity = tile_worked(ptile);

    if (pcity == nullptr || city_owner(pcity) != pplayer) {
      return false;
    }
  }

  // Placing extras is not allowed to tiles where also workers do changes.
  unit_list_iterate(ptile->units, punit)
  {
    tile_changing_activities_iterate(act)
    {
      if (punit->activity == act) {
        return false;
      }
    }
    tile_changing_activities_iterate_end;
  }
  unit_list_iterate_end;

  return player_can_build_extra(pextra, pplayer, ptile);
}

/**
   Tells if unit can build extra on tile.
 */
bool can_build_extra(const struct extra_type *pextra,
                     const struct unit *punit, const struct tile *ptile)
{
  struct player *pplayer = unit_owner(punit);

  if (!can_build_extra_base(pextra, pplayer, ptile)) {
    return false;
  }

  return are_reqs_active(pplayer, tile_owner(ptile), nullptr, nullptr, ptile,
                         punit, unit_type_get(punit), nullptr, nullptr,
                         nullptr, &pextra->reqs, RPT_CERTAIN);
}

/**
   Is it possible at all to remove this extra now
 */
static bool can_extra_be_removed(const struct extra_type *pextra,
                                 const struct tile *ptile)
{
  struct city *pcity = tile_city(ptile);

  // Cannot remove EF_ALWAYS_ON_CITY_CENTER extras from city center.
  if (pcity != nullptr) {
    if (extra_has_flag(pextra, EF_ALWAYS_ON_CITY_CENTER)) {
      return false;
    }
    if (extra_has_flag(pextra, EF_AUTO_ON_CITY_CENTER)) {
      struct tile *vtile = tile_virtual_new(ptile);

      // Would extra get rebuilt if removed
      tile_remove_extra(vtile, pextra);
      if (player_can_build_extra(pextra, city_owner(pcity), vtile)) {
        /* No need to worry about conflicting extras - extra would had
         * not been here if conflicting one is. */
        tile_virtual_destroy(vtile);

        return false;
      }

      tile_virtual_destroy(vtile);
    }
  }

  return true;
}

/**
   Tells if player can remove extra from tile with suitable unit.
 */
bool player_can_remove_extra(const struct extra_type *pextra,
                             const struct player *pplayer,
                             const struct tile *ptile)
{
  if (!can_extra_be_removed(pextra, ptile)) {
    return false;
  }

  // For huts, it's not checked if player has any non-HUT_NOTHING units
  return are_reqs_active(pplayer, tile_owner(ptile), nullptr, nullptr, ptile,
                         nullptr, nullptr, nullptr, nullptr, nullptr,
                         &pextra->rmreqs, RPT_POSSIBLE);
}

/**
   Tells if unit can remove extra from tile.
   Does not examine action requirements if an action is required for it.
 */
bool can_remove_extra(const struct extra_type *pextra,
                      const struct unit *punit, const struct tile *ptile)
{
  if (!can_extra_be_removed(pextra, ptile)) {
    return false;
  }

  return are_reqs_active(unit_owner(punit), tile_owner(ptile), nullptr,
                         nullptr, ptile, punit, unit_type_get(punit),
                         nullptr, nullptr, nullptr, &pextra->rmreqs,
                         RPT_CERTAIN);
}

/**
   Is tile native to extra?
 */
bool is_native_tile_to_extra(const struct extra_type *pextra,
                             const struct tile *ptile)
{
  struct terrain *pterr = tile_terrain(ptile);

  if (terrain_has_resource(pterr, pextra)) {
    return true;
  }

  if (is_extra_caused_by(pextra, EC_IRRIGATION)
      && pterr->irrigation_result != pterr) {
    return false;
  }

  if (is_extra_caused_by(pextra, EC_MINE) && pterr->mining_result != pterr) {
    return false;
  }

  if (is_extra_caused_by(pextra, EC_BASE)) {
    if (pterr->base_time == 0) {
      return false;
    }
    if (tile_city(ptile) != nullptr
        && extra_base_get(pextra)->border_sq >= 0) {
      return false;
    }
  }

  if (is_extra_caused_by(pextra, EC_ROAD)) {
    struct road_type *proad = extra_road_get(pextra);

    if (road_has_flag(proad, RF_RIVER)) {
      if (!terrain_has_flag(pterr, TER_CAN_HAVE_RIVER)) {
        return false;
      }
    } else if (pterr->road_time == 0) {
      return false;
    }
  }

  return are_reqs_active(nullptr, nullptr, nullptr, nullptr, ptile, nullptr,
                         nullptr, nullptr, nullptr, nullptr, &pextra->reqs,
                         RPT_POSSIBLE);
}

/**
   Returns TRUE iff an extra that conflicts with pextra exists at ptile.
 */
bool extra_conflicting_on_tile(const struct extra_type *pextra,
                               const struct tile *ptile)
{
  extra_type_iterate(old_extra)
  {
    if (tile_has_extra(ptile, old_extra)
        && !can_extras_coexist(old_extra, pextra)) {
      return true;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Returns TRUE iff an extra on the tile is a hut (removed by entering).
   The effect of entering is handled by unit_enter_hut() in unittools.c
 */
bool hut_on_tile(const struct tile *ptile)
{
  extra_type_by_rmcause_iterate(ERM_ENTER, extra)
  {
    if (tile_has_extra(ptile, extra)) {
      return true;
    }
  }
  extra_type_by_rmcause_iterate_end;

  return false;
}

/**
   Returns TRUE iff the unit can enter any hut on the tile.
   For the unit, tests only its class and its owner.
 */
bool unit_can_enter_hut(const struct unit *punit, const struct tile *ptile)
{
  if (HUT_NORMAL != unit_class_get(punit)->hut_behavior) {
    return false;
  }
  extra_type_by_rmcause_iterate(ERM_ENTER, extra)
  {
    if (tile_has_extra(ptile, extra)
        && are_reqs_active(unit_owner(punit), tile_owner(ptile), nullptr,
                           nullptr, ptile, nullptr, nullptr, nullptr,
                           nullptr, nullptr, &extra->rmreqs, RPT_POSSIBLE)) {
      return true;
    }
  }
  extra_type_by_rmcause_iterate_end;
  return false;
}

/**
   Returns TRUE iff the unit can enter or frighten any hut on the tile.
   For the unit, tests only its class and its owner.
 */
bool unit_can_displace_hut(const struct unit *punit,
                           const struct tile *ptile)
{
  if (HUT_NOTHING == unit_class_get(punit)->hut_behavior) {
    return false;
  }
  extra_type_by_rmcause_iterate(ERM_ENTER, extra)
  {
    if (tile_has_extra(ptile, extra)
        && are_reqs_active(unit_owner(punit), tile_owner(ptile), nullptr,
                           nullptr, ptile, nullptr, nullptr, nullptr,
                           nullptr, nullptr, &extra->rmreqs, RPT_POSSIBLE)) {
      return true;
    }
  }
  extra_type_by_rmcause_iterate_end;
  return false;
}

/**
   Returns next extra by cause that unit or player can build to tile.
 */
struct extra_type *next_extra_for_tile(const struct tile *ptile,
                                       enum extra_cause cause,
                                       const struct player *pplayer,
                                       const struct unit *punit)
{
  if (cause == EC_IRRIGATION) {
    struct terrain *pterrain = tile_terrain(ptile);

    if (pterrain->irrigation_result != pterrain) {
      // No extra can be created by irrigation the tile
      return nullptr;
    }
  }
  if (cause == EC_MINE) {
    struct terrain *pterrain = tile_terrain(ptile);

    if (pterrain->mining_result != pterrain) {
      // No extra can be created by mining the tile
      return nullptr;
    }
  }

  extra_type_by_cause_iterate(cause, pextra)
  {
    if (!tile_has_extra(ptile, pextra)) {
      if (punit != nullptr) {
        if (can_build_extra(pextra, punit, ptile)) {
          return pextra;
        }
      } else {
        // punit is certainly nullptr, pplayer can be too
        if (player_can_build_extra(pextra, pplayer, ptile)) {
          return pextra;
        }
      }
    }
  }
  extra_type_by_cause_iterate_end;

  return nullptr;
}

/**
   Returns prev extra by cause that unit or player can remove from tile.
 */
struct extra_type *prev_extra_in_tile(const struct tile *ptile,
                                      enum extra_rmcause rmcause,
                                      const struct player *pplayer,
                                      const struct unit *punit)
{
  fc_assert(punit != nullptr || pplayer != nullptr);

  extra_type_by_rmcause_iterate(rmcause, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      if (punit != nullptr) {
        if (can_remove_extra(pextra, punit, ptile)) {
          return pextra;
        }
      } else {
        if (player_can_remove_extra(pextra, pplayer, ptile)) {
          return pextra;
        }
      }
    }
  }
  extra_type_by_rmcause_iterate_end;

  return nullptr;
}

/**
   Is extra native to unit class?
 */
bool is_native_extra_to_uclass(const struct extra_type *pextra,
                               const struct unit_class *pclass)
{
  return BV_ISSET(pextra->native_to, uclass_index(pclass));
}

/**
   Is extra native to unit type?
 */
bool is_native_extra_to_utype(const struct extra_type *pextra,
                              const struct unit_type *punittype)
{
  return is_native_extra_to_uclass(pextra, utype_class(punittype));
}

/**
   Check if extra has given flag
 */
bool extra_has_flag(const struct extra_type *pextra, enum extra_flag_id flag)
{
  return BV_ISSET(pextra->flags, flag);
}

/**
   Returns TRUE iff any cardinally adjacent tile contains an extra with
   the given flag (does not check ptile itself).
 */
bool is_extra_flag_card_near(const struct tile *ptile,
                             enum extra_flag_id flag)
{
  extra_type_iterate(pextra)
  {
    if (extra_has_flag(pextra, flag)) {
      cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile)
      {
        if (tile_has_extra(adjc_tile, pextra)) {
          return true;
        }
      }
      cardinal_adjc_iterate_end;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Returns TRUE iff any adjacent tile contains an extra with the given flag
   (does not check ptile itself).
 */
bool is_extra_flag_near_tile(const struct tile *ptile,
                             enum extra_flag_id flag)
{
  extra_type_iterate(pextra)
  {
    if (extra_has_flag(pextra, flag)) {
      adjc_iterate(&(wld.map), ptile, adjc_tile)
      {
        if (tile_has_extra(adjc_tile, pextra)) {
          return true;
        }
      }
      adjc_iterate_end;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Initialize user extra flags.
 */
void user_extra_flags_init()
{
  int i;

  for (i = 0; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
    user_flag_init(&user_extra_flags[i]);
  }
}

/**
   Frees the memory associated with all extra flags
 */
void extra_flags_free()
{
  int i;

  for (i = 0; i < MAX_NUM_USER_EXTRA_FLAGS; i++) {
    user_flag_free(&user_extra_flags[i]);
  }
}

/**
   Sets user defined name for extra flag.
 */
void set_user_extra_flag_name(enum extra_flag_id id, const char *name,
                              const char *helptxt)
{
  int efid = id - EF_USER_FLAG_1;

  fc_assert_ret(id >= EF_USER_FLAG_1 && id <= EF_LAST_USER_FLAG);
  delete[] user_extra_flags[efid].name;
  user_extra_flags[efid].name = nullptr;
  if (name && name[0] != '\0') {
    user_extra_flags[efid].name = fc_strdup(name);
  }
  delete[] user_extra_flags[efid].helptxt;
  user_extra_flags[efid].helptxt = nullptr;
  if (helptxt && helptxt[0] != '\0') {
    user_extra_flags[efid].helptxt = fc_strdup(helptxt);
  }
}

/**
   Extra flag name callback, called from specenum code.
 */
const char *extra_flag_id_name_cb(enum extra_flag_id flag)
{
  if (flag < EF_USER_FLAG_1 || flag > EF_LAST_USER_FLAG) {
    return nullptr;
  }

  return user_extra_flags[flag - EF_USER_FLAG_1].name;
}

/**
   Return the (untranslated) help text of the user extra flag.
 */
const char *extra_flag_helptxt(enum extra_flag_id id)
{
  fc_assert(id >= EF_USER_FLAG_1 && id <= EF_LAST_USER_FLAG);

  return user_extra_flags[id - EF_USER_FLAG_1].helptxt;
}

/**
   Can two extras coexist in same tile?
 */
bool can_extras_coexist(const struct extra_type *pextra1,
                        const struct extra_type *pextra2)
{
  if (pextra1 == pextra2) {
    return true;
  }

  return !BV_ISSET(pextra1->conflicts, extra_index(pextra2));
}

/**
   Does the extra count toward environment upset?
 */
bool extra_causes_env_upset(struct extra_type *pextra,
                            enum environment_upset_type upset)
{
  switch (upset) {
  case EUT_GLOBAL_WARMING:
    return extra_has_flag(pextra, EF_GLOBAL_WARMING);
  case EUT_NUCLEAR_WINTER:
    return extra_has_flag(pextra, EF_NUCLEAR_WINTER);
  }

  return false;
}

/**
   Is the extra caused by some kind of worker action?
 */
bool is_extra_caused_by_worker_action(const struct extra_type *pextra)
{
  // Is any of the worker build action bits set?
  return (
      pextra->causes
      & (1 << EC_IRRIGATION | 1 << EC_MINE | 1 << EC_BASE | 1 << EC_ROAD));
}

/**
   Is the extra removed by some kind of worker action?
 */
bool is_extra_removed_by_worker_action(const struct extra_type *pextra)
{
  // Is any of the worker remove action bits set?
  return (pextra->rmcauses
          & (1 << ERM_CLEANPOLLUTION | 1 << ERM_CLEANFALLOUT
             | 1 << ERM_PILLAGE));
}

/**
   Is the extra caused by specific worker action?
 */
bool is_extra_caused_by_action(const struct extra_type *pextra,
                               const struct action *paction)
{
  enum unit_activity act = action_get_activity(paction);
  return is_extra_caused_by(pextra, activity_to_extra_cause(act));
}

/**
   Is the extra removed by specific worker action?
 */
bool is_extra_removed_by_action(const struct extra_type *pextra,
                                const struct action *paction)
{
  enum unit_activity act = action_get_activity(paction);
  return is_extra_removed_by(pextra, activity_to_extra_rmcause(act));
}

/**
   What extra cause activity is considered to be?
 */
enum extra_cause activity_to_extra_cause(enum unit_activity act)
{
  switch (act) {
  case ACTIVITY_IRRIGATE:
    return EC_IRRIGATION;
  case ACTIVITY_MINE:
    return EC_MINE;
  case ACTIVITY_BASE:
    return EC_BASE;
  case ACTIVITY_GEN_ROAD:
    return EC_ROAD;
  default:
    break;
  }

  return EC_NONE;
}

/**
   What extra rmcause activity is considered to be?
 */
enum extra_rmcause activity_to_extra_rmcause(enum unit_activity act)
{
  switch (act) {
  case ACTIVITY_PILLAGE:
    return ERM_PILLAGE;
  case ACTIVITY_POLLUTION:
    return ERM_CLEANPOLLUTION;
  case ACTIVITY_FALLOUT:
    return ERM_CLEANFALLOUT;
  default:
    break;
  }

  return ERM_NONE;
}

/**
   Who owns extras on tile
 */
struct player *extra_owner(const struct tile *ptile)
{
  return ptile->extras_owner;
}

/**
   Are all the requirements for extra to appear on tile fulfilled.
 */
bool can_extra_appear(const struct extra_type *pextra,
                      const struct tile *ptile)
{
  return !tile_has_extra(ptile, pextra)
         && is_extra_caused_by(pextra, EC_APPEARANCE)
         && is_native_tile_to_extra(pextra, ptile)
         && !extra_conflicting_on_tile(pextra, ptile)
         && are_reqs_active(nullptr, tile_owner(ptile), nullptr, nullptr,
                            ptile, nullptr, nullptr, nullptr, nullptr,
                            nullptr, &pextra->appearance_reqs, RPT_CERTAIN);
}

/**
   Are all the requirements for extra to disappear from tile fulfilled.
 */
bool can_extra_disappear(const struct extra_type *pextra,
                         const struct tile *ptile)
{
  return tile_has_extra(ptile, pextra)
         && is_extra_removed_by(pextra, ERM_DISAPPEARANCE)
         && can_extra_be_removed(pextra, ptile)
         && are_reqs_active(nullptr, tile_owner(ptile), nullptr, nullptr,
                            ptile, nullptr, nullptr, nullptr, nullptr,
                            nullptr, &pextra->disappearance_reqs,
                            RPT_CERTAIN);
}

/**
   Extra is not hidden from the user.
 */
bool player_knows_extra_exist(const struct player *pplayer,
                              const struct extra_type *pextra,
                              const struct tile *ptile)
{
  if (!tile_has_extra(ptile, pextra)) {
    return false;
  }

  return research_invention_state(research_get(pplayer),
                                  pextra->visibility_req)
         == TECH_KNOWN;
}
