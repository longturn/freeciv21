// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "tile.h"

// utility
#include "bitvector.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "base.h"
#include "city.h"
#include "extras.h"
#include "fc_interface.h"
#include "fc_types.h"
#include "game.h"
#include "map.h"
#include "road.h"
#include "terrain.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"

// Qt
#include <QBitArray>
#include <QStringLiteral>
#include <QtLogging> // qDebug, qWarning, qCricital, etc

// std
#include <cstring> // str*, mem*

static bv_extras empty_extras;

#ifndef tile_index
/**
   Return the tile index.
 */
int tile_index(const struct tile *ptile) { return ptile->index; }
#endif

#ifndef tile_owner
/**
   Return the player who owns this tile (or nullptr if none).
 */
struct player *tile_owner(const struct tile *ptile) { return ptile->owner; }
#endif

#ifndef tile_claimer
/**
   Return the player who owns this tile (or nullptr if none).
 */
struct tile *tile_claimer(const struct tile *ptile)
{
  return ptile->claimer;
}
#endif

/**
   Set the owner of a tile (may be nullptr).
 */
void tile_set_owner(struct tile *ptile, struct player *pplayer,
                    struct tile *claimer)
{
  if (BORDERS_DISABLED != game.info.borders
      // City tiles are always owned by the city owner.
      || (tile_city(ptile) != nullptr || ptile->owner != nullptr)) {
    ptile->owner = pplayer;
    ptile->claimer = claimer;
  }
}

/**
   Return the city on this tile (or nullptr), checking for city center.
 */
struct city *tile_city(const struct tile *ptile)
{
  if (!ptile) {
    return nullptr;
  }

  struct city *pcity = ptile->worked;

  if (nullptr != pcity && is_city_center(pcity, ptile)) {
    return pcity;
  }
  return nullptr;
}

#ifndef tile_worked
/**
   Return any city working the specified tile (or nullptr).
 */
struct city *tile_worked(const struct tile *ptile) { return ptile->worked; }
#endif

/**
   Set the city/worker on the tile (may be nullptr).
 */
void tile_set_worked(struct tile *ptile, struct city *pcity)
{
  ptile->worked = pcity;
}

#ifndef tile_terrain
/**
   Return the terrain at the specified tile.
 */
struct terrain *tile_terrain(const struct tile *ptile)
{
  return ptile->terrain;
}
#endif

/**
   Set the given terrain at the specified tile.
 */
void tile_set_terrain(struct tile *ptile, struct terrain *pterrain)
{
  /* The terrain change is valid if one of the following is TRUE:
   * - pterrain is nullptr (= unknown terrain)
   * - ptile is a virtual tile
   * - pterrain does not has the flag TER_NO_CITIES
   * - there is no city on ptile
   * - client may have had tile fogged and is receiving terrain change before
   *   city removal
   * This should be read as: The terrain change is INVALID if a terrain with
   * the flag TER_NO_CITIES is given for a real tile with a city (i.e. all
   * check evaluate to TRUE). */
  fc_assert_msg(
      nullptr == pterrain || !is_server() || tile_virtual_check(ptile)
          || !terrain_has_flag(pterrain, TER_NO_CITIES)
          || nullptr == tile_city(ptile),
      "At (%d, %d), the terrain \"%s\" (nb %d) doesn't "
      "support cities, whereas \"%s\" (nb %d) is built there.",
      TILE_XY(ptile), terrain_rule_name(pterrain), terrain_number(pterrain),
      city_name_get(tile_city(ptile)), tile_city(ptile)->id);

  ptile->terrain = pterrain;
  if (ptile->resource != nullptr) {
    if (nullptr != pterrain
        && terrain_has_resource(pterrain, ptile->resource)) {
      BV_SET(ptile->extras, extra_index(ptile->resource));
    } else {
      BV_CLR(ptile->extras, extra_index(ptile->resource));
    }
  }
}

/**
   Returns a bit vector of the extras present at nullptr tile.
 */
const bv_extras *tile_extras_null()
{
  static bool empty_cleared = false;

  if (!empty_cleared) {
    BV_CLR_ALL(empty_extras);
    empty_cleared = true;
  }

  return &(empty_extras);
}

/**
   Check if tile contains base providing effect
 */
bool tile_has_base_flag(const struct tile *ptile, enum base_flag_id flag)
{
  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    struct base_type *pbase = extra_base_get(pextra);

    if (tile_has_extra(ptile, pextra) && base_has_flag(pbase, flag)) {
      return true;
    }
  }
  extra_type_by_cause_iterate_end;

  return false;
}

/**
   Check if tile contains base providing effect for unit
 */
bool tile_has_base_flag_for_unit(const struct tile *ptile,
                                 const struct unit_type *punittype,
                                 enum base_flag_id flag)
{
  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    struct base_type *pbase = extra_base_get(pextra);

    if (tile_has_extra(ptile, pextra)
        && base_has_flag_for_utype(pbase, flag, punittype)) {
      return true;
    }
  }
  extra_type_by_cause_iterate_end;

  return false;
}

/**
   Check if tile contains base providing effect for unit
 */
bool tile_has_claimable_base(const struct tile *ptile,
                             const struct unit_type *punittype)
{
  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    struct base_type *pbase = extra_base_get(pextra);

    if (tile_has_extra(ptile, pextra) && territory_claiming_base(pbase)
        && is_native_extra_to_uclass(pextra, utype_class(punittype))) {
      return true;
    }
  }
  extra_type_by_cause_iterate_end;

  return false;
}

/**
   Calculate defense bonus given for unit type by bases and roads
 */
int tile_extras_defense_bonus(const struct tile *ptile,
                              const struct unit_type *punittype)
{
  return tile_extras_class_defense_bonus(ptile, utype_class(punittype));
}

/**
   Calculate defense bonus given for unit class by extras.
 */
int tile_extras_class_defense_bonus(const struct tile *ptile,
                                    const struct unit_class *pclass)
{
  int natural_bonus = 0;
  int fortification_bonus = 0;
  int total_bonus;

  extra_type_by_cause_iterate(EC_NATURAL_DEFENSIVE, pextra)
  {
    if (tile_has_extra(ptile, pextra)
        && is_native_extra_to_uclass(pextra, pclass)) {
      natural_bonus += pextra->defense_bonus;
    }
  }
  extra_type_by_cause_iterate_end;

  extra_type_by_cause_iterate(EC_DEFENSIVE, pextra)
  {
    if (tile_has_extra(ptile, pextra)
        && is_native_extra_to_uclass(pextra, pclass)) {
      fortification_bonus += pextra->defense_bonus;
    }
  }
  extra_type_by_cause_iterate_end;

  total_bonus =
      (100 + natural_bonus) * (100 + fortification_bonus) / 100 - 100;

  return total_bonus;
}

/**
   Calculate output increment given by roads
 */
int tile_roads_output_incr(const struct tile *ptile, enum output_type_id o)
{
  int const_incr = 0;
  int incr = 0;

  extra_type_by_cause_iterate(EC_ROAD, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      struct road_type *proad = extra_road_get(pextra);

      const_incr += proad->tile_incr_const[o];
      incr += proad->tile_incr[o];
    }
  }
  extra_type_by_cause_iterate_end;

  return const_incr
         + incr * tile_terrain(ptile)->road_output_incr_pct[o] / 100;
}

/**
   Calculate output bonus given by roads
 */
int tile_roads_output_bonus(const struct tile *ptile, enum output_type_id o)
{
  int bonus = 0;

  extra_type_by_cause_iterate(EC_ROAD, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      struct road_type *proad = extra_road_get(pextra);

      bonus += proad->tile_bonus[o];
    }
  }
  extra_type_by_cause_iterate_end;

  return bonus;
}

/**
   Check if tile contains refuel extra native for unit
 */
bool tile_has_refuel_extra(const struct tile *ptile,
                           const struct unit_type *punittype)
{
  extra_type_iterate(pextra)
  {
    if (tile_has_extra(ptile, pextra) && extra_has_flag(pextra, EF_REFUEL)
        && is_native_extra_to_utype(pextra, punittype)) {
      return true;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Check if tile contains base native for unit
 */
bool tile_has_native_base(const struct tile *ptile,
                          const struct unit_type *punittype)
{
  extra_type_by_cause_iterate(EC_BASE, pextra)
  {
    if (tile_has_extra(ptile, pextra)
        && is_native_extra_to_utype(pextra, punittype)) {
      return true;
    }
  }
  extra_type_by_cause_iterate_end;

  return false;
}

#ifndef tile_resource
/**
   Return the resource at the specified tile.
 */
const struct resource_type *tile_resource(const struct tile *ptile)
{
  return ptile->resource;
}
#endif

/**
   Set the given resource at the specified tile.
 */
void tile_set_resource(struct tile *ptile, struct extra_type *presource)
{
  if (presource == ptile->resource) {
    return; // No change
  }

  if (ptile->resource != nullptr) {
    tile_remove_extra(ptile, ptile->resource);
  }
  if (presource != nullptr) {
    if (ptile->terrain && terrain_has_resource(ptile->terrain, presource)) {
      tile_add_extra(ptile, presource);
    }
  }

  ptile->resource = presource;
}

#ifndef tile_continent
/**
   Return the continent ID of the tile.  Typically land has a positive
   continent number and ocean has a negative number; no tile should have
   a 0 continent number.
 */
Continent_id tile_continent(const struct tile *ptile)
{
  return ptile->continent;
}
#endif

/**
   Set the continent ID of the tile.  See tile_continent.
 */
void tile_set_continent(struct tile *ptile, Continent_id val)
{
  ptile->continent = val;
}

/**
   Return a known_type enumeration value for the tile.

   Note that the client only has known data about its own player.
 */
enum known_type tile_get_known(const struct tile *ptile,
                               const struct player *pplayer)
{
  if (tile_virtual_check(ptile)) {
    return TILE_KNOWN_SEEN;
  }
  if (!pplayer->tile_known->at(tile_index(ptile))) {
    return TILE_UNKNOWN;
  } else if (!fc_funcs->player_tile_vision_get(ptile, pplayer, V_MAIN)) {
    return TILE_KNOWN_UNSEEN;
  } else {
    return TILE_KNOWN_SEEN;
  }
}

/**
   Returns TRUE iff the target_tile is seen by pow_player.
 */
bool tile_is_seen(const struct tile *target_tile,
                  const struct player *pow_player)
{
  return tile_get_known(target_tile, pow_player) == TILE_KNOWN_SEEN;
}

/**
   Time to complete the given activity on the given tile.

   See also action_get_act_time()
 */
int tile_activity_time(enum unit_activity activity, const struct tile *ptile,
                       const struct extra_type *tgt)
{
  struct terrain *pterrain = tile_terrain(ptile);

  // Make sure nobody uses old activities
  fc_assert_ret_val(activity != ACTIVITY_FORTRESS
                        && activity != ACTIVITY_AIRBASE,
                    FC_INFINITY);

  switch (activity) {
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_PILLAGE:
    return terrain_extra_removal_time(pterrain, activity, tgt)
           * ACTIVITY_FACTOR;
  case ACTIVITY_TRANSFORM:
    return pterrain->transform_time * ACTIVITY_FACTOR;
  case ACTIVITY_CULTIVATE:
    return pterrain->cultivate_time * ACTIVITY_FACTOR;
  case ACTIVITY_PLANT:
    return pterrain->plant_time * ACTIVITY_FACTOR;
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_MINE:
  case ACTIVITY_BASE:
  case ACTIVITY_GEN_ROAD:
    return terrain_extra_build_time(pterrain, activity, tgt)
           * ACTIVITY_FACTOR;
  default:
    return 0;
  }
}

/**
   Create extra to tile.
 */
static void tile_create_extra(struct tile *ptile, const extra_type *pextra)
{
  if (fc_funcs->create_extra != nullptr) {
    // Assume callback calls tile_add_extra() itself.
    fc_funcs->create_extra(ptile, pextra, nullptr);
  } else {
    tile_add_extra(ptile, pextra);
  }
}

/**
   Destroy extra from tile.
 */
static void tile_destroy_extra(struct tile *ptile, struct extra_type *pextra)
{
  if (fc_funcs->destroy_extra != nullptr) {
    // Assume callback calls tile_remove_extra() itself.
    fc_funcs->destroy_extra(ptile, pextra);
  } else {
    tile_remove_extra(ptile, pextra);
  }
}

/**
   Change the terrain to the given type.  This does secondary tile updates to
   the tile (as will happen when mining/irrigation/transforming changes the
   tile's terrain).
 */
void tile_change_terrain(struct tile *ptile, struct terrain *pterrain)
{
  tile_set_terrain(ptile, pterrain);

  // Remove unsupported extras
  extra_type_iterate(pextra)
  {
    if (tile_has_extra(ptile, pextra)
        && (!is_native_tile_to_extra(pextra, ptile)
            || extra_has_flag(pextra, EF_TERR_CHANGE_REMOVES))) {
      tile_destroy_extra(ptile, pextra);
    }
  }
  extra_type_iterate_end;
}

/**
   Recursively add all extra dependencies to add given extra.
 */
static bool add_recursive_extras(struct tile *ptile,
                                 const extra_type *pextra, int rec)
{
  if (rec > MAX_EXTRA_TYPES) {
    // Infinite recursion
    return false;
  }

  // First place dependency extras
  extra_deps_iterate(&(pextra->reqs), pdep)
  {
    if (!tile_has_extra(ptile, pdep)) {
      add_recursive_extras(ptile, pdep, rec + 1);
    }
  }
  extra_deps_iterate_end;

  // Is tile native for extra after that?
  if (!is_native_tile_to_extra(pextra, ptile)) {
    return false;
  }

  tile_create_extra(ptile, pextra);

  return true;
}

/**
   Recursively remove all extras depending on given extra.
 */
static bool rm_recursive_extras(struct tile *ptile,
                                struct extra_type *pextra, int rec)
{
  if (rec > MAX_EXTRA_TYPES) {
    // Infinite recursion
    return false;
  }

  extra_type_iterate(pdepending)
  {
    if (tile_has_extra(ptile, pdepending)) {
      extra_deps_iterate(&(pdepending->reqs), pdep)
      {
        if (pdep == pextra) {
          // Depends on what we are going to remove
          if (!rm_recursive_extras(ptile, pdepending, rec + 1)) {
            return false;
          }
        }
      }
      extra_deps_iterate_end;
    }
  }
  extra_type_iterate_end;

  tile_destroy_extra(ptile, pextra);

  return true;
}

/**
   Add extra and adjust other extras accordingly.

   If not all necessary adjustments can be done, returns FALSE.
   When problem occurs, changes to tile extras are not reverted.
   Pass virtual tile to the function if you are not sure it will success
   and don't want extras adjusted at all in case of failure.
 */
bool tile_extra_apply(struct tile *ptile, const extra_type *tgt)
{
  // Add extra with its dependencies
  if (!add_recursive_extras(ptile, tgt, 0)) {
    return false;
  }

  // Remove conflicting extras
  extra_type_iterate(pextra)
  {
    if (tile_has_extra(ptile, pextra) && !can_extras_coexist(pextra, tgt)) {
      tile_destroy_extra(ptile, pextra);
    }
  }
  extra_type_iterate_end;

  return true;
}

/**
   Remove extra and adjust other extras accordingly.

   If not all necessary adjustments can be done, returns FALSE.
   When problem occurs, changes to tile extras are not reverted.
   Pass virtual tile to the function if you are not sure it will success
   and don't want extras adjusted at all in case of failure.
 */
bool tile_extra_rm_apply(struct tile *ptile, struct extra_type *tgt)
{
  // Remove extra with everything depending on it.
  return rm_recursive_extras(ptile, tgt, 0);
}

/**
   Build irrigation on the tile.  This may change the extras of the tile
   or change the terrain type itself.
 */
static void tile_irrigate(struct tile *ptile, struct extra_type *tgt)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain == pterrain->irrigation_result) {
    /* Ideally activity should already been cancelled before nullptr tgt
     * gets this far, but it's possible that terrain got changed from
     * one that gets transformed by irrigation (-> nullptr tgt) to one
     * that does not (-> nullptr tgt illegal) since legality of the action
     * was last checked */
    if (tgt != nullptr) {
      tile_extra_apply(ptile, tgt);
    }
  } else if (pterrain->irrigation_result) {
    tile_change_terrain(ptile, pterrain->irrigation_result);
  }
}

/**
   Build a mine on the tile.  This may change the extras of the tile
   or change the terrain type itself.
 */
static void tile_mine(struct tile *ptile, struct extra_type *tgt)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain == pterrain->mining_result) {
    /* Ideally activity should already been cancelled before nullptr tgt
     * gets this far, but it's possible that terrain got changed from
     * one that gets transformed by mining (-> nullptr tgt) to one
     * that does not (-> nullptr tgt illegal) since legality of the action
     * was last checked */
    if (tgt != nullptr) {
      tile_extra_apply(ptile, tgt);
    }
  } else if (pterrain->mining_result) {
    tile_change_terrain(ptile, pterrain->mining_result);
  }
}

/**
   Transform (ACTIVITY_TRANSFORM) the tile. This usually changes the tile's
   terrain type.
 */
static void tile_transform(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain->transform_result != T_NONE) {
    tile_change_terrain(ptile, pterrain->transform_result);
  }
}

/**
   Plant (ACTIVITY_PLANT) the tile. This usually changes the tile's
   terrain type.
 */
static void tile_plant(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain->mining_result != T_NONE
      && pterrain->mining_result != pterrain) {
    tile_change_terrain(ptile, pterrain->mining_result);
  }
}

/**
   Cultivate (ACTIVITY_CULTIVATE) the tile. This usually changes the tile's
   terrain type.
 */
static void tile_cultivate(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (pterrain->irrigation_result != T_NONE
      && pterrain->irrigation_result != pterrain) {
    tile_change_terrain(ptile, pterrain->irrigation_result);
  }
}

/**
   Apply an activity (Activity_type_id, e.g., ACTIVITY_TRANSFORM) to a tile.
   Return false if there was a error or if the activity is not implemented
   by this function.
 */
bool tile_apply_activity(struct tile *ptile, Activity_type_id act,
                         struct extra_type *tgt)
{
  /* FIXME: for irrigate, mine, and transform we always return TRUE
   * even if the activity fails. */
  switch (act) {
  case ACTIVITY_MINE:
    tile_mine(ptile, tgt);
    return true;

  case ACTIVITY_IRRIGATE:
    tile_irrigate(ptile, tgt);
    return true;

  case ACTIVITY_TRANSFORM:
    tile_transform(ptile);
    return true;

  case ACTIVITY_CULTIVATE:
    tile_cultivate(ptile);
    return true;

  case ACTIVITY_PLANT:
    tile_plant(ptile);
    return true;

  case ACTIVITY_OLD_ROAD:
  case ACTIVITY_OLD_RAILROAD:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_AIRBASE:
    fc_assert(false);
    return false;

  case ACTIVITY_PILLAGE:
  case ACTIVITY_BASE:
  case ACTIVITY_GEN_ROAD:
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT:
    // do nothing  - not implemented
    return false;

  case ACTIVITY_IDLE:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_SENTRY:
  case ACTIVITY_GOTO:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_CONVERT:
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_FORTIFYING:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
    /* do nothing - these activities have no effect
       on terrain type or tile extras */
    return false;
  }
  fc_assert(false);
  return false;
}

/**
   Add one entry about pollution situation to buffer.
   Return if there has been any pollution (even prior calling this)
 */
static bool tile_info_pollution(QString &buf, const QString &pollution,
                                bool prevp, bool linebreak)
{
  if (!prevp) {
    if (linebreak) {
      buf += QStringLiteral("\n[");
    } else {
      buf += QStringLiteral(" [");
    }
  } else {
    buf += QStringLiteral("/");
  }

  buf += pollution;

  return true;
}

/**
   Construct a tile_info struct containing information about ptile.
 */
tile_info::tile_info(const struct tile *ptile)
{
  terrain = QString(terrain_name_translation(tile_terrain(ptile)));

  if (tile_resource_is_valid(ptile)) {
    resource = QString(extra_name_translation(ptile->resource));
  }

  extra_type_iterate(pextra)
  {
    if (pextra->category == ECAT_NATURAL
        && tile_has_visible_extra(ptile, pextra)) {
      extras.append(QString(extra_name_translation(pextra)));
    }
  }
  extra_type_iterate_end;

  extra_type_iterate(pextra)
  {
    if (pextra->category == ECAT_NUISANCE
        && tile_has_visible_extra(ptile, pextra)) {
      nuisances.append(QString(extra_name_translation(pextra)));
    }
  }
  extra_type_iterate_end;
}

/**
   Return a (static) string with tile name describing terrain and
   extras of some categories.
   If include_nuisances is set, pollution and nuclear fallout will be
   ignored.

   Examples:
     "Hills"
     "Hills (Coals)"
     "Hills (Coals) [Pollution]"
 */
const char *tile_get_info_text(const tile_info &info, bool include_nuisances,
                               int linebreaks)
{
  bool pollution;
  bool lb = false;

  QString s = info.terrain;
  if (linebreaks & TILE_LB_TERRAIN_RIVER) {
    // Linebreak needed before next text
    lb = true;
  }

  for (auto extra : info.extras) {
    if (lb) {
      s += QStringLiteral("\n");
      lb = false;
    } else {
      s += QStringLiteral("/");
    }
    s += extra;
  }
  if (linebreaks & TILE_LB_RIVER_RESOURCE) {
    // New linebreak requested
    lb = true;
  }

  if (!info.resource.isEmpty()) {
    if (lb) {
      s += QStringLiteral("\n");
      lb = false;
    } else {
      s += QStringLiteral(" ");
    }
    s += QString("(%1)").arg(info.resource);
  }
  if (linebreaks & TILE_LB_RESOURCE_POLL) {
    // New linebreak requested
    lb = true;
  }

  if (include_nuisances) {
    pollution = false;
    for (auto nuiscance : info.nuisances) {
      pollution = tile_info_pollution(s, nuiscance, pollution, lb);
    }
    if (pollution) {
      s += QStringLiteral("]");
    }
  }

  static char ret[256];
  fc_strlcpy(ret, s.toStdString().c_str(), sizeof(ret));
  return ret;
}

/**
   Returns TRUE if the given tile has a road of given type on it.
 */
bool tile_has_road(const struct tile *ptile, const struct road_type *proad)
{
  return tile_has_extra(ptile, road_extra_get(proad));
}

/**
   Tile has any river type
 */
bool tile_has_river(const struct tile *ptile)
{
  // TODO: Have a list of rivers and iterate only that
  extra_type_by_cause_iterate(EC_ROAD, priver)
  {
    if (tile_has_extra(ptile, priver)
        && road_has_flag(extra_road_get(priver), RF_RIVER)) {
      return true;
    }
  }
  extra_type_by_cause_iterate_end;

  return false;
}

/**
   Check if tile contains road providing effect
 */
bool tile_has_road_flag(const struct tile *ptile, enum road_flag_id flag)
{
  extra_type_by_cause_iterate(EC_ROAD, pextra)
  {
    if (tile_has_extra(ptile, pextra)) {
      struct road_type *proad = extra_road_get(pextra);

      if (road_has_flag(proad, flag)) {
        return true;
      }
    }
  }
  extra_type_by_cause_iterate_end;

  return false;
}

/**
   Check if tile contains extra providing effect
 */
bool tile_has_extra_flag(const struct tile *ptile, enum extra_flag_id flag)
{
  extra_type_iterate(pextra)
  {
    if (tile_has_extra(ptile, pextra) && extra_has_flag(pextra, flag)) {
      return true;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Returns TRUE if the given tile has a extra conflicting with the given one.
 */
bool tile_has_conflicting_extra(const struct tile *ptile,
                                const struct extra_type *pextra)
{
  extra_type_iterate(pconfl)
  {
    if (BV_ISSET(pextra->conflicts, extra_index(pconfl))
        && tile_has_extra(ptile, pconfl)) {
      return true;
    }
  }
  extra_type_iterate_end;

  return false;
}

/**
   Returns TRUE if the given tile has a road of given type on it.
 */
bool tile_has_visible_extra(const struct tile *ptile,
                            const struct extra_type *pextra)
{
  bool hidden = false;

  if (!BV_ISSET(ptile->extras, extra_index(pextra))) {
    return false;
  }

  extra_type_iterate(top)
  {
    int topi = extra_index(top);

    if (BV_ISSET(pextra->hidden_by, topi) && BV_ISSET(ptile->extras, topi)) {
      hidden = true;
      break;
    }
  }
  extra_type_iterate_end;

  return !hidden;
}

/**
   Adds extra to tile
 */
void tile_add_extra(struct tile *ptile, const struct extra_type *pextra)
{
  if (pextra != nullptr) {
    BV_SET(ptile->extras, extra_index(pextra));
  }
}

/**
   Removes extra from tile if such exist
 */
void tile_remove_extra(struct tile *ptile, const struct extra_type *pextra)
{
  if (pextra != nullptr) {
    BV_CLR(ptile->extras, extra_index(pextra));
  }
}

/**
   Returns a virtual tile. If ptile is given, the properties of this tile are
   copied, else it is completely blank (except for the unit list
   vtile->units, which is created for you). Be sure to call tile_virtual_free
   on it when it is no longer needed.
 */
struct tile *tile_virtual_new(const struct tile *ptile)
{
  struct tile *vtile;

  vtile = new tile[1]();

  // initialise some values
  vtile->index = TILE_INDEX_NONE;
  vtile->continent = -1;

  BV_CLR_ALL(vtile->extras);
  vtile->resource = nullptr;
  vtile->terrain = nullptr;
  vtile->units = unit_list_new();
  vtile->worked = nullptr;
  vtile->owner = nullptr;
  vtile->placing = nullptr;
  vtile->extras_owner = nullptr;
  vtile->claimer = nullptr;
  vtile->spec_sprite = nullptr;

  if (ptile) {
    /* Used by is_city_center to give virtual tiles the output bonuses
     * they deserve. */
    vtile->index = tile_index(ptile);

    // Copy all but the unit list.
    extra_type_iterate(pextra)
    {
      if (BV_ISSET(ptile->extras, extra_number(pextra))) {
        BV_SET(vtile->extras, extra_number(pextra));
      }
    }
    extra_type_iterate_end;

    vtile->resource = ptile->resource;
    vtile->terrain = ptile->terrain;
    vtile->worked = ptile->worked;
    vtile->owner = ptile->owner;
    vtile->extras_owner = ptile->extras_owner;
    vtile->claimer = ptile->claimer;
    vtile->spec_sprite = nullptr;
  }

  return vtile;
}

/**
   Frees all memory used by the virtual tile, including freeing virtual
   units in the tile's unit list and the virtual city on this tile if one
   exists.

   NB: Do not call this on real tiles!
 */
void tile_virtual_destroy(struct tile *vtile)
{
  struct city *vcity;

  if (!vtile) {
    return;
  }

  if (vtile->units) {
    unit_list_iterate(vtile->units, vunit)
    {
      if (unit_is_virtual(vunit)) {
        unit_virtual_destroy(vunit);
      }
    }
    unit_list_iterate_end;
    unit_list_destroy(vtile->units);
    vtile->units = nullptr;
  }

  vcity = tile_city(vtile);
  if (vcity) {
    if (city_is_virtual(vcity)) {
      destroy_city_virtual(vcity);
    }
    tile_set_worked(vtile, nullptr);
  }

  delete[] vtile;
}

/**
   Check if the given tile is a virtual one or not.
 */
bool tile_virtual_check(const tile *vtile)
{
  int tindex;

  if (!vtile || map_is_empty()) {
    return false;
  } else if (tile_index(vtile) == TILE_INDEX_NONE) {
    return true;
  }

  tindex = tile_index(vtile);
  fc_assert_ret_val(0 <= tindex && tindex < map_num_tiles(), false);

  return (vtile != wld.map.tiles + tindex);
}

/**
   Sets label for tile. Returns whether label changed.
 */
bool tile_set_label(struct tile *ptile, const char *label)
{
  bool changed = false;

  // Handle empty label as nullptr label
  if (label != nullptr && label[0] == '\0') {
    label = nullptr;
  }

  if (ptile->label != nullptr) {
    if (label == nullptr) {
      changed = true;
    } else if (strcmp(ptile->label, label)) {
      changed = true;
    }
    delete[] ptile->label;
    ptile->label = nullptr;
  } else if (label != nullptr) {
    changed = true;
  }

  if (label != nullptr) {
    if (strlen(label) >= MAX_LEN_MAP_LABEL) {
      qCritical("Overlong map label '%s'", label);
    }
    ptile->label = fc_strdup(label);
  }

  return changed;
}

/**
   Is there a placing ongoing?
 */
bool tile_is_placing(const struct tile *ptile)
{
  return ptile->placing != nullptr;
}
