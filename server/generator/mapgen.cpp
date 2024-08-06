/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2021 Freeciv21 and Freeciv
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

#include <fc_config.h>

#include <QBitArray>
#include <cstdlib>
#include <cstring>
// utility
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"
#include "shared.h"

// common
#include "game.h"
#include "map.h"
#include "road.h"

/* server/generator */
#include "fair_islands.h"
#include "fracture_map.h"
#include "height_map.h"
#include "islands.h"
#include "mapgen_topology.h"
#include "mapgen_utils.h"
#include "startpos.h"
#include "temperature_map.h"

#include "mapgen.h"

struct extra_type *river_types[MAX_ROAD_TYPES];
int river_type_count = 0;

static void make_huts(int number);
static void add_resources(int prob);
static void adjust_terrain_param();

#define RIVERS_MAXTRIES 32767
/* This struct includes two dynamic bitvectors. They are needed to mark
   tiles as blocked to prevent a river from falling into itself, and for
   storing rivers temporarly. */
struct river_map {
  QBitArray blocked;
  QBitArray ok;
};

static int river_test_blocked(struct river_map *privermap,
                              struct tile *ptile, struct extra_type *priver);
static int river_test_rivergrid(struct river_map *privermap,
                                struct tile *ptile,
                                struct extra_type *priver);
static int river_test_highlands(struct river_map *privermap,
                                struct tile *ptile,
                                struct extra_type *priver);
static int river_test_adjacent_ocean(struct river_map *privermap,
                                     struct tile *ptile,
                                     struct extra_type *priver);
static int river_test_adjacent_river(struct river_map *privermap,
                                     struct tile *ptile,
                                     struct extra_type *priver);
static int river_test_adjacent_highlands(struct river_map *privermap,
                                         struct tile *ptile,
                                         struct extra_type *priver);
static int river_test_swamp(struct river_map *privermap, struct tile *ptile,
                            struct extra_type *priver);
static int river_test_adjacent_swamp(struct river_map *privermap,
                                     struct tile *ptile,
                                     struct extra_type *priver);
static int river_test_height_map(struct river_map *privermap,
                                 struct tile *ptile,
                                 struct extra_type *priver);
static void river_blockmark(struct river_map *privermap, struct tile *ptile);
static bool make_river(struct river_map *privermap, struct tile *ptile,
                       struct extra_type *priver);
static void make_rivers();

static void river_types_init();

/* These are the old parameters of terrains types in %
   TODO: they depend on the hardcoded terrains */
int forest_pct = 0;
int desert_pct = 0;
int swamp_pct = 0;
int mountain_pct = 0;
int jungle_pct = 0;
int river_pct = 0;

// MISCELANEOUS (OTHER CONDITIONS)

// necessary condition of swamp placement
static int hmap_low_level = 0;
#define ini_hmap_low_level()                                                \
  {                                                                         \
    hmap_low_level =                                                        \
        (4 * swamp_pct * (hmap_max_level - hmap_shore_level)) / 100         \
        + hmap_shore_level;                                                 \
  }
// should be used after having hmap_low_level initialized
#define map_pos_is_low(ptile) ((hmap((ptile)) < hmap_low_level))

typedef enum { MC_NONE, MC_LOW, MC_NLOW } miscellaneous_c;

/**
  These functions test for conditions used in rand_map_pos_characteristic
 */

/**
   Checks if the given location satisfy some wetness condition
 */
bool test_wetness(const struct tile *ptile, wetness_c c)
{
  switch (c) {
  case WC_ALL:
    return true;
  case WC_DRY:
    return map_pos_is_dry(ptile);
  case WC_NDRY:
    return !map_pos_is_dry(ptile);
  }
  qCritical("Invalid wetness_c %d", c);
  return false;
}

/**
   Checks if the given location satisfy some miscellaneous condition
 */
static bool test_miscellaneous(const struct tile *ptile, miscellaneous_c c)
{
  switch (c) {
  case MC_NONE:
    return true;
  case MC_LOW:
    return map_pos_is_low(ptile);
  case MC_NLOW:
    return !map_pos_is_low(ptile);
  }
  qCritical("Invalid miscellaneous_c %d", c);
  return false;
}

/**
  Passed as data to rand_map_pos_filtered() by rand_map_pos_characteristic()
 */
struct DataFilter {
  wetness_c wc;
  temperature_type tc;
  miscellaneous_c mc;
};

/**
   A filter function to be passed to rand_map_pos_filtered().  See
   rand_map_pos_characteristic for more explanation.
 */
static bool condition_filter(const struct tile *ptile, const void *data)
{
  const struct DataFilter *filter = static_cast<const DataFilter *>(data);

  return not_placed(ptile) && tmap_is(ptile, filter->tc)
         && test_wetness(ptile, filter->wc)
         && test_miscellaneous(ptile, filter->mc);
}

/**
   Return random map coordinates which have some conditions and which are
   not yet placed on pmap.
   Returns FALSE if there is no such position.
 */
static struct tile *rand_map_pos_characteristic(wetness_c wc,
                                                temperature_type tc,
                                                miscellaneous_c mc)
{
  struct DataFilter filter;

  filter.wc = wc;
  filter.tc = tc;
  filter.mc = mc;

  return rand_map_pos_filtered(&(wld.map), &filter, condition_filter);
}

/**
   We don't want huge areas of hill/mountains,
   so we put in a plains here and there, where it gets too 'heigh'

   Return TRUE if the terrain at the given map position is too heigh.
 */
static bool terrain_is_too_high(struct tile *ptile, int thill, int my_height)
{
  square_iterate(&(wld.map), ptile, 1, tile1)
  {
    if (hmap(tile1) + (hmap_max_level - hmap_mountain_level) / 5 < thill) {
      return false;
    }
  }
  square_iterate_end;
  return true;
}

/**
   make_relief() will convert all squares that are higher than thill to
   mountains and hills. Note that thill will be adjusted according to
   the map.server.steepness value, so increasing map.mountains will result
   in more hills and mountains.
 */
static void make_relief()
{
  /* Calculate the mountain level.  map.server.mountains specifies the
   * percentage of land that is turned into hills and mountains. */
  hmap_mountain_level = (((hmap_max_level - hmap_shore_level)
                          * (100 - wld.map.server.steepness))
                             / 100
                         + hmap_shore_level);

  whole_map_iterate(&(wld.map), ptile)
  {
    if (not_placed(ptile)
        && ((hmap_mountain_level < hmap(ptile)
             && (fc_rand(10) > 5
                 || !terrain_is_too_high(ptile, hmap_mountain_level,
                                         hmap(ptile))))
            || area_is_too_flat(ptile, hmap_mountain_level, hmap(ptile)))) {
      if (tmap_is(ptile, TT_HOT)) {
        // Prefer hills to mountains in hot regions.
        tile_set_terrain(ptile,
                         pick_terrain(MG_MOUNTAINOUS,
                                      fc_rand(10) < 4 ? MG_UNUSED : MG_GREEN,
                                      MG_UNUSED));
      } else {
        // Prefer mountains hills to in cold regions.
        tile_set_terrain(
            ptile, pick_terrain(MG_MOUNTAINOUS, MG_UNUSED,
                                fc_rand(10) < 8 ? MG_GREEN : MG_UNUSED));
      }
      map_set_placed(ptile);
    }
  }
  whole_map_iterate_end;
}

/**
   Add frozen tiles in the arctic zone.
   If ruleset has frozen ocean, use that, else use frozen land terrains with
   appropriate texturing.
   This is used in generators 2-4.
 */
void make_polar()
{
  struct terrain *ocean = pick_ocean(TERRAIN_OCEAN_DEPTH_MAXIMUM, true);

  whole_map_iterate(&(wld.map), ptile)
  {
    if (tmap_is(ptile, TT_FROZEN)
        || (tmap_is(ptile, TT_COLD) && (fc_rand(10) > 7)
            && is_temperature_type_near(ptile, TT_FROZEN))) {
      if (ocean) {
        tile_set_terrain(ptile, ocean);
      } else {
        tile_set_terrain(ptile,
                         pick_terrain(MG_FROZEN, MG_UNUSED, MG_TROPICAL));
      }
    }
  }
  whole_map_iterate_end;
}

/**
   If separatepoles is set, return false if this tile has to keep ocean
 */
static bool ok_for_separate_poles(struct tile *ptile)
{
  if (!wld.map.server.separatepoles) {
    return true;
  }
  adjc_iterate(&(wld.map), ptile, tile1)
  {
    if (tile_continent(tile1) > 0) {
      return false;
    }
  }
  adjc_iterate_end;
  return true;
}

/**
   Place untextured land at the poles on any tile that is not already
   covered with TER_FROZEN terrain.
   This is used by generators 1 and 5.
 */
static void make_polar_land()
{
  assign_continent_numbers();

  whole_map_iterate(&(wld.map), ptile)
  {
    if ((tile_terrain(ptile) == T_UNKNOWN
         || !terrain_has_flag(tile_terrain(ptile), TER_FROZEN))
        && ((tmap_is(ptile, TT_FROZEN) && ok_for_separate_poles(ptile))
            || (tmap_is(ptile, TT_COLD) && fc_rand(10) > 7
                && is_temperature_type_near(ptile, TT_FROZEN)
                && ok_for_separate_poles(ptile)))) {
      tile_set_terrain(ptile, T_UNKNOWN);
      tile_set_continent(ptile, 0);
    }
  }
  whole_map_iterate_end;
}

/**
   Recursively generate terrains.
 */
static void place_terrain(struct tile *ptile, int diff,
                          struct terrain *pterrain, int *to_be_placed,
                          wetness_c wc, temperature_type tc,
                          miscellaneous_c mc)
{
  if (*to_be_placed <= 0) {
    return;
  }
  fc_assert_ret(not_placed(ptile));
  tile_set_terrain(ptile, pterrain);
  map_set_placed(ptile);
  (*to_be_placed)--;

  cardinal_adjc_iterate(&(wld.map), ptile, tile1)
  {
    // Check L_UNIT and H_UNIT against 0.
    int Delta =
        (abs(map_colatitude(tile1) - map_colatitude(ptile)) / MAX(L_UNIT, 1)
         + abs(hmap(tile1) - (hmap(ptile))) / MAX(H_UNIT, 1));
    if (not_placed(tile1) && tmap_is(tile1, tc) && test_wetness(tile1, wc)
        && test_miscellaneous(tile1, mc) && Delta < diff
        && fc_rand(10) > 4) {
      place_terrain(tile1, diff - 1 - Delta, pterrain, to_be_placed, wc, tc,
                    mc);
    }
  }
  cardinal_adjc_iterate_end;
}

/**
   A simple function that adds plains grassland or tundra to the
   current location.
 */
static void make_plain(struct tile *ptile, int *to_be_placed)
{
  // in cold place we get tundra instead
  if (tmap_is(ptile, TT_FROZEN)) {
    tile_set_terrain(ptile,
                     pick_terrain(MG_FROZEN, MG_UNUSED, MG_MOUNTAINOUS));
  } else if (tmap_is(ptile, TT_COLD)) {
    tile_set_terrain(ptile,
                     pick_terrain(MG_COLD, MG_UNUSED, MG_MOUNTAINOUS));
  } else {
    tile_set_terrain(ptile,
                     pick_terrain(MG_TEMPERATE, MG_GREEN, MG_MOUNTAINOUS));
  }
  map_set_placed(ptile);
  (*to_be_placed)--;
}

/**
   Make_plains converts all not yet placed terrains to plains (tundra, grass)
   used by generators 2-4
 */
void make_plains()
{
  int to_be_placed;

  whole_map_iterate(&(wld.map), ptile)
  {
    if (not_placed(ptile)) {
      to_be_placed = 1;
      make_plain(ptile, &to_be_placed);
    }
  }
  whole_map_iterate_end;
}

/**
  This place randomly a cluster of terrains with some characteristics
 */
#define PLACE_ONE_TYPE(count, alternate, ter, wc, tc, mc, weight)           \
  if ((count) > 0) {                                                        \
    struct tile *ptile;                                                     \
    /* Place some terrains */                                               \
    if ((ptile = rand_map_pos_characteristic((wc), (tc), (mc)))) {          \
      place_terrain(ptile, (weight), (ter), &(count), (wc), (tc), (mc));    \
    } else {                                                                \
      /* If rand_map_pos_temperature returns FALSE we may as well stop */   \
      /* looking for this time and go to alternate type. */                 \
      (alternate) += (count);                                               \
      (count) = 0;                                                          \
    }                                                                       \
  }

/**
   Make_terrains calls make_forest, make_dessert,etc  with random free
   locations until there  has been made enough.
   Comment: funtions as make_swamp, etc. has to have a non 0 probability
   to place one terrains in called position. Else make_terrains will get
   in a infinite loop!
 */
static void make_terrains()
{
  int total = 0;
  int forests_count = 0;
  int jungles_count = 0;
  int deserts_count = 0;
  int alt_deserts_count = 0;
  int plains_count = 0;
  int swamps_count = 0;

  whole_map_iterate(&(wld.map), ptile)
  {
    if (not_placed(ptile)) {
      total++;
    }
  }
  whole_map_iterate_end;

  forests_count = total * forest_pct / (100 - mountain_pct);
  jungles_count = total * jungle_pct / (100 - mountain_pct);

  deserts_count = total * desert_pct / (100 - mountain_pct);
  swamps_count = total * swamp_pct / (100 - mountain_pct);

  // grassland, tundra,arctic and plains is counted in plains_count
  plains_count =
      total - forests_count - deserts_count - swamps_count - jungles_count;

  // the placement loop
  do {
    PLACE_ONE_TYPE(forests_count, plains_count,
                   pick_terrain(MG_FOLIAGE, MG_TEMPERATE, MG_TROPICAL),
                   WC_ALL, TT_NFROZEN, MC_NONE, 60);
    PLACE_ONE_TYPE(jungles_count, forests_count,
                   pick_terrain(MG_FOLIAGE, MG_TROPICAL, MG_COLD), WC_ALL,
                   TT_TROPICAL, MC_NONE, 50);
    PLACE_ONE_TYPE(swamps_count, forests_count,
                   pick_terrain(MG_WET, MG_UNUSED, MG_FOLIAGE), WC_NDRY,
                   TT_HOT, MC_LOW, 50);
    PLACE_ONE_TYPE(deserts_count, alt_deserts_count,
                   pick_terrain(MG_DRY, MG_TROPICAL, MG_COLD), WC_DRY,
                   TT_NFROZEN, MC_NLOW, 80);
    PLACE_ONE_TYPE(alt_deserts_count, plains_count,
                   pick_terrain(MG_DRY, MG_TROPICAL, MG_WET), WC_ALL,
                   TT_NFROZEN, MC_NLOW, 40);

    // make the plains and tundras
    if (plains_count > 0) {
      struct tile *ptile;

      // Don't use any restriction here !
      if ((ptile = rand_map_pos_characteristic(WC_ALL, TT_ALL, MC_NONE))) {
        make_plain(ptile, &plains_count);
      } else {
        /* If rand_map_pos_temperature returns FALSE we may as well stop
         * looking for plains. */
        plains_count = 0;
      }
    }
  } while (forests_count > 0 || jungles_count > 0 || deserts_count > 0
           || alt_deserts_count > 0 || plains_count > 0 || swamps_count > 0);
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_blocked(struct river_map *privermap,
                              struct tile *ptile, struct extra_type *priver)
{
  if (privermap->blocked.at(tile_index(ptile))) {
    return 1;
  }

  // any un-blocked?
  cardinal_adjc_iterate(&(wld.map), ptile, ptile1)
  {
    if (!privermap->blocked.at(tile_index(ptile1))) {
      return 0;
    }
  }
  cardinal_adjc_iterate_end;

  return 1; // none non-blocked |- all blocked
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_rivergrid(struct river_map *privermap,
                                struct tile *ptile,
                                struct extra_type *priver)
{
  return (count_river_type_tile_card(ptile, priver, false) > 1) ? 1 : 0;
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_highlands(struct river_map *privermap,
                                struct tile *ptile,
                                struct extra_type *priver)
{
  return tile_terrain(ptile)->property[MG_MOUNTAINOUS];
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_adjacent_ocean(struct river_map *privermap,
                                     struct tile *ptile,
                                     struct extra_type *priver)
{
  return 100 - count_terrain_class_near_tile(ptile, true, true, TC_OCEAN);
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_adjacent_river(struct river_map *privermap,
                                     struct tile *ptile,
                                     struct extra_type *priver)
{
  return 100 - count_river_type_tile_card(ptile, priver, true);
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_adjacent_highlands(struct river_map *privermap,
                                         struct tile *ptile,
                                         struct extra_type *priver)
{
  int sum = 0;

  adjc_iterate(&(wld.map), ptile, ptile2)
  {
    sum += tile_terrain(ptile2)->property[MG_MOUNTAINOUS];
  }
  adjc_iterate_end;

  return sum;
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_swamp(struct river_map *privermap, struct tile *ptile,
                            struct extra_type *priver)
{
  return FC_INFINITY - tile_terrain(ptile)->property[MG_WET];
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_adjacent_swamp(struct river_map *privermap,
                                     struct tile *ptile,
                                     struct extra_type *priver)
{
  int sum = 0;

  adjc_iterate(&(wld.map), ptile, ptile2)
  {
    sum += tile_terrain(ptile2)->property[MG_WET];
  }
  adjc_iterate_end;

  return FC_INFINITY - sum;
}

/**
   Help function used in make_river(). See the help there.
 */
static int river_test_height_map(struct river_map *privermap,
                                 struct tile *ptile,
                                 struct extra_type *priver)
{
  return hmap(ptile);
}

/**
   Called from make_river. Marks all directions as blocked.  -Erik Sigra
 */
static void river_blockmark(struct river_map *privermap, struct tile *ptile)
{
  log_debug("Blockmarking (%d, %d) and adjacent tiles.", TILE_XY(ptile));

  privermap->blocked.setBit(tile_index(ptile));

  cardinal_adjc_iterate(&(wld.map), ptile, ptile1)
  {
    privermap->blocked.setBit(tile_index(ptile1));
  }
  cardinal_adjc_iterate_end;
}

struct test_func {
  int (*func)(struct river_map *privermap, struct tile *ptile,
              struct extra_type *priver);
  bool fatal;
};

#define NUM_TEST_FUNCTIONS 9
static struct test_func test_funcs[NUM_TEST_FUNCTIONS] = {
    {river_test_blocked, true},
    {river_test_rivergrid, true},
    {river_test_highlands, false},
    {river_test_adjacent_ocean, false},
    {river_test_adjacent_river, false},
    {river_test_adjacent_highlands, false},
    {river_test_swamp, false},
    {river_test_adjacent_swamp, false},
    {river_test_height_map, false}};

/**
  Makes a river starting at (x, y). Returns 1 if it succeeds.
  Return 0 if it fails. The river is stored in river_map.

  How to make a river path look natural
  =====================================
  Rivers always flow down. Thus rivers are best implemented on maps
  where every tile has an explicit height value. However, Freeciv21 has
  a flat map. But there are certain things that help the user imagine
  differences in height between tiles. The selection of direction for
  rivers should confirm and even amplify the user's image of the map's
  topology.

  To decide which direction the river takes, the possible directions
  are tested in a series of test until there is only 1 direction
  left. Some tests are fatal. This means that they can sort away all
  remaining directions. If they do so, the river is aborted. Here
  follows a description of the test series.

  * Falling into itself: fatal
      (river_test_blocked)
      This is tested by looking up in the river_map array if a tile or
      every tile surrounding the tile is marked as blocked. A tile is
      marked as blocked if it belongs to the current river or has been
      evaluated in a previous iteration in the creation of the current
      river.

      Possible values:
      0: Is not falling into itself.
      1: Is falling into itself.

  * Forming a 4-river-grid: optionally fatal
      (river_test_rivergrid)
      A minimal 4-river-grid is formed when an intersection in the map
      grid is surrounded by 4 river tiles. There can be larger river
      grids consisting of several overlapping minimal 4-river-grids.

      Possible values:
      0: Is not forming a 4-river-grid.
      1: Is forming a 4-river-grid.

  * Highlands:
      (river_test_highlands)
      Rivers must not flow up in mountains or hills if there are
      alternatives.

      Possible values:
      0: Is not hills and not mountains.
      1: Is hills.
      2: Is mountains.

  * Adjacent ocean:
      (river_test_adjacent_ocean)
      Rivers must flow down to coastal areas when possible:

      Possible values: 0-100

  * Adjacent river:
      (river_test_adjacent_river)
      Rivers must flow down to areas near other rivers when possible:

      Possible values: 0-100

  * Adjacent highlands:
      (river_test_adjacent_highlands)
      Rivers must not flow towards highlands if there are alternatives.

  * Swamps:
      (river_test_swamp)
      Rivers must flow down in swamps when possible.

      Possible values:
      0: Is swamps.
      1: Is not swamps.

  * Adjacent swamps:
      (river_test_adjacent_swamp)
      Rivers must flow towards swamps when possible.

  * height_map:
      (river_test_height_map)
      Rivers must flow in the direction which takes it to the tile with
      the lowest value on the height_map.

      Possible values:
      n: height_map[...]

  If these rules haven't decided the direction, the random number
  generator gets the desicion.                              -Erik Sigra
 */
static bool make_river(struct river_map *privermap, struct tile *ptile,
                       struct extra_type *priver)
{
  /* Comparison value for each tile surrounding the current tile.  It is
   * the suitability to continue a river to the tile in that direction;
   * lower is better.  However rivers may only run in cardinal directions;
   * the other directions are ignored entirely. */
  int rd_comparison_val[8];

  bool rd_direction_is_valid[8];
  int num_valid_directions, func_num, direction;

  while (true) {
    // Mark the current tile as river.
    privermap->ok.setBit(tile_index(ptile));
    log_debug("The tile at (%d, %d) has been marked as river in river_map.",
              TILE_XY(ptile));

    // Test if the river is done.
    // We arbitrarily make rivers end at the poles.
    if (count_river_near_tile(ptile, priver) > 0
        || count_terrain_class_near_tile(ptile, true, true, TC_OCEAN) > 0
        || (tile_terrain(ptile)->property[MG_FROZEN] > 0
            && map_colatitude(ptile) < 0.8 * COLD_LEVEL)) {
      log_debug("The river ended at (%d, %d).", TILE_XY(ptile));
      return true;
    }

    // Else choose a direction to continue the river.
    log_debug("The river did not end at (%d, %d). Evaluating directions...",
              TILE_XY(ptile));

    // Mark all available cardinal directions as available.
    memset(rd_direction_is_valid, 0, sizeof(rd_direction_is_valid));
    cardinal_adjc_dir_base_iterate(&(wld.map), ptile, dir)
    {
      rd_direction_is_valid[dir] = true;
    }
    cardinal_adjc_dir_base_iterate_end;

    // Test series that selects a direction for the river.
    for (func_num = 0; func_num < NUM_TEST_FUNCTIONS; func_num++) {
      int best_val = -1;

      // first get the tile values for the function
      cardinal_adjc_dir_iterate(&(wld.map), ptile, ptile1, dir)
      {
        if (rd_direction_is_valid[dir]) {
          rd_comparison_val[dir] =
              (test_funcs[func_num].func)(privermap, ptile1, priver);
          fc_assert_action(rd_comparison_val[dir] >= 0, continue);
          if (best_val == -1) {
            best_val = rd_comparison_val[dir];
          } else {
            best_val = MIN(rd_comparison_val[dir], best_val);
          }
        }
      }
      cardinal_adjc_dir_iterate_end;
      fc_assert_action(best_val != -1, continue);

      // should we abort?
      if (best_val > 0 && test_funcs[func_num].fatal) {
        return false;
      }

      // mark the less attractive directions as invalid
      cardinal_adjc_dir_base_iterate(&(wld.map), ptile, dir)
      {
        if (rd_direction_is_valid[dir]) {
          if (rd_comparison_val[dir] != best_val) {
            rd_direction_is_valid[dir] = false;
          }
        }
      }
      cardinal_adjc_dir_base_iterate_end;
    }

    /* Directions evaluated with all functions. Now choose the best
       direction before going to the next iteration of the while loop */
    num_valid_directions = 0;
    cardinal_adjc_dir_base_iterate(&(wld.map), ptile, dir)
    {
      if (rd_direction_is_valid[dir]) {
        num_valid_directions++;
      }
    }
    cardinal_adjc_dir_base_iterate_end;

    if (num_valid_directions == 0) {
      return false; // river aborted
    }

    // One or more valid directions: choose randomly.
    log_debug("mapgen.c: Had to let the random number"
              " generator select a direction for a river.");
    direction = fc_rand(num_valid_directions);
    log_debug("mapgen.c: direction: %d", direction);

    // Find the direction that the random number generator selected.
    cardinal_adjc_dir_iterate(&(wld.map), ptile, tile1, dir)
    {
      if (rd_direction_is_valid[dir]) {
        if (direction > 0) {
          direction--;
        } else {
          river_blockmark(privermap, ptile);
          ptile = tile1;
          break;
        }
      }
    }
    cardinal_adjc_dir_iterate_end;
    fc_assert_ret_val(direction == 0, false);
  } // end while; (Make a river.)
}

/**
   Calls make_river until there are enough river tiles on the map. It stops
   when it has tried to create RIVERS_MAXTRIES rivers.           -Erik Sigra
 */
static void make_rivers()
{
  struct tile *ptile;
  struct terrain *pterrain;
  struct river_map rivermap;
  struct extra_type *road_river = nullptr;

  /* Formula to make the river density similar om different sized maps.
     Avoids too few rivers on large maps and too many rivers on small maps.
   */
  int desirable_riverlength =
      river_pct *
      // The size of the map (poles counted in river_pct).
      map_num_tiles() *
      // Rivers need to be on land only.
      wld.map.server.landpercent /
      /* Adjustment value. Tested by me. Gives no rivers with 'set
         rivers 0', gives a reasonable amount of rivers with default
         settings and as many rivers as possible with 'set rivers 100'. */
      5325;

  // The number of river tiles that have been set.
  int current_riverlength = 0;

  /* Counts the number of iterations (should increase with 1 during
     every iteration of the main loop in this function).
     Is needed to stop a potentially infinite loop. */
  int iteration_counter = 0;

  if (river_type_count <= 0) {
    // No river type available
    return;
  }

  create_placed_map(); // needed bu rand_map_characteristic
  set_all_ocean_tiles_placed();

  rivermap.blocked.resize(MAP_INDEX_SIZE);
  rivermap.ok.resize(MAP_INDEX_SIZE);

  // The main loop in this function.
  while (current_riverlength < desirable_riverlength
         && iteration_counter < RIVERS_MAXTRIES) {
    if (!(ptile =
              rand_map_pos_characteristic(WC_ALL, TT_NFROZEN, MC_NLOW))) {
      break; // mo more spring places
    }
    pterrain = tile_terrain(ptile);

    /* Check if it is suitable to start a river on the current tile.
     */
    if (
        // Don't start a river on ocean.
        !is_ocean(pterrain)

        // Don't start a river on river.
        && !tile_has_river(ptile)

        /* Don't start a river on a tile is surrounded by > 1 river +
           ocean tile. */
        && (count_river_near_tile(ptile, nullptr)
                + count_terrain_class_near_tile(ptile, true, false, TC_OCEAN)
            <= 1)

        /* Don't start a river on a tile that is surrounded by hills or
           mountains unless it is hard to find somewhere else to start
           it. */
        && (count_terrain_property_near_tile(ptile, true, true,
                                             MG_MOUNTAINOUS)
                < 90
            || iteration_counter >= RIVERS_MAXTRIES / 10 * 5)

        /* Don't start a river on hills unless it is hard to find
           somewhere else to start it. */
        && (pterrain->property[MG_MOUNTAINOUS] == 0
            || iteration_counter >= RIVERS_MAXTRIES / 10 * 6)

        /* Don't start a river on arctic unless it is hard to find
           somewhere else to start it. */
        && (pterrain->property[MG_FROZEN] == 0
            || iteration_counter >= RIVERS_MAXTRIES / 10 * 8)

        /* Don't start a river on desert unless it is hard to find
           somewhere else to start it. */
        && (pterrain->property[MG_DRY] == 0
            || iteration_counter >= RIVERS_MAXTRIES / 10 * 9)) {
      // Reset river map before making a new river.
      rivermap.blocked.fill(false);
      rivermap.ok.fill(false);

      road_river = river_types[fc_rand(river_type_count)];

      extra_type_by_cause_iterate(EC_ROAD, oriver)
      {
        if (oriver != road_river) {
          whole_map_iterate(&(wld.map), rtile)
          {
            if (tile_has_extra(rtile, oriver)) {
              rivermap.blocked.setBit(tile_index(rtile));
            }
          }
          whole_map_iterate_end;
        }
      }
      extra_type_by_cause_iterate_end;

      log_debug("Found a suitable starting tile for a river at (%d, %d)."
                " Starting to make it.",
                TILE_XY(ptile));

      // Try to make a river. If it is OK, apply it to the map.
      if (make_river(&rivermap, ptile, road_river)) {
        whole_map_iterate(&(wld.map), ptile1)
        {
          if (rivermap.ok.at(tile_index(ptile1))) {
            struct terrain *river_terrain = tile_terrain(ptile1);

            if (!terrain_has_flag(river_terrain, TER_CAN_HAVE_RIVER)) {
              // We have to change the terrain to put a river here.
              river_terrain = pick_terrain_by_flag(TER_CAN_HAVE_RIVER);
              if (river_terrain != nullptr) {
                tile_set_terrain(ptile1, river_terrain);
              }
            }

            tile_add_extra(ptile1, road_river);
            current_riverlength++;
            map_set_placed(ptile1);
            log_debug("Applied a river to (%d, %d).", TILE_XY(ptile1));
          }
        }
        whole_map_iterate_end;
      } else {
        log_debug("mapgen.c: A river failed. It might have gotten stuck "
                  "in a helix.");
      }
    } // end if;
    iteration_counter++;
    log_debug("current_riverlength: %d; desirable_riverlength: %d; "
              "iteration_counter: %d",
              current_riverlength, desirable_riverlength, iteration_counter);
  } // end while;

  destroy_placed_map();
}

/**
   make land simply does it all based on a generated heightmap
   1) with map.server.landpercent it generates a ocean/unknown map
   2) it then calls the above functions to generate the different terrains
 */
static void make_land()
{
  struct terrain *land_fill = nullptr;

  if (HAS_POLES) {
    normalize_hmap_poles();
  }

  /* Pick a non-ocean terrain just once and fill all land tiles with "
   * that terrain. We must set some terrain (and not T_UNKNOWN) so that "
   * continent number assignment works. */
  terrain_type_iterate(pterrain)
  {
    if (!is_ocean(pterrain)
        && !terrain_has_flag(pterrain, TER_NOT_GENERATED)) {
      land_fill = pterrain;
      break;
    }
  }
  terrain_type_iterate_end;

  fc_assert_exit_msg(nullptr != land_fill,
                     "No land terrain type could be found for the purpose "
                     "of temporarily filling in land tiles during map "
                     "generation. This could be an error in Freeciv21, or a "
                     "mistake in the terrain.ruleset file. Please make sure "
                     "there is at least one land terrain type in the "
                     "ruleset, or use a different map generator. If this "
                     "error persists, please report it at: %s",
                     BUG_URL);

  hmap_shore_level =
      (hmap_max_level * (100 - wld.map.server.landpercent)) / 100;
  ini_hmap_low_level();
  whole_map_iterate(&(wld.map), ptile)
  {
    tile_set_terrain(ptile, T_UNKNOWN); // set as oceans count is used
    if (hmap(ptile) < hmap_shore_level) {
      int depth = (hmap_shore_level - hmap(ptile)) * 100 / hmap_shore_level;
      int ocean = 0;
      int land = 0;

      // This is to make shallow connection between continents less likely
      adjc_iterate(&(wld.map), ptile, other)
      {
        if (hmap(other) < hmap_shore_level) {
          ocean++;
        } else {
          land++;
          break;
        }
      }
      adjc_iterate_end;

      depth += 30 * (ocean - land) / MAX(1, (ocean + land));

      depth = MIN(depth, TERRAIN_OCEAN_DEPTH_MAXIMUM);

      /* Generate sea ice here, if ruleset supports it. Dummy temperature
       * map is sufficient for this. If ruleset doesn't support it,
       * use unfrozen ocean; make_polar_land() will later fill in with
       * land-based ice. Ice has a ragged margin. */
      {
        bool frozen =
            HAS_POLES
            && (tmap_is(ptile, TT_FROZEN)
                || (tmap_is(ptile, TT_COLD) && fc_rand(10) > 7
                    && is_temperature_type_near(ptile, TT_FROZEN)));
        struct terrain *pterrain = pick_ocean(depth, frozen);

        if (frozen && !pterrain) {
          pterrain = pick_ocean(depth, false);
          fc_assert(pterrain);
        }
        tile_set_terrain(ptile, pterrain);
      }
    } else {
      // See note above for 'land_fill'.
      tile_set_terrain(ptile, land_fill);
    }
  }
  whole_map_iterate_end;

  if (HAS_POLES) {
    renormalize_hmap_poles();
  }

  // destroy old dummy temperature map ...
  destroy_tmap();
  // ... and create a real temperature map (needs hmap and oceans)
  create_tmap(true);

  if (HAS_POLES) {     /* this is a hack to terrains set with not frizzed
                          oceans*/
    make_polar_land(); /* make extra land at poles*/
  }

  create_placed_map(); // here it means land terrains to be placed
  set_all_ocean_tiles_placed();
  if (MAPGEN_FRACTURE == wld.map.server.generator) {
    make_fracture_relief();
  } else {
    make_relief(); // base relief on map
  }
  make_terrains(); // place all exept mountains and hill
  destroy_placed_map();

  make_rivers(); // use a new placed_map. destroy older before call
}

/**
   Returns if this is a 1x1 island
 */
static bool is_tiny_island(struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  if (is_ocean(pterrain) || pterrain->property[MG_FROZEN] > 0) {
    /* The arctic check is needed for iso-maps: the poles may not have
     * any cardinally adjacent land tiles, but that's okay. */
    return false;
  }

  adjc_iterate(&(wld.map), ptile, tile1)
  {
    /* This was originally a cardinal_adjc_iterate, which seemed to cause
     * two or three problems. /MSS */
    if (!is_ocean_tile(tile1)) {
      return false;
    }
  }
  adjc_iterate_end;

  return true;
}

/**
   Removes all 1x1 islands (sets them to ocean).
   This happens before regenerate_lakes(), so don't need to worry about
   TER_FRESHWATER here.
 */
static void remove_tiny_islands()
{
  whole_map_iterate(&(wld.map), ptile)
  {
    if (is_tiny_island(ptile)) {
      struct terrain *shallow = most_shallow_ocean(
          terrain_has_flag(tile_terrain(ptile), TER_FROZEN));

      fc_assert_ret(nullptr != shallow);
      tile_set_terrain(ptile, shallow);
      extra_type_by_cause_iterate(EC_ROAD, priver)
      {
        if (tile_has_extra(ptile, priver)
            && road_has_flag(extra_road_get(priver), RF_RIVER)) {
          tile_extra_rm_apply(ptile, priver);
        }
      }
      extra_type_by_cause_iterate_end;
      tile_set_continent(ptile, 0);
    }
  }
  whole_map_iterate_end;
}

/**
   Debugging function to print information about the map that's been
   generated.
 */
static void print_mapgen_map()
{
  int terrain_counts[terrain_count()];
  int total = 0, ocean = 0;

  terrain_type_iterate(pterrain)
  {
    terrain_counts[terrain_index(pterrain)] = 0;
  }
  terrain_type_iterate_end;

  whole_map_iterate(&(wld.map), ptile)
  {
    struct terrain *pterrain = tile_terrain(ptile);

    terrain_counts[terrain_index(pterrain)]++;
    if (is_ocean(pterrain)) {
      ocean++;
    }
    total++;
  }
  whole_map_iterate_end;

  qDebug("map settings:");
  qDebug("  %-20s :      %5d%%", "mountain_pct", mountain_pct);
  qDebug("  %-20s :      %5d%%", "desert_pct", desert_pct);
  qDebug("  %-20s :      %5d%%", "forest_pct", forest_pct);
  qDebug("  %-20s :      %5d%%", "jungle_pct", jungle_pct);
  qDebug("  %-20s :      %5d%%", "swamp_pct", swamp_pct);

  qDebug("map statistics:");

  // avoid div by 0
  total = qMax(1, total);
  ocean = qMax(1, ocean);
  terrain_type_iterate(pterrain)
  {
    if (is_ocean(pterrain)) {
      qDebug("  %-20s : %6d %5.1f%% (ocean: %5.1f%%)",
             terrain_rule_name(pterrain),
             terrain_counts[terrain_index(pterrain)],
             static_cast<float>(terrain_counts[terrain_index(pterrain)])
                 * 100 / total,
             static_cast<float>(terrain_counts[terrain_index(pterrain)])
                 * 100 / ocean);
    } else {
      fc_assert_ret_msg(total - ocean, "Div by zero");
      qDebug("  %-20s : %6d %5.1f%% (land:  %5.1f%%)",
             terrain_rule_name(pterrain),
             terrain_counts[terrain_index(pterrain)],
             static_cast<float>(terrain_counts[terrain_index(pterrain)])
                 * 100 / total,
             static_cast<float>(terrain_counts[terrain_index(pterrain)])
                 * 100 / (total - ocean));
    }
  }
  terrain_type_iterate_end;
}

/**
   See stdinhand.c for information on map generation methods.

 FIXME: Some continent numbers are unused at the end of this function, fx
        removed completely by remove_tiny_islands.
        When this function is finished various data is written to "islands",
        indexed by continent numbers, so a simple renumbering would not
        work...

   If "autosize" is specified then mapgen will automatically size the map
   based on the map.server.size server parameter and the specified topology.
   If not map.xsize and map.ysize will be used.
 */
bool map_generate(bool autosize, struct unit_type *initial_unit)
{
  auto rstate = fc_rand_state();

  if (wld.map.server.seed_setting == 0) {
    // Create a random map seed.
    fc_rand_seed(fc_rand_state());
  } else {
    fc_srand(wld.map.server.seed_setting);
  }
  wld.map.server.seed = wld.map.server.seed_setting;

  /* don't generate tiles with mapgen == MAPGEN_SCENARIO as we've loaded *
     them from file.
     Also, don't delete (the handcrafted!) tiny islands in a scenario */
  if (wld.map.server.generator != MAPGEN_SCENARIO) {
    wld.map.server.have_huts = false;
    wld.map.server.have_resources = false;
    river_types_init();

    generator_init_topology(autosize);
    // Map can be already allocated, if we failed first map generation
    if (map_is_empty()) {
      main_map_allocate();
    }
    adjust_terrain_param();
    // if one mapgenerator fails, it will choose another mapgenerator
    // with a lower number to try again

    // create a temperature map
    create_tmap(false);

    if (MAPGEN_FAIR == wld.map.server.generator
        && !map_generate_fair_islands()) {
      wld.map.server.generator = MAPGEN_ISLAND;
    }

    if (MAPGEN_ISLAND == wld.map.server.generator
        && !map_generate_island()) {
      wld.map.server.generator = MAPGEN_FRACTAL;
    }

    if (MAPGEN_FRACTAL == wld.map.server.generator) {
      make_pseudofractal_hmap();
    }

    if (MAPGEN_RANDOM == wld.map.server.generator) {
      make_random_hmap();
    }

    if (MAPGEN_FRACTURE == wld.map.server.generator) {
      make_fracture_hmap();
    }

    // if hmap only generator make anything else
    if (MAPGEN_RANDOM == wld.map.server.generator
        || MAPGEN_FRACTAL == wld.map.server.generator
        || MAPGEN_FRACTURE == wld.map.server.generator) {
      make_land();
      delete[] height_map;
      height_map = nullptr;
    }
    if (!wld.map.server.tinyisles) {
      remove_tiny_islands();
    }

    smooth_water_depth();

    // Continent numbers must be assigned before regenerate_lakes()
    assign_continent_numbers();

    // Turn small oceans into lakes.
    regenerate_lakes();
  } else {
    assign_continent_numbers();
  }

  // create a temperature map if it was not done before
  if (!temperature_is_initialized()) {
    create_tmap(false);
  }

  // some scenarios already provide specials
  if (!wld.map.server.have_resources) {
    add_resources(wld.map.server.riches);
  }

  if (!wld.map.server.have_huts) {
    make_huts(wld.map.server.huts * map_num_tiles() / 1000);
  }

  // restore previous random state:
  fc_rand_set_state(rstate);

  /* We don't want random start positions in a scenario which already
   * provides them. */
  if (0 == map_startpos_count()) {
    enum map_startpos mode = MAPSTARTPOS_ALL;

    switch (wld.map.server.generator) {
    case MAPGEN_FAIR:
      fc_assert_msg(false, "Fair island generator failed to allocated "
                           "start positions!");
      break;
    case MAPGEN_SCENARIO:
    case MAPGEN_RANDOM:
    case MAPGEN_FRACTURE:
      mode = wld.map.server.startpos;
      break;
    case MAPGEN_FRACTAL:
      if (wld.map.server.startpos == MAPSTARTPOS_DEFAULT) {
        qDebug("Map generator chose startpos=ALL");
        mode = MAPSTARTPOS_ALL;
      } else {
        mode = wld.map.server.startpos;
      }
      break;
    case MAPGEN_ISLAND:
      switch (wld.map.server.startpos) {
      case MAPSTARTPOS_DEFAULT:
      case MAPSTARTPOS_VARIABLE:
        qDebug("Map generator chose startpos=SINGLE");
        // fallthrough
      case MAPSTARTPOS_SINGLE:
        mode = MAPSTARTPOS_SINGLE;
        break;
      default:
        qDebug("Map generator chose startpos=2or3");
        // fallthrough
      case MAPSTARTPOS_2or3:
        mode = MAPSTARTPOS_2or3;
        break;
      }
      break;
    }

    for (;;) {
      bool success;

      success = create_start_positions(mode, initial_unit);
      if (success) {
        wld.map.server.startpos = mode;
        break;
      }

      switch (mode) {
      case MAPSTARTPOS_SINGLE:
        qDebug("Falling back to startpos=2or3");
        mode = MAPSTARTPOS_2or3;
        continue;
      case MAPSTARTPOS_2or3:
        qDebug("Falling back to startpos=ALL");
        mode = MAPSTARTPOS_ALL;
        break;
      case MAPSTARTPOS_ALL:
        qDebug("Falling back to startpos=VARIABLE");
        mode = MAPSTARTPOS_VARIABLE;
        break;
      default:
        qCritical(_("The server couldn't allocate starting positions."));
        destroy_tmap();
        return false;
      }
    }
  }

  // destroy temperature map
  destroy_tmap();

  print_mapgen_map();

  return true;
}

/**
   Convert parameters from the server into terrains percents parameters for
   the generators
 */
static void adjust_terrain_param()
{
  int polar =
      2 * ICE_BASE_LEVEL * wld.map.server.landpercent / MAX_COLATITUDE;
  float mount_factor = (100.0 - polar - 30 * 0.8) / 10000;
  float factor = (100.0 - polar - wld.map.server.steepness * 0.8) / 10000;

  mountain_pct = mount_factor * wld.map.server.steepness * 90;

  // 27 % if wetness == 50 &
  forest_pct = factor * (wld.map.server.wetness * 40 + 700);
  jungle_pct =
      forest_pct * (MAX_COLATITUDE - TROPICAL_LEVEL) / (MAX_COLATITUDE * 2);
  forest_pct -= jungle_pct;

  // 3 - 11 %
  river_pct = (100 - polar) * (3 + wld.map.server.wetness / 12) / 100;

  // 7 %  if wetness == 50 && temperature == 50
  swamp_pct = factor
              * MAX(0, (wld.map.server.wetness * 12 - 150
                        + wld.map.server.temperature * 10));
  desert_pct = factor
               * MAX(0, (wld.map.server.temperature * 15 - 250
                         + (100 - wld.map.server.wetness) * 10));
}

/**
   Return TRUE if a safe tile is in a radius of 1.  This function is used to
   test where to place specials on the sea.
 */
static bool near_safe_tiles(struct tile *ptile)
{
  square_iterate(&(wld.map), ptile, 1, tile1)
  {
    if (!terrain_has_flag(tile_terrain(tile1), TER_UNSAFE_COAST)) {
      return true;
    }
  }
  square_iterate_end;

  return false;
}

/**
   This function spreads out huts on the map, a position can be used for a
   hut if there isn't another hut close and if it's not on the ocean.
 */
static void make_huts(int number)
{
  int count = 0;
  struct tile *ptile;

  create_placed_map(); // here it means placed huts

  while (number > 0 && count++ < map_num_tiles() * 2) {
    // Add a hut.  But not on a polar area, or too close to another hut.
    if ((ptile = rand_map_pos_characteristic(WC_ALL, TT_NFROZEN, MC_NONE))) {
      struct extra_type *phut = rand_extra_for_tile(ptile, EC_HUT, true);

      number--;
      if (phut != nullptr) {
        tile_add_extra(ptile, phut);
      }
      set_placed_near_pos(ptile, 3);
    }
  }
  destroy_placed_map();
}

/**
   Return TRUE iff there's a resource within one tile of the given map
   position.
 */
static bool is_resource_close(const struct tile *ptile)
{
  square_iterate(&(wld.map), ptile, 1, tile1)
  {
    if (nullptr != tile_resource(tile1)) {
      return true;
    }
  }
  square_iterate_end;

  return false;
}

/**
   Add specials to the map with given probability (out of 1000).
 */
static void add_resources(int prob)
{
  whole_map_iterate(&(wld.map), ptile)
  {
    const struct terrain *pterrain = tile_terrain(ptile);

    if (is_resource_close(ptile) || fc_rand(1000) >= prob) {
      continue;
    }
    if (!is_ocean(pterrain) || near_safe_tiles(ptile)
        || wld.map.server.ocean_resources) {
      int i = 0;
      struct extra_type **r;

      for (r = pterrain->resources; *r; r++) {
        /* This is a standard way to get a random element from the
         * pterrain->resources list, without computing its length in
         * advance. Note that if *(pterrain->resources) == nullptr, then
         * this loop is a no-op. */
        if ((*r)->generated) {
          if (0 == fc_rand(++i)) {
            tile_set_resource(ptile, *r);
          }
        }
      }
    }
  }
  whole_map_iterate_end;

  wld.map.server.have_resources = true;
}

/*************************************************************************/

/**
   Initialize river types array
 */
static void river_types_init()
{
  river_type_count = 0;

  extra_type_by_cause_iterate(EC_ROAD, priver)
  {
    if (road_has_flag(extra_road_get(priver), RF_RIVER)
        && priver->generated) {
      river_types[river_type_count++] = priver;
    }
  }
  extra_type_by_cause_iterate_end;
}
