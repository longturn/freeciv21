// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "bitvector.h"

// common
#include "extras.h"
#include "fc_types.h"
#include "player.h"
#include "road.h"
#include "terrain.h"
#include "unitlist.h"

// Qt
#include <QtContainerFwd> // QStringList = QList<QString>

/* Convenience macro for accessing tile coordinates.  This should only be
 * used for debugging. */
#define TILE_XY(ptile)                                                      \
  ((ptile) ? index_to_map_pos_x(tile_index(ptile)) : -1),                   \
      ((ptile) ? index_to_map_pos_y(tile_index(ptile)) : -1)

#define TILE_INDEX_NONE (-1)

struct tile {
  int index; /* Index coordinate of the tile. Used to calculate (x, y) pairs
              * (index_to_map_pos()) and (nat_x, nat_y) pairs
              * (index_to_native_pos()). */
  Continent_id continent;
  bv_extras extras;
  struct extra_type *resource; // nullptr for no resource
  struct terrain *terrain;     // nullptr for unknown tiles
  struct unit_list *units;
  struct city *worked;  // nullptr for not worked
  struct player *owner; // nullptr for not owned
  struct extra_type *placing;
  int infra_turns;
  struct player *extras_owner;
  struct tile *claimer;
  char *label; // nullptr for no label
  char *spec_sprite;
};

// 'struct tile_list' and related functions.
#define SPECLIST_TAG tile
#define SPECLIST_TYPE struct tile
#include "speclist.h"
#define tile_list_iterate(tile_list, ptile)                                 \
  TYPED_LIST_ITERATE(struct tile, tile_list, ptile)
#define tile_list_iterate_end LIST_ITERATE_END

// Tile accessor functions.
#define tile_index(_pt_) (_pt_)->index

struct city *tile_city(const struct tile *ptile);

#define tile_continent(_tile) ((_tile)->continent)
/*Continent_id tile_continent(const struct tile *ptile);*/
void tile_set_continent(struct tile *ptile, Continent_id val);

#define tile_owner(_tile) ((_tile)->owner)
/*struct player *tile_owner(const struct tile *ptile);*/
void tile_set_owner(struct tile *ptile, struct player *pplayer,
                    struct tile *claimer);
#define tile_claimer(_tile) ((_tile)->claimer)

#define tile_resource(_tile) ((_tile)->resource)
static inline bool tile_resource_is_valid(const struct tile *ptile)
{
  return ptile->resource != nullptr
         && BV_ISSET(ptile->extras, ptile->resource->id);
}
/*const struct resource *tile_resource(const struct tile *ptile);*/
void tile_set_resource(struct tile *ptile, struct extra_type *presource);

#define tile_terrain(_tile) ((_tile)->terrain)
/*struct terrain *tile_terrain(const struct tile *ptile);*/
void tile_set_terrain(struct tile *ptile, struct terrain *pterrain);

#define tile_worked(_tile) ((_tile)->worked)
// struct city *tile_worked(const struct tile *ptile);
void tile_set_worked(struct tile *ptile, struct city *pcity);

const bv_extras *tile_extras_null();
static inline const bv_extras *tile_extras(const struct tile *ptile)
{
  return &(ptile->extras);
}

bool tile_has_base_flag(const struct tile *ptile, enum base_flag_id flag);
bool tile_has_base_flag_for_unit(const struct tile *ptile,
                                 const struct unit_type *punittype,
                                 enum base_flag_id flag);
bool tile_has_refuel_extra(const struct tile *ptile,
                           const struct unit_type *punittype);
bool tile_has_native_base(const struct tile *ptile,
                          const struct unit_type *punittype);
bool tile_has_claimable_base(const struct tile *ptile,
                             const struct unit_type *punittype);
int tile_extras_defense_bonus(const struct tile *ptile,
                              const struct unit_type *punittype);
int tile_extras_class_defense_bonus(const struct tile *ptile,
                                    const struct unit_class *pclass);

bool tile_has_road(const struct tile *ptile, const struct road_type *proad);
bool tile_has_road_flag(const struct tile *ptile, enum road_flag_id flag);
int tile_roads_output_incr(const struct tile *ptile, enum output_type_id o);
int tile_roads_output_bonus(const struct tile *ptile, enum output_type_id o);
bool tile_has_river(const struct tile *tile);

bool tile_extra_apply(struct tile *ptile, const extra_type *tgt);
bool tile_extra_rm_apply(struct tile *ptile, struct extra_type *tgt);
#define tile_has_extra(ptile, pextra)                                       \
  BV_ISSET(ptile->extras, extra_index(pextra))
bool tile_has_conflicting_extra(const struct tile *ptile,
                                const struct extra_type *pextra);
bool tile_has_visible_extra(const struct tile *ptile,
                            const struct extra_type *pextra);
void tile_add_extra(struct tile *ptile, const struct extra_type *pextra);
void tile_remove_extra(struct tile *ptile, const struct extra_type *pextra);
bool tile_has_extra_flag(const struct tile *ptile, enum extra_flag_id flag);
;

// Vision related
enum known_type tile_get_known(const struct tile *ptile,
                               const struct player *pplayer);

bool tile_is_seen(const struct tile *target_tile,
                  const struct player *pow_player);

/* A somewhat arbitrary integer value.  Activity times are multiplied by
 * this amount, and divided by them later before being used.  This may
 * help to avoid rounding errors; however it should probably be removed. */
#define ACTIVITY_FACTOR 10
int tile_activity_time(enum unit_activity activity, const struct tile *ptile,
                       const struct extra_type *tgt);

// These are higher-level functions that handle side effects on the tile.
void tile_change_terrain(struct tile *ptile, struct terrain *pterrain);
bool tile_apply_activity(struct tile *ptile, Activity_type_id act,
                         struct extra_type *tgt);

/**
  Contains some information about a tile as human readable translated
  string.
 */
struct tile_info {
  explicit tile_info(const struct tile *ptile);

  QString terrain;
  QString resource;
  QStringList extras;
  QStringList nuisances;
};

#define TILE_LB_TERRAIN_RIVER (1 << 0)
#define TILE_LB_RIVER_RESOURCE (1 << 1)
#define TILE_LB_RESOURCE_POLL (1 << 2)
const char *tile_get_info_text(const tile_info &info, bool include_nuisances,
                               int linebreaks);

// Virtual tiles are tiles that do not exist on the game map.
struct tile *tile_virtual_new(const struct tile *ptile);
void tile_virtual_destroy(struct tile *vtile);
bool tile_virtual_check(const tile *vtile);

bool tile_set_label(struct tile *ptile, const char *label);
bool tile_is_placing(const struct tile *ptile);
