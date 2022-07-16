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

/**********************************************************************
   Functions for handling the nations.
***********************************************************************/

// utility
#include "fcintl.h"
#include "support.h"

// common
#include "connection.h"
#include "game.h"
#include "government.h"
#include "player.h"
#include "traits.h"

#include "nation.h"

// Nation set structure.
struct nation_set {
  struct name_translation name;
  char description[MAX_LEN_MSG];
};

std::vector<nation_type> nations;
static int num_nation_sets;
static struct nation_set nation_sets[MAX_NUM_NATION_SETS];
static int num_nation_groups;
static struct nation_group nation_groups[MAX_NUM_NATION_GROUPS];

/****************************************************************************
  Runs action if the nation is not valid.
****************************************************************************/
#ifdef FREECIV_DEBUG
#define NATION_CHECK(pnation, action)                                       \
  fc_assert_action(nation_check(pnation), action)

/**
   Returns TRUE if the nation is valid, else, print an error message and
   returns false.
 */
static bool nation_check(const nation_type *pnation)
{
  if (0 == game.control.nation_count) {
    qCritical("Function called before nations setup.");
    return false;
  }
  if (nullptr == pnation) {
    qCritical("This function has nullptr nation argument.");
    return false;
  }
  if (pnation->item_number < 0
      || pnation->item_number >= game.control.nation_count
      || &nations[nation_index(pnation)] != pnation) {
    qCritical("This function has bad nation number %d (count %d).",
              pnation->item_number, game.control.nation_count);
    return false;
  }
  return true;
}

#else                                 // FREECIV_DEBUG
#define NATION_CHECK(pnation, action) // Do Nothing.
#endif                                // FREECIV_DEBUG

/**
   Returns the nation that has the given (translated) plural noun.
   Returns NO_NATION_SELECTED if none match.
 */
struct nation_type *nation_by_translated_plural(const char *name)
{
  for (auto &pnation : nations) {
    if (0 == strcmp(nation_plural_translation(&pnation), name)) {
      return &pnation;
    }
  } // iterate over nations - pnation

  return NO_NATION_SELECTED;
}

/**
   Returns the nation that has the given (untranslated) rule name
 (adjective). Returns NO_NATION_SELECTED if none match.
 */
struct nation_type *nation_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  for (auto &pnation : nations) {
    if (0 == fc_strcasecmp(nation_rule_name(&pnation), qname)) {
      return &pnation;
    }
  } // iterate over nations - pnation

  return NO_NATION_SELECTED;
}

/**
   Return the (untranslated) rule name of the nation (adjective form).
   You don't have to free the return pointer.
 */
const char *nation_rule_name(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return "");

  return rule_name_get(&pnation->adjective);
}

/**
   Return the (translated) adjective for the given nation.
   You don't have to free the return pointer.
 */
const char *nation_adjective_translation(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return "");
  return name_translation_get(&pnation->adjective);
}

/**
   Return the (translated) plural noun of the given nation.
   You don't have to free the return pointer.
 */
const char *nation_plural_translation(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return "");
  return name_translation_get(&pnation->noun_plural);
}

/**
   Return the (translated) adjective for the given nation of a player.
   You don't have to free the return pointer.
 */
const char *nation_adjective_for_player(const struct player *pplayer)
{
  return nation_adjective_translation(nation_of_player(pplayer));
}

/**
   Return the (translated) plural noun of the given nation of a player.
   You don't have to free the return pointer.
 */
const char *nation_plural_for_player(const struct player *pplayer)
{
  return nation_plural_translation(nation_of_player(pplayer));
}

/**
   Return whether a nation is "pickable" -- whether players can select it
   at game start.
   (Client only function -- on the server, use client_can_pick_nation().)
 */
bool is_nation_pickable(const struct nation_type *nation)
{
  fc_assert_ret_val(!is_server(), false);
  return nation->client.is_pickable;
}

/**
   Return whether a nation is "playable"; i.e., whether a human player can
   choose this nation.  Barbarian and observer nations are not playable.

   This does not check whether a nation is "used" or "available".
 */
bool is_nation_playable(const struct nation_type *nation)
{
  NATION_CHECK(nation, return false);
  return nation->is_playable;
}

/**
   Returns which kind of barbarians can use this nation.

   This does not check whether a nation is "used" or "available".
 */
enum barbarian_type nation_barbarian_type(const struct nation_type *nation)
{
  NATION_CHECK(nation, return NOT_A_BARBARIAN);
  return nation->barb_type;
}

/****************************************************************************
  Nation leader.
****************************************************************************/
struct nation_leader {
  char *name;
  bool is_male;
};

/**
   Returns the list the nation leader names.
 */
const struct nation_leader_list *
nation_leaders(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return nullptr);
  return pnation->leaders;
}

/**
   Create a new leader for the nation.
 */
struct nation_leader *nation_leader_new(struct nation_type *pnation,
                                        const char *name, bool is_male)
{
  NATION_CHECK(pnation, return nullptr);
  auto *pleader = new nation_leader;
  pleader->name = fc_strdup(name);
  pleader->is_male = is_male;

  nation_leader_list_append(pnation->leaders, pleader);
  return pleader;
}

/**
   Destroy a nation leader created with nation_leader_new().
 */
static void nation_leader_destroy(struct nation_leader *pleader)
{
  delete[] pleader->name;
  delete pleader;
}

/**
   Returns the nation leader structure which match 'name' or nullptr if not
   found.
 */
struct nation_leader *
nation_leader_by_name(const struct nation_type *pnation, const char *name)
{
  NATION_CHECK(pnation, return nullptr);
  nation_leader_list_iterate(pnation->leaders, pleader)
  {
    if (0 == fc_strcasecmp(name, pleader->name)) {
      return pleader;
    }
  }
  nation_leader_list_iterate_end;
  return nullptr;
}

/**
   Return the name of the nation leader.
 */
const char *nation_leader_name(const struct nation_leader *pleader)
{
  fc_assert_ret_val(nullptr != pleader, nullptr);
  return pleader->name;
}

/**
   Return the sex of the nation leader.
 */
bool nation_leader_is_male(const struct nation_leader *pleader)
{
  fc_assert_ret_val(nullptr != pleader, true);
  return pleader->is_male;
}

/**
   Return translated version of nation legend.
 */
const char *nation_legend_translation(const struct nation_type *pnation,
                                      const char *legend)
{
  if (pnation->translation_domain == nullptr) {
    return _(legend);
  }

  return DG_(pnation->translation_domain, legend);
}

/****************************************************************************
  Nation default cities. The nation_city structure holds information about
  a default choice for the city name. The 'name' field is, of course, just
  the name for the city. The 'river' and 'terrain' fields are entries
  recording whether the terrain is present near the city - we give higher
  priority to cities which have matching terrain. In the case of a river we
  only care if the city is _on_ the river, for other terrain features we
  give the bonus if the city is close to the terrain.

  This is controlled through the nation's ruleset like this:
    cities = "Washington (ocean, river, swamp)", "New York (!mountains)"
****************************************************************************/
struct nation_city {
  char *name;
  enum nation_city_preference river;
  enum nation_city_preference terrain[MAX_NUM_TERRAINS];
};

/**
   Return the default cities of the nation (server only function).
 */
const struct nation_city_list *
nation_cities(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return nullptr);
  fc_assert_ret_val(is_server(), nullptr);

  return pnation->server.default_cities;
}

/**
   Create a new default city for the nation (server only function).
 */
struct nation_city *nation_city_new(struct nation_type *pnation,
                                    const char *name)
{
  struct nation_city *pncity;

  NATION_CHECK(pnation, return nullptr);
  fc_assert_ret_val(is_server(), nullptr);

  fc_assert(0 == NCP_NONE);
  pncity = new nation_city{}; // Set NCP_NONE.
  pncity->name = fc_strdup(name);

  nation_city_list_append(pnation->server.default_cities, pncity);
  return pncity;
}

/**
   Destroy a default nation city created with nation_city_new().
 */
static void nation_city_destroy(struct nation_city *pncity)
{
  delete[] pncity->name;
  delete pncity;
}

/**
   Reverts the nation city preference.
 */
enum nation_city_preference
nation_city_preference_revert(enum nation_city_preference prefer)
{
  switch (prefer) {
  case NCP_DISLIKE:
    return NCP_LIKE;
  case NCP_NONE:
    return NCP_NONE;
  case NCP_LIKE:
    return NCP_DISLIKE;
  }

  qCritical("%s(): Wrong nation_city_preference variant (%d).", __FUNCTION__,
            prefer);
  return NCP_NONE;
}

/**
   Set the default nation city preference for the terrain.
 */
void nation_city_set_terrain_preference(struct nation_city *pncity,
                                        const struct terrain *pterrain,
                                        enum nation_city_preference prefer)
{
  fc_assert_ret(nullptr != pncity);
  fc_assert_ret(nullptr != pterrain);
  pncity->terrain[terrain_index(pterrain)] = prefer;
}

/**
   Set the default nation city preference about rivers.
 */
void nation_city_set_river_preference(struct nation_city *pncity,
                                      enum nation_city_preference prefer)
{
  fc_assert_ret(nullptr != pncity);
  pncity->river = prefer;
}

/**
   Return the name of the default nation city.
 */
const char *nation_city_name(const struct nation_city *pncity)
{
  fc_assert_ret_val(nullptr != pncity, nullptr);
  return pncity->name;
}

/**
   Return the default nation city preference for the terrain.
 */
enum nation_city_preference
nation_city_terrain_preference(const struct nation_city *pncity,
                               const struct terrain *pterrain)
{
  fc_assert_ret_val(nullptr != pncity, NCP_DISLIKE);
  fc_assert_ret_val(nullptr != pterrain, NCP_DISLIKE);
  return pncity->terrain[terrain_index(pterrain)];
}

/**
   Return the default nation city preference for rivers.
 */
enum nation_city_preference
nation_city_river_preference(const struct nation_city *pncity)
{
  fc_assert_ret_val(nullptr != pncity, NCP_DISLIKE);
  return pncity->river;
}

/**
   Return the nation of a player.
 */
struct nation_type *nation_of_player(const struct player *pplayer)
{
  fc_assert_ret_val(nullptr != pplayer, nullptr);
  NATION_CHECK(pplayer->nation, return nullptr);
  return pplayer->nation;
}

/**
   Return the nation of the player who owns the city.
 */
struct nation_type *nation_of_city(const struct city *pcity)
{
  fc_assert_ret_val(pcity != nullptr, nullptr);
  return nation_of_player(city_owner(pcity));
}

/**
   Return the nation of the player who owns the unit.
 */
struct nation_type *nation_of_unit(const struct unit *punit)
{
  fc_assert_ret_val(punit != nullptr, nullptr);
  return nation_of_player(unit_owner(punit));
}

/**
   Return the nation with the given index.

   This function returns nullptr for an out-of-range index (some callers
   rely on this).
 */
struct nation_type *nation_by_number(const Nation_type_id nation)
{
  if (nation < 0 || nation >= game.control.nation_count) {
    return nullptr;
  }
  return &nations[nation];
}

/**
   Return the nation index.

   Currently same as nation_index(), paired with nation_count()
   indicates use as an array index.
 */
Nation_type_id nation_index(const struct nation_type *pnation)
{
  fc_assert_ret_val(nullptr != pnation, 0);
  return pnation - &nations[0];
}

/**
   Allocate resources associated with the given nation.
   Constructor for nation_type
 */
nation_type::nation_type()
{
  item_number = 0;
  translation_domain = nullptr;
  leaders = nation_leader_list_new_full(nation_leader_destroy);
  sets = nation_set_list_new();
  groups = nation_group_list_new();

  if (is_server()) {
    server.default_cities = nation_city_list_new_full(nation_city_destroy);
    server.civilwar_nations = nation_list_new();
    server.parent_nations = nation_list_new();
    server.conflicts_with = nation_list_new();
    // server.rgb starts out nullptr
    server.traits = new trait_limits[TRAIT_COUNT];
  }
}

/**
   De-allocate resources associated with the given nation.
   Destructor for nation_type
 */
nation_type::~nation_type()
{
  delete[] legend;
  delete[] translation_domain;
  nation_leader_list_destroy(leaders);
  nation_set_list_destroy(sets);
  nation_group_list_destroy(groups);

  if (is_server()) {
    nation_city_list_destroy(server.default_cities);
    nation_list_destroy(server.civilwar_nations);
    nation_list_destroy(server.parent_nations);
    nation_list_destroy(server.conflicts_with);
    rgbcolor_destroy(server.rgb);
    delete[] server.traits;
  }
}

/**
   Allocate space for the given number of nations.
 */
void nations_alloc(int num)
{
  game.control.nation_count = num;
  nations.resize(game.control.nation_count);
  int i = 0;
  for (auto &pnation : nations) {
    pnation.item_number = i++;
  } // iterate over nations - pnation
}

/**
   De-allocate the currently allocated nations.
 */
void nations_free()
{
  nations.clear();
  game.control.nation_count = 0;
}

/**
   Returns initial government type for this nation.
   Always returns non-nullptr -- nation-specific government or failing that
   ruleset default government.
 */
struct government *
init_government_of_nation(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return game.default_government);
  if (pnation->init_government) {
    return pnation->init_government;
  } else {
    return game.default_government;
  }
}

/**
   Returns nation's style.
 */
struct nation_style *style_of_nation(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return 0);
  return pnation->style;
}

/**
   Returns nation's player color preference, or nullptr if none.
   Server only function.
 */
const struct rgbcolor *nation_color(const struct nation_type *pnation)
{
  NATION_CHECK(pnation, return nullptr);
  return pnation->server.rgb;
}

/**
   Return the number of nation sets.
 */
int nation_set_count() { return num_nation_sets; }

/**
   Return the nation set index.
 */
int nation_set_index(const struct nation_set *pset)
{
  fc_assert_ret_val(nullptr != pset, 0);
  return pset - nation_sets;
}

/**
   Return the nation set index.
 */
int nation_set_number(const struct nation_set *pset)
{
  return nation_set_index(pset);
}

/**
   Add new set into the array of nation sets.
 */
struct nation_set *nation_set_new(const char *set_name,
                                  const char *set_rule_name,
                                  const char *set_description)
{
  struct nation_set *pset;

  if (MAX_NUM_NATION_SETS <= num_nation_sets) {
    qCritical("Too many nation sets (%d is the maximum).",
              MAX_NUM_NATION_SETS);
    return nullptr;
  }

  // Print the name and truncate if needed.
  pset = nation_sets + num_nation_sets;
  names_set(&pset->name, nullptr, set_name, set_rule_name);
  (void) sz_loud_strlcpy(
      pset->description, set_description,
      "Nation set description \"%s\" too long; truncating.");

  if (nullptr != nation_set_by_rule_name(rule_name_get(&pset->name))) {
    qCritical("Duplicate nation set name %s.", rule_name_get(&pset->name));
    return nullptr;
  }

  if (nullptr != nation_group_by_rule_name(rule_name_get(&pset->name))) {
    qCritical("Nation set name %s is already used for a group.",
              rule_name_get(&pset->name));
    return nullptr;
  }

  num_nation_sets++;

  return pset;
}

/**
   Return the nation set with the given index.

   This function returns nullptr for an out-of-range index (some callers
   rely on this).
 */
struct nation_set *nation_set_by_number(int id)
{
  if (id < 0 || id >= num_nation_sets) {
    return nullptr;
  }
  return nation_sets + id;
}

/**
   Return the nation set that has the given (untranslated) rule name.
   Returns nullptr if no set is found.
 */
struct nation_set *nation_set_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  nation_sets_iterate(pset)
  {
    if (0 == fc_strcasecmp(rule_name_get(&pset->name), qname)) {
      return pset;
    }
  }
  nation_sets_iterate_end;

  return nullptr;
}

/**
   Return the untranslated name of a nation set (including qualifier, if
 any). You usually want nation_set_rule_name() instead. You don't have to
 free the return pointer.
 */
const char *nation_set_untranslated_name(const struct nation_set *pset)
{
  fc_assert_ret_val(nullptr != pset, nullptr);
  return untranslated_name(&pset->name);
}

/**
   Return the (untranslated) rule name of a nation set.
   You don't have to free the return pointer.
 */
const char *nation_set_rule_name(const struct nation_set *pset)
{
  fc_assert_ret_val(nullptr != pset, nullptr);

  return rule_name_get(&pset->name);
}

/**
   Return the translated name of a nation set.
   You don't have to free the return pointer.
 */
const char *nation_set_name_translation(const struct nation_set *pset)
{
  fc_assert_ret_val(nullptr != pset, nullptr);
  return name_translation_get(&pset->name);
}

/**
   Return the (untranslated) user description of a nation set.
   You don't have to free the return pointer.
 */
const char *nation_set_description(const struct nation_set *pset)
{
  fc_assert_ret_val(nullptr != pset, nullptr);
  return pset->description;
}

/**
   Check if the given nation is in a given set
 */
bool nation_is_in_set(const struct nation_type *pnation,
                      const struct nation_set *pset)
{
  fc_assert_ret_val(nullptr != pnation, false);

  nation_set_list_iterate(pnation->sets, aset)
  {
    if (aset == pset) {
      return true;
    }
  }
  nation_set_list_iterate_end;
  return false;
}

/**
   Returns the nation set that would be selected by the given value of the
   'nationset' server setting.
   This differs from nation_set_by_rule_name() for the empty string, where
   the first (ruleset default) nationset will be used; and similarly for
   a nationset not matched in the ruleset.
   The knowledge of the interpretation of this setting encapsulated here is
   required on both server and client.
 */
struct nation_set *nation_set_by_setting_value(const char *setting)
{
  struct nation_set *pset = nullptr;

  if (strlen(setting) > 0) {
    pset = nation_set_by_rule_name(setting);
  }
  if (pset == nullptr) {
    /* Either no nation set specified, or the specified one isn't in the
     * current ruleset. Default to the first nation set specified by
     * the ruleset. */
    pset = nation_set_by_number(0);
  }
  fc_assert(pset != nullptr);

  return pset;
}

/****************************************************************************
  Nation set iterator.
****************************************************************************/
struct nation_set_iter {
  struct iterator vtable;
  struct nation_set *p, *end;
};
#define NATION_SET_ITER(p) ((struct nation_set_iter *) (p))

/**
   Implementation of iterator 'sizeof' function.
 */
size_t nation_set_iter_sizeof() { return sizeof(struct nation_set_iter); }

/**
   Implementation of iterator 'next' function.
 */
static void nation_set_iter_next(struct iterator *iter)
{
  NATION_SET_ITER(iter)->p++;
}

/**
   Implementation of iterator 'get' function.
 */
static void *nation_set_iter_get(const struct iterator *iter)
{
  return NATION_SET_ITER(iter)->p;
}

/**
   Implementation of iterator 'valid' function.
 */
static bool nation_set_iter_valid(const struct iterator *iter)
{
  struct nation_set_iter *it = NATION_SET_ITER(iter);
  return it->p < it->end;
}

/**
   Implementation of iterator 'init' function.
 */
struct iterator *nation_set_iter_init(struct nation_set_iter *it)
{
  it->vtable.next = nation_set_iter_next;
  it->vtable.get = nation_set_iter_get;
  it->vtable.valid = nation_set_iter_valid;
  it->p = nation_sets;
  it->end = nation_sets + nation_set_count();
  return ITERATOR(it);
}

/**
   Return the number of nation groups.
 */
int nation_group_count() { return num_nation_groups; }

/**
   Return the nation group index.
 */
int nation_group_index(const struct nation_group *pgroup)
{
  fc_assert_ret_val(nullptr != pgroup, 0);
  return pgroup - nation_groups;
}

/**
   Return the nation group index.
 */
int nation_group_number(const struct nation_group *pgroup)
{
  return nation_group_index(pgroup);
}

/**
   Add new group into the array of groups.
 */
struct nation_group *nation_group_new(const char *name)
{
  struct nation_group *pgroup;

  if (MAX_NUM_NATION_GROUPS <= num_nation_groups) {
    qCritical("Too many nation groups (%d is the maximum).",
              MAX_NUM_NATION_GROUPS);
    return nullptr;
  }

  // Print the name and truncate if needed.
  pgroup = nation_groups + num_nation_groups;
  name_set(&pgroup->name, nullptr, name);
  if (nullptr != nation_group_by_rule_name(rule_name_get(&pgroup->name))) {
    qCritical("Duplicate nation group name %s.",
              rule_name_get(&pgroup->name));
    return nullptr;
  }

  if (nullptr != nation_set_by_rule_name(rule_name_get(&pgroup->name))) {
    qCritical("Nation group name %s is already used for a set.",
              rule_name_get(&pgroup->name));
    return nullptr;
  }

  if (is_server()) {
    pgroup->server.match = 0;
  }
  num_nation_groups++;

  return pgroup;
}

/**
   Return the nation group with the given index.

   This function returns nullptr for an out-of-range index (some callers
   rely on this).
 */
struct nation_group *nation_group_by_number(int id)
{
  if (id < 0 || id >= num_nation_groups) {
    return nullptr;
  }
  return nation_groups + id;
}

/**
   Return the nation group that has the given (untranslated) rule name.
   Returns nullptr if no group is found.
 */
struct nation_group *nation_group_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  nation_groups_iterate(pgroup)
  {
    if (0 == fc_strcasecmp(rule_name_get(&pgroup->name), qname)) {
      return pgroup;
    }
  }
  nation_groups_iterate_end;

  return nullptr;
}

/**
   Set whether this group should appear in the nation selection UI.
 */
void nation_group_set_hidden(struct nation_group *pgroup, bool hidden)
{
  fc_assert_ret(nullptr != pgroup);
  pgroup->hidden = hidden;
}

/**
   Set how much the AI will try to select a nation in the same group.
   Server only function.
 */
void nation_group_set_match(struct nation_group *pgroup, int match)
{
  fc_assert_ret(is_server());
  fc_assert_ret(nullptr != pgroup);
  pgroup->server.match = match;
}

/**
   Return whether this group should appear in the nation selection UI.
 */
bool is_nation_group_hidden(struct nation_group *pgroup)
{
  fc_assert_ret_val(nullptr != pgroup, true);
  return pgroup->hidden;
}

/**
   Return the untranslated name of a nation group (including qualifier,
   if any).
   You usually want nation_group_rule_name() instead.
   You don't have to free the return pointer.
 */
const char *nation_group_untranslated_name(const struct nation_group *pgroup)
{
  fc_assert_ret_val(nullptr != pgroup, nullptr);
  return untranslated_name(&pgroup->name);
}

/**
   Return the (untranslated) rule name of a nation group.
   You don't have to free the return pointer.
 */
const char *nation_group_rule_name(const struct nation_group *pgroup)
{
  fc_assert_ret_val(nullptr != pgroup, nullptr);

  return rule_name_get(&pgroup->name);
}

/**
   Return the translated name of a nation group.
   You don't have to free the return pointer.
 */
const char *nation_group_name_translation(const struct nation_group *pgroup)
{
  fc_assert_ret_val(nullptr != pgroup, nullptr);
  return name_translation_get(&pgroup->name);
}

/**
   Check if the given nation is in a given group
 */
bool nation_is_in_group(const struct nation_type *pnation,
                        const struct nation_group *pgroup)
{
  fc_assert_ret_val(nullptr != pnation, false);

  nation_group_list_iterate(pnation->groups, agroup)
  {
    if (agroup == pgroup) {
      return true;
    }
  }
  nation_group_list_iterate_end;
  return false;
}

/****************************************************************************
  Nation group iterator.
****************************************************************************/
struct nation_group_iter {
  struct iterator vtable;
  struct nation_group *p, *end;
};
#define NATION_GROUP_ITER(p) ((struct nation_group_iter *) (p))

/**
   Implementation of iterator 'sizeof' function.
 */
size_t nation_group_iter_sizeof()
{
  return sizeof(struct nation_group_iter);
}

/**
   Implementation of iterator 'next' function.
 */
static void nation_group_iter_next(struct iterator *iter)
{
  NATION_GROUP_ITER(iter)->p++;
}

/**
   Implementation of iterator 'get' function.
 */
static void *nation_group_iter_get(const struct iterator *iter)
{
  return NATION_GROUP_ITER(iter)->p;
}

/**
   Implementation of iterator 'valid' function.
 */
static bool nation_group_iter_valid(const struct iterator *iter)
{
  struct nation_group_iter *it = NATION_GROUP_ITER(iter);
  return it->p < it->end;
}

/**
   Implementation of iterator 'init' function.
 */
struct iterator *nation_group_iter_init(struct nation_group_iter *it)
{
  it->vtable.next = nation_group_iter_next;
  it->vtable.get = nation_group_iter_get;
  it->vtable.valid = nation_group_iter_valid;
  it->p = nation_groups;
  it->end = nation_groups + nation_group_count();
  return ITERATOR(it);
}

/*
   Initializes all nation set/group data.
 */
void nation_sets_groups_init() { num_nation_sets = num_nation_groups = 0; }

/**
   Frees and resets all nation set/group data.
 */
void nation_sets_groups_free() { num_nation_sets = num_nation_groups = 0; }

/**
   Return TRUE iff the editor is allowed to edit the player's nation in
   pregame.
 */
bool can_conn_edit_players_nation(const struct connection *pconn,
                                  const struct player *pplayer)
{
  return (can_conn_edit(pconn)
          || (game.info.is_new_game
              && ((!pconn->observer && pconn->playing == pplayer)
                  || pconn->access_level >= ALLOW_CTRL)));
}

/**
   Returns how much two nations look good in the same game.
   Nations in the same group are considered to match, if that nation group
   has a 'match' greater than zero.
   Negative return value means that we really really don't want these
   nations together. This is dictated by "conflicts_with" in individual
   nation definitions. (If 'ignore_conflicts' is set, conflicts are not
   taken into account at all.)
   Server side function.
 */
int nations_match(const struct nation_type *pnation1,
                  const struct nation_type *pnation2, bool ignore_conflicts)
{
  bool in_conflict = false;
  int sum = 0;

  fc_assert_ret_val(is_server(), -1);
  NATION_CHECK(pnation1, return -1);
  NATION_CHECK(pnation2, return -1);

  if (!ignore_conflicts) {
    nation_list_iterate(pnation1->server.conflicts_with, pnation0)
    {
      if (pnation0 == pnation2) {
        in_conflict = true;
        sum = 1; // Be sure to returns something negative.
        break;
      }
    }
    nation_list_iterate_end;

    if (!in_conflict) {
      nation_list_iterate(pnation2->server.conflicts_with, pnation0)
      {
        if (pnation0 == pnation1) {
          in_conflict = true;
          sum = 1; // Be sure to returns something negative.
          break;
        }
      }
      nation_list_iterate_end;
    }
  }

  nation_group_list_iterate(pnation1->groups, pgroup)
  {
    if (nation_is_in_group(pnation2, pgroup)) {
      sum += pgroup->server.match;
    }
  }
  nation_group_list_iterate_end;

  return (in_conflict ? -sum : sum);
}
