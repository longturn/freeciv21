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

// utility
#include "fcintl.h"
#include "log.h" // fc_assert
#include "rand.h"
#include "shared.h"
#include "support.h"

// common
#include "extras.h"
#include "game.h"
#include "map.h"
#include "rgbcolor.h"
#include "road.h"

#include "terrain.h"

static struct terrain civ_terrains[MAX_NUM_TERRAINS];
static struct user_flag user_terrain_flags[MAX_NUM_USER_TER_FLAGS];

/**
   Initialize terrain and resource structures.
 */
void terrains_init()
{
  int i;

  for (i = 0; i < ARRAY_SIZE(civ_terrains); i++) {
    // Can't use terrain_by_number here because it does a bounds check.
    civ_terrains[i].item_number = i;
    civ_terrains[i].ruledit_disabled = false;
    civ_terrains[i].rgb = nullptr;
    civ_terrains[i].animal = nullptr;
  }
}

/**
   Free memory which is associated with terrain types.
 */
void terrains_free()
{
  terrain_type_iterate(pterrain)
  {
    delete pterrain->helptext;
    pterrain->helptext = nullptr;
    delete[] pterrain->resources;
    pterrain->resources = nullptr;
    if (pterrain->rgb != nullptr) {
      /* Server allocates this on ruleset loading, client when
       * ruleset packet is received. */
      rgbcolor_destroy(pterrain->rgb);
      pterrain->rgb = nullptr;
    }
  }
  terrain_type_iterate_end;
}

/**
   Return the first item of terrains.
 */
struct terrain *terrain_array_first()
{
  if (game.control.terrain_count > 0) {
    return civ_terrains;
  }
  return nullptr;
}

/**
   Return the last item of terrains.
 */
const struct terrain *terrain_array_last()
{
  if (game.control.terrain_count > 0) {
    return &civ_terrains[game.control.terrain_count - 1];
  }
  return nullptr;
}

/**
   Return the number of terrains.
 */
Terrain_type_id terrain_count() { return game.control.terrain_count; }

/**
   Return the terrain identifier.
 */
char terrain_identifier(const struct terrain *pterrain)
{
  fc_assert_ret_val(pterrain, '\0');
  return pterrain->identifier;
}

/**
   Return the terrain index.

   Currently same as terrain_number(), paired with terrain_count()
   indicates use as an array index.
 */
Terrain_type_id terrain_index(const struct terrain *pterrain)
{
  fc_assert_ret_val(pterrain, 0);
  return pterrain - civ_terrains;
}

/**
   Return the terrain index.
 */
Terrain_type_id terrain_number(const struct terrain *pterrain)
{
  fc_assert_ret_val(pterrain, 0);
  return pterrain->item_number;
}

/**
   Return the terrain for the given terrain index.
 */
struct terrain *terrain_by_number(const Terrain_type_id type)
{
  if (type < 0 || type >= game.control.terrain_count) {
    // This isn't an error; some T_UNKNOWN callers depend on it.
    return nullptr;
  }
  return &civ_terrains[type];
}

/**
   Return the terrain type matching the name, or T_UNKNOWN if none matches.
 */
struct terrain *terrain_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  terrain_type_iterate(pterrain)
  {
    if (0 == fc_strcasecmp(terrain_rule_name(pterrain), qname)) {
      return pterrain;
    }
  }
  terrain_type_iterate_end;

  return T_UNKNOWN;
}

/**
   Return the terrain type matching the name, or T_UNKNOWN if none matches.
 */
struct terrain *terrain_by_translated_name(const char *name)
{
  terrain_type_iterate(pterrain)
  {
    if (0 == strcmp(terrain_name_translation(pterrain), name)) {
      return pterrain;
    }
  }
  terrain_type_iterate_end;

  return T_UNKNOWN;
}

/**
   Return the (translated) name of the terrain.
   You don't have to free the return pointer.
 */
const char *terrain_name_translation(const struct terrain *pterrain)
{
  return name_translation_get(&pterrain->name);
}

/**
   Return the (untranslated) rule name of the terrain.
   You don't have to free the return pointer.
 */
const char *terrain_rule_name(const struct terrain *pterrain)
{
  return rule_name_get(&pterrain->name);
}

/**
   Check for resource in terrain resources list.
 */
bool terrain_has_resource(const struct terrain *pterrain,
                          const struct extra_type *presource)
{
  struct extra_type **r = pterrain->resources;

  while (nullptr != *r) {
    if (*r == presource) {
      return true;
    }
    r++;
  }
  return false;
}

/**
   Initialize resource_type structure.
 */
struct resource_type *resource_type_init(struct extra_type *pextra)
{
  auto *presource = new resource_type;

  pextra->data.resource = presource;

  presource->self = pextra;

  return presource;
}

/**
   Free the memory associated with resource types
 */
void resource_types_free()
{
  // Resource structure itself is freed as part of extras destruction.
}

/**
   This iterator behaves like adjc_iterate or cardinal_adjc_iterate depending
   on the value of card_only.
 */
#define variable_adjc_iterate(nmap, center_tile, _tile, card_only)          \
  {                                                                         \
    enum direction8 *_tile##_list;                                          \
    int _tile##_count;                                                      \
                                                                            \
    if (card_only) {                                                        \
      _tile##_list = wld.map.cardinal_dirs;                                 \
      _tile##_count = wld.map.num_cardinal_dirs;                            \
    } else {                                                                \
      _tile##_list = wld.map.valid_dirs;                                    \
      _tile##_count = wld.map.num_valid_dirs;                               \
    }                                                                       \
    adjc_dirlist_iterate(nmap, center_tile, _tile, _tile##_dir,             \
                         _tile##_list, _tile##_count)                       \
    {

#define variable_adjc_iterate_end                                           \
  }                                                                         \
  adjc_dirlist_iterate_end;                                                 \
  }

/**
   Returns TRUE iff any cardinally adjacent tile contains the given terrain.
 */
bool is_terrain_card_near(const struct tile *ptile,
                          const struct terrain *pterrain, bool check_self)
{
  if (!pterrain) {
    return false;
  }

  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    if (tile_terrain(adjc_tile) == pterrain) {
      return true;
    }
  }
  cardinal_adjc_iterate_end;

  return check_self && ptile->terrain == pterrain;
}

/**
   Returns TRUE iff any adjacent tile contains the given terrain.
 */
bool is_terrain_near_tile(const struct tile *ptile,
                          const struct terrain *pterrain, bool check_self)
{
  if (!pterrain) {
    return false;
  }

  adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    if (tile_terrain(adjc_tile) == pterrain) {
      return true;
    }
  }
  adjc_iterate_end;

  return check_self && ptile->terrain == pterrain;
}

/**
   Return the number of adjacent tiles that have the given terrain property.
 */
int count_terrain_property_near_tile(const struct tile *ptile,
                                     bool cardinal_only, bool percentage,
                                     enum mapgen_terrain_property prop)
{
  int count = 0, total = 0;

  variable_adjc_iterate(&(wld.map), ptile, adjc_tile, cardinal_only)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (pterrain->property[prop] > 0) {
      count++;
    }
    total++;
  }
  variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / std::max(1, total);
  }
  return count;
}

/**
   Returns TRUE iff any cardinally adjacent tile contains terrain with the
   given flag (does not check ptile itself).
 */
bool is_terrain_flag_card_near(const struct tile *ptile,
                               enum terrain_flag_id flag)
{
  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (T_UNKNOWN != pterrain && terrain_has_flag(pterrain, flag)) {
      return true;
    }
  }
  cardinal_adjc_iterate_end;

  return false;
}

/**
   Returns TRUE iff any adjacent tile contains terrain with the given flag
   (does not check ptile itself).
 */
bool is_terrain_flag_near_tile(const struct tile *ptile,
                               enum terrain_flag_id flag)
{
  adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (T_UNKNOWN != pterrain && terrain_has_flag(pterrain, flag)) {
      return true;
    }
  }
  adjc_iterate_end;

  return false;
}

/**
   Return the number of adjacent tiles that have terrain with the given flag
   (not including ptile itself).
 */
int count_terrain_flag_near_tile(const struct tile *ptile,
                                 bool cardinal_only, bool percentage,
                                 enum terrain_flag_id flag)
{
  int count = 0, total = 0;

  variable_adjc_iterate(&(wld.map), ptile, adjc_tile, cardinal_only)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (T_UNKNOWN != pterrain && terrain_has_flag(pterrain, flag)) {
      count++;
    }
    total++;
  }
  variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / std::max(1, total);
  }
  return count;
}

/**
   Return a (static) string with extra(s) name(s):
     eg: "Mine"
     eg: "Road/Farmland"
   This only includes "infrastructure", i.e., man-made extras.
 */
const char *get_infrastructure_text(bv_extras extras)
{
  static char s[256];
  char *p;
  int len;

  s[0] = '\0';

  extra_type_iterate(pextra)
  {
    if (pextra->category == ECAT_INFRA
        && BV_ISSET(extras, extra_index(pextra))) {
      bool hidden = false;

      extra_type_iterate(top)
      {
        int topi = extra_index(top);

        if (BV_ISSET(pextra->hidden_by, topi) && BV_ISSET(extras, topi)) {
          hidden = true;
          break;
        }
      }
      extra_type_iterate_end;

      if (!hidden) {
        cat_snprintf(s, sizeof(s), "%s/", extra_name_translation(pextra));
      }
    }
  }
  extra_type_iterate_end;

  len = qstrlen(s);
  p = s + len - 1;
  if (len > 0 && *p == '/') {
    *p = '\0';
  }

  return s;
}

/**
   Returns the highest-priority (best) extra to be pillaged from the
   terrain set.  May return nullptr if nothing is available.
 */
struct extra_type *get_preferred_pillage(bv_extras extras)
{
  // Semi-arbitrary preference order reflecting previous behavior
  static const enum extra_cause prefs[] = {
      EC_IRRIGATION, EC_MINE,      EC_BASE,    EC_ROAD,     EC_HUT,
      EC_APPEARANCE, EC_POLLUTION, EC_FALLOUT, EC_RESOURCE, EC_NONE};
  int i;

  for (i = 0; i < ARRAY_SIZE(prefs); i++) {
    extra_type_by_cause_iterate_rev(prefs[i], pextra)
    {
      if (is_extra_removed_by(pextra, ERM_PILLAGE)
          && BV_ISSET(extras, extra_index(pextra))) {
        return pextra;
      }
    }
    extra_type_by_cause_iterate_rev_end;
  }

  return nullptr;
}

/**
   What terrain class terrain type belongs to.
 */
enum terrain_class terrain_type_terrain_class(const struct terrain *pterrain)
{
  return pterrain->tclass;
}

/**
   Is there terrain of the given class cardinally near tile?
   (Does not check ptile itself.)
 */
bool is_terrain_class_card_near(const struct tile *ptile,
                                enum terrain_class tclass)
{
  cardinal_adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (pterrain != T_UNKNOWN) {
      if (terrain_type_terrain_class(pterrain) == tclass) {
        return true;
      }
    }
  }
  cardinal_adjc_iterate_end;

  return false;
}

/**
   Is there terrain of the given class near tile?
   (Does not check ptile itself.)
 */
bool is_terrain_class_near_tile(const struct tile *ptile,
                                enum terrain_class tclass)
{
  adjc_iterate(&(wld.map), ptile, adjc_tile)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (pterrain != T_UNKNOWN) {
      if (terrain_type_terrain_class(pterrain) == tclass) {
        return true;
      }
    }
  }
  adjc_iterate_end;

  return false;
}

/**
   Return the number of adjacent tiles that have given terrain class
   (not including ptile itself).
 */
int count_terrain_class_near_tile(const struct tile *ptile,
                                  bool cardinal_only, bool percentage,
                                  enum terrain_class tclass)
{
  int count = 0, total = 0;

  variable_adjc_iterate(&(wld.map), ptile, adjc_tile, cardinal_only)
  {
    struct terrain *pterrain = tile_terrain(adjc_tile);

    if (T_UNKNOWN != pterrain
        && terrain_type_terrain_class(pterrain) == tclass) {
      count++;
    }
    total++;
  }
  variable_adjc_iterate_end;

  if (percentage) {
    count = count * 100 / std::max(1, total);
  }

  return count;
}

/**
   Return the (translated) name of the given terrain class.
   You don't have to free the return pointer.
 */
const char *terrain_class_name_translation(enum terrain_class tclass)
{
  if (!terrain_class_is_valid(tclass)) {
    return nullptr;
  }

  return _(terrain_class_name(tclass));
}

/**
   Can terrain support given infrastructure?
 */
bool terrain_can_support_alteration(const struct terrain *pterrain,
                                    enum terrain_alteration alter)
{
  switch (alter) {
  case TA_CAN_IRRIGATE:
    return (pterrain == pterrain->irrigation_result);
  case TA_CAN_MINE:
    return (pterrain == pterrain->mining_result);
  case TA_CAN_ROAD:
    return (pterrain->road_time > 0);
  default:
    break;
  }

  fc_assert(false);
  return false;
}

/**
    Time to complete the extra building activity on the given terrain.
 */
int terrain_extra_build_time(const struct terrain *pterrain,
                             enum unit_activity activity,
                             const struct extra_type *tgt)
{
  int factor;

  if (tgt != nullptr && tgt->build_time != 0) {
    // Extra specific build time
    return tgt->build_time;
  }

  if (tgt == nullptr) {
    factor = 1;
  } else {
    factor = tgt->build_time_factor;
  }

  // Terrain and activity specific build time
  switch (activity) {
  case ACTIVITY_BASE:
    return pterrain->base_time * factor;
  case ACTIVITY_GEN_ROAD:
    return pterrain->road_time * factor;
  case ACTIVITY_IRRIGATE:
    return pterrain->irrigation_time * factor;
  case ACTIVITY_MINE:
    return pterrain->mining_time * factor;
  default:
    fc_assert(false);
    return 0;
  }
}

/**
    Time to complete the extra removal activity on the given terrain.
 */
int terrain_extra_removal_time(const struct terrain *pterrain,
                               enum unit_activity activity,
                               const struct extra_type *tgt)
{
  int factor;

  if (tgt != nullptr && tgt->removal_time != 0) {
    // Extra specific removal time
    return tgt->removal_time;
  }

  if (tgt == nullptr) {
    factor = 1;
  } else {
    factor = tgt->removal_time_factor;
  }

  // Terrain and activity specific removal time
  switch (activity) {
  case ACTIVITY_POLLUTION:
    return pterrain->clean_pollution_time * factor;
  case ACTIVITY_FALLOUT:
    return pterrain->clean_fallout_time * factor;
  case ACTIVITY_PILLAGE:
    return pterrain->pillage_time * factor;
  default:
    fc_assert(false);
    return 0;
  }
}

/**
   Initialize user terrain type flags.
 */
void user_terrain_flags_init()
{
  int i;

  for (i = 0; i < MAX_NUM_USER_TER_FLAGS; i++) {
    user_flag_init(&user_terrain_flags[i]);
  }
}

/**
   Frees the memory associated with all user terrain flags
 */
void user_terrain_flags_free()
{
  int i;

  for (i = 0; i < MAX_NUM_USER_TER_FLAGS; i++) {
    user_flag_free(&user_terrain_flags[i]);
  }
}

/**
   Sets user defined name for terrain flag.
 */
void set_user_terrain_flag_name(enum terrain_flag_id id, const char *name,
                                const char *helptxt)
{
  int tfid = id - TER_USER_1;

  fc_assert_ret(id >= TER_USER_1 && id <= TER_USER_LAST);

  delete[] user_terrain_flags[tfid].name;
  user_terrain_flags[tfid].name = nullptr;
  if (name && name[0] != '\0') {
    user_terrain_flags[tfid].name = fc_strdup(name);
  }

  delete[] user_terrain_flags[tfid].helptxt;
  user_terrain_flags[tfid].helptxt = nullptr;
  if (helptxt && helptxt[0] != '\0') {
    user_terrain_flags[tfid].helptxt = fc_strdup(helptxt);
  }
}

/**
   Terrain flag name callback, called from specenum code.
 */
const char *terrain_flag_id_name_cb(enum terrain_flag_id flag)
{
  if (flag < TER_USER_1 || flag > TER_USER_LAST) {
    return nullptr;
  }

  return user_terrain_flags[flag - TER_USER_1].name;
}

/**
   Return the (untranslated) helptxt of the user terrain flag.
 */
const char *terrain_flag_helptxt(enum terrain_flag_id id)
{
  fc_assert(id >= TER_USER_1 && id <= TER_USER_LAST);

  return user_terrain_flags[id - TER_USER_1].helptxt;
}
