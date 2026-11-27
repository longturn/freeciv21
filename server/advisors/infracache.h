// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// common
#include "fc_types.h"

struct player;

struct adv_city {
  /* Used for caching change in value from a worker performing
   * a particular activity on a particular tile. */
  struct worker_activity_cache *act_cache;
  int act_cache_radius_sq;

  // building desirabilities - easiest to handle them here -- Syela
  /* The units of building_want are output
   * (shields/gold/luxuries) multiplied by a priority
   * (SHIELD_WEIGHTING, etc or ai->shields_priority, etc)
   */
  adv_want building_want[B_LAST];

  int downtown; /* distance from neighbours, for locating
                   wonders wisely */
};

void adv_city_alloc(struct city *pcity);
void adv_city_free(struct city *pcity);

void initialize_infrastructure_cache(struct player *pplayer);

void adv_city_update(struct city *pcity);

int city_tile_value(const struct city *pcity, const struct tile *ptile,
                    int foodneed, int prodneed);

void adv_city_worker_act_set(struct city *pcity, int city_tile_index,
                             enum unit_activity act_id, int value);
int adv_city_worker_act_get(const struct city *pcity, int city_tile_index,
                            enum unit_activity act_id);
void adv_city_worker_extra_set(struct city *pcity, int city_tile_index,
                               const struct extra_type *pextra, int value);
int adv_city_worker_extra_get(const struct city *pcity, int city_tile_index,
                              const struct extra_type *pextra);
void adv_city_worker_rmextra_set(struct city *pcity, int city_tile_index,
                                 const struct extra_type *pextra, int value);
int adv_city_worker_rmextra_get(const struct city *pcity,
                                int city_tile_index,
                                const struct extra_type *pextra);
