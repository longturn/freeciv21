// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "islands.h"

// common
#include "game.h"
#include "map.h"
#include "terrain.h"

// server/generator
#include "height_map.h"
#include "mapgen.h"
#include "mapgen_topology.h"
#include "mapgen_utils.h"
#include "temperature_map.h"

// utility
#include "rand.h"

// common variables for generator 2, 3 and 4
struct gen234_state {
  int isleindex, n, e, s, w;
  long int totalmass;
};

// define one terrain selection
struct terrain_select {
  int weight;
  enum mapgen_terrain_property target;
  enum mapgen_terrain_property prefer;
  enum mapgen_terrain_property avoid;
  int temp_condition;
  int wet_condition;
};

#define SPECLIST_TAG terrain_select
#include "speclist.h"
// list iterator for terrain_select
#define terrain_select_list_iterate(tersel_list, ptersel)                   \
  TYPED_LIST_ITERATE(struct terrain_select, tersel_list, ptersel)
#define terrain_select_list_iterate_end LIST_ITERATE_END

static struct terrain_select *tersel_new(int weight,
                                         enum mapgen_terrain_property target,
                                         enum mapgen_terrain_property prefer,
                                         enum mapgen_terrain_property avoid,
                                         int temp_condition,
                                         int wet_condition);
static void tersel_free(struct terrain_select *ptersel);

// terrain selection lists for make_island()
static struct {
  bool init;
  struct terrain_select_list *forest;
  struct terrain_select_list *desert;
  struct terrain_select_list *mountain;
  struct terrain_select_list *swamp;
} island_terrain = {.init = false};

static void fill_island(int coast, long int *bucket,
                        const struct terrain_select_list *tersel_list,
                        const struct gen234_state *const pstate);
static bool make_island(int islemass, int starters,
                        struct gen234_state *pstate,
                        int min_specific_island_size);
static bool create_island(int islemass, struct gen234_state *pstate);

static void fill_island_rivers(int coast, long int *bucket,
                               const struct gen234_state *const pstate);

static long int checkmass;

/**
   Initialize terrain selection lists for make_island().
 */
static void island_terrain_init()
{
  struct terrain_select *ptersel;

  // forest
  island_terrain.forest = terrain_select_list_new_full(tersel_free);
  ptersel =
      tersel_new(1, MG_FOLIAGE, MG_TROPICAL, MG_DRY, TT_TROPICAL, WC_ALL);
  terrain_select_list_append(island_terrain.forest, ptersel);
  ptersel =
      tersel_new(3, MG_FOLIAGE, MG_TEMPERATE, MG_UNUSED, TT_ALL, WC_ALL);
  terrain_select_list_append(island_terrain.forest, ptersel);
  ptersel =
      tersel_new(1, MG_FOLIAGE, MG_WET, MG_FROZEN, TT_TROPICAL, WC_NDRY);
  terrain_select_list_append(island_terrain.forest, ptersel);
  ptersel =
      tersel_new(1, MG_FOLIAGE, MG_COLD, MG_UNUSED, TT_NFROZEN, WC_ALL);
  terrain_select_list_append(island_terrain.forest, ptersel);

  // desert
  island_terrain.desert = terrain_select_list_new_full(tersel_free);
  ptersel = tersel_new(3, MG_DRY, MG_TROPICAL, MG_GREEN, TT_HOT, WC_DRY);
  terrain_select_list_append(island_terrain.desert, ptersel);
  ptersel =
      tersel_new(2, MG_DRY, MG_TEMPERATE, MG_GREEN, TT_NFROZEN, WC_DRY);
  terrain_select_list_append(island_terrain.desert, ptersel);
  ptersel = tersel_new(1, MG_COLD, MG_DRY, MG_TROPICAL, TT_NHOT, WC_DRY);
  terrain_select_list_append(island_terrain.desert, ptersel);
  ptersel = tersel_new(1, MG_FROZEN, MG_DRY, MG_UNUSED, TT_FROZEN, WC_DRY);
  terrain_select_list_append(island_terrain.desert, ptersel);

  // mountain
  island_terrain.mountain = terrain_select_list_new_full(tersel_free);
  ptersel =
      tersel_new(2, MG_MOUNTAINOUS, MG_GREEN, MG_UNUSED, TT_ALL, WC_ALL);
  terrain_select_list_append(island_terrain.mountain, ptersel);
  ptersel =
      tersel_new(1, MG_MOUNTAINOUS, MG_UNUSED, MG_GREEN, TT_ALL, WC_ALL);
  terrain_select_list_append(island_terrain.mountain, ptersel);

  // swamp
  island_terrain.swamp = terrain_select_list_new_full(tersel_free);
  ptersel =
      tersel_new(1, MG_WET, MG_TROPICAL, MG_FOLIAGE, TT_TROPICAL, WC_NDRY);
  terrain_select_list_append(island_terrain.swamp, ptersel);
  ptersel = tersel_new(2, MG_WET, MG_TEMPERATE, MG_FOLIAGE, TT_HOT, WC_NDRY);
  terrain_select_list_append(island_terrain.swamp, ptersel);
  ptersel = tersel_new(1, MG_WET, MG_COLD, MG_FOLIAGE, TT_NHOT, WC_NDRY);
  terrain_select_list_append(island_terrain.swamp, ptersel);

  island_terrain.init = true;
}

/**
   Free memory allocated for terrain selection lists.
 */
static void island_terrain_free()
{
  if (!island_terrain.init) {
    return;
  }

  terrain_select_list_destroy(island_terrain.forest);
  terrain_select_list_destroy(island_terrain.desert);
  terrain_select_list_destroy(island_terrain.mountain);
  terrain_select_list_destroy(island_terrain.swamp);

  island_terrain.init = false;
}

/**
   make an island, fill every tile type except plains
   note: you have to create big islands first.
   Return TRUE if successful.
   min_specific_island_size is a percent value.
 */
static bool make_island(int islemass, int starters,
                        struct gen234_state *pstate,
                        int min_specific_island_size)
{
  // int may be only 2 byte !
  static long int tilefactor, balance, lastplaced;
  static long int riverbuck, mountbuck, desertbuck, forestbuck, swampbuck;
  int i;

  /* The terrain selection lists have to be initialised.
   * (see island_terrain_init()) */
  fc_assert_ret_val(island_terrain.init, false);

  if (islemass == 0) {
    /* this only runs to initialise static things, not to actually
     * create an island. */
    balance = 0;
    // 0 = none, poles, then isles
    pstate->isleindex = wld.map.num_continents + 1;

    checkmass = pstate->totalmass;

    // caveat: this should really be sent to all players
    if (pstate->totalmass > 3000) {
      qInfo(_("High landmass - this may take a few seconds."));
    }

    i = river_pct + mountain_pct + desert_pct + forest_pct + swamp_pct;
    i = (i <= 90) ? 100 : i * 11 / 10;
    tilefactor = pstate->totalmass / i;
    riverbuck = -static_cast<long int>(fc_rand(pstate->totalmass));
    mountbuck = -static_cast<long int>(fc_rand(pstate->totalmass));
    desertbuck = -static_cast<long int>(fc_rand(pstate->totalmass));
    forestbuck = -static_cast<long int>(fc_rand(pstate->totalmass));
    swampbuck = -static_cast<long int>(fc_rand(pstate->totalmass));
    lastplaced = pstate->totalmass;
  } else {
    // makes the islands this big
    islemass = islemass - balance;

    if (islemass > lastplaced + 1 + lastplaced / 50) {
      // don't create big isles we can't place
      islemass = lastplaced + 1 + lastplaced / 50;
    }

    // isle creation does not perform well for nonsquare islands
    if (islemass > (wld.map.ysize - 6) * (wld.map.ysize - 6)) {
      islemass = (wld.map.ysize - 6) * (wld.map.ysize - 6);
    }

    if (islemass > (wld.map.xsize - 2) * (wld.map.xsize - 2)) {
      islemass = (wld.map.xsize - 2) * (wld.map.xsize - 2);
    }

    i = islemass;
    if (i <= 0) {
      return false;
    }
    fc_assert_ret_val(starters >= 0, false);
    qDebug("island %i", pstate->isleindex);

    /* keep trying to place an island, and decrease the size of
     * the island we're trying to create until we succeed.
     * If we get too small, return an error. */
    while (!create_island(i, pstate)) {
      if (i < islemass * min_specific_island_size / 100) {
        return false;
      }
      i--;
    }
    i++;
    lastplaced = i;
    if (i * 10 > islemass) {
      balance = i - islemass;
    } else {
      balance = 0;
    }

    qDebug("ini=%d, plc=%d, bal=%ld, tot=%ld", islemass, i, balance,
           checkmass);

    i *= tilefactor;

    riverbuck += river_pct * i;
    fill_island_rivers(1, &riverbuck, pstate);

    // forest
    forestbuck += forest_pct * i;
    fill_island(60, &forestbuck, island_terrain.forest, pstate);

    // desert
    desertbuck += desert_pct * i;
    fill_island(40, &desertbuck, island_terrain.desert, pstate);

    // mountain
    mountbuck += mountain_pct * i;
    fill_island(20, &mountbuck, island_terrain.mountain, pstate);

    // swamp
    swampbuck += swamp_pct * i;
    fill_island(80, &swampbuck, island_terrain.swamp, pstate);

    pstate->isleindex++;
    wld.map.num_continents++;
  }
  return true;
}

/**
   fill ocean and make polar
   A temperature map is created in map_fractal_generate().
 */
static void initworld(struct gen234_state *pstate)
{
  struct terrain *deepest_ocean =
      pick_ocean(TERRAIN_OCEAN_DEPTH_MAXIMUM, false);

  fc_assert(nullptr != deepest_ocean);
  height_map = new int[MAP_INDEX_SIZE];

  create_placed_map(); // land tiles which aren't placed yet

  whole_map_iterate(&(wld.map), ptile)
  {
    tile_set_terrain(ptile, deepest_ocean);
    tile_set_continent(ptile, 0);
    map_set_placed(ptile); // not a land tile
    BV_CLR_ALL(ptile->extras);
    tile_set_owner(ptile, nullptr, nullptr);
    ptile->extras_owner = nullptr;
  }
  whole_map_iterate_end;

  if (HAS_POLES) {
    make_polar();
  }

  /* Set poles numbers.  After the map is generated continents will
   * be renumbered. */
  make_island(0, 0, pstate, 0);
}

/* This variable is the Default Minimum Specific Island Size,
 * ie the smallest size we'll typically permit our island, as a % of
 * the size we wanted. So if we ask for an island of size x, the island
 * creation will return if it would create an island smaller than
 *  x * DMSIS / 100 */
#define DMSIS 10

/**
   island base map generators
 */
static void map_island_generate_variable()
{
  long int totalweight;
  struct gen234_state state;
  struct gen234_state *pstate = &state;
  int i;
  bool done = false;
  int spares = 1;
  // constant that makes up that an island actually needs additional space

  /* put 70% of land in big continents,
   *     20% in medium, and
   *     10% in small. */
  int bigfrac = 70, midfrac = 20, smallfrac = 10;

  if (wld.map.server.landpercent > 85) {
    qDebug("ISLAND generator: falling back to RANDOM generator");
    wld.map.server.generator = MAPGEN_RANDOM;
    return;
  }

  pstate->totalmass =
      ((wld.map.ysize - 6 - spares) * wld.map.server.landpercent
       * (wld.map.xsize - spares))
      / 100;
  totalweight = 100 * player_count();

  fc_assert_action(!placed_map_is_initialized(),
                   wld.map.server.generator = MAPGEN_RANDOM;
                   return );

  while (!done && bigfrac > midfrac) {
    done = true;

    if (placed_map_is_initialized()) {
      destroy_placed_map();
    }

    initworld(pstate);

    // Create one big island for each player.
    for (i = player_count(); i > 0; i--) {
      if (!make_island(bigfrac * pstate->totalmass / totalweight, 1, pstate,
                       95)) {
        /* we couldn't make an island at least 95% as big as we wanted,
         * and since we're trying hard to be fair, we need to start again,
         * with all big islands reduced slightly in size.
         * Take the size reduction from the big islands and add it to the
         * small islands to keep overall landmass unchanged.
         * Note that the big islands can get very small if necessary, and
         * the smaller islands will not exist if we can't place them
         * easily. */
        qDebug("Island too small, trying again with all smaller "
               "islands.");
        midfrac += bigfrac * 0.01;
        smallfrac += bigfrac * 0.04;
        bigfrac *= 0.95;
        done = false;
        break;
      }
    }
  }

  if (bigfrac <= midfrac) {
    // We could never make adequately big islands.
    qDebug("ISLAND generator: falling back to RANDOM generator");
    wld.map.server.generator = MAPGEN_RANDOM;

    // init world created this map, destroy it before abort
    destroy_placed_map();
    delete[] height_map;
    height_map = nullptr;
    return;
  }

  /* Now place smaller islands, but don't worry if they're small,
   * or even non-existent. One medium and one small per player. */
  for (i = player_count(); i > 0; i--) {
    make_island(midfrac * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }
  for (i = player_count(); i > 0; i--) {
    make_island(smallfrac * pstate->totalmass / totalweight, 0, pstate,
                DMSIS);
  }

  make_plains();
  destroy_placed_map();
  delete[] height_map;
  height_map = nullptr;

  if (checkmass > wld.map.xsize + wld.map.ysize + totalweight) {
    qDebug("%ld mass left unplaced", checkmass);
  }
}

/**
   On popular demand, this tries to mimick the generator 3 as best as
   possible.
 */
static void map_island_generate_single()
{
  int spares = 1;
  int j = 0;
  long int islandmass, landmass, size;
  long int maxmassdiv6 = 20;
  int bigislands;
  struct gen234_state state;
  struct gen234_state *pstate = &state;

  if (wld.map.server.landpercent > 80) {
    qDebug("ISLAND generator: falling back to FRACTAL generator due "
           "to landpercent > 80.");
    wld.map.server.generator = MAPGEN_FRACTAL;
    return;
  }

  if (wld.map.xsize < 40 || wld.map.ysize < 40) {
    qDebug("ISLAND generator: falling back to FRACTAL generator due "
           "to unsupported map size.");
    wld.map.server.generator = MAPGEN_FRACTAL;
    return;
  }

  pstate->totalmass =
      (((wld.map.ysize - 6 - spares) * wld.map.server.landpercent
        * (wld.map.xsize - spares))
       / 100);

  bigislands = player_count();

  landmass =
      (wld.map.xsize * (wld.map.ysize - 6) * wld.map.server.landpercent)
      / 100;
  // subtracting the arctics
  if (landmass > 3 * wld.map.ysize + player_count() * 3) {
    landmass -= 3 * wld.map.ysize;
  }

  islandmass = (landmass) / (3 * bigislands);
  if (islandmass < 4 * maxmassdiv6) {
    islandmass = (landmass) / (2 * bigislands);
  }
  if (islandmass < 3 * maxmassdiv6 && player_count() * 2 < landmass) {
    islandmass = (landmass) / (bigislands);
  }

  if (islandmass < 2) {
    islandmass = 2;
  }
  if (islandmass > maxmassdiv6 * 6) {
    islandmass = maxmassdiv6 * 6; // !PS: let's try this
  }

  initworld(pstate);

  while (pstate->isleindex - 2 <= bigislands && checkmass > islandmass
         && ++j < 500) {
    make_island(islandmass, 1, pstate, DMSIS);
  }

  if (j == 500) {
    qInfo(_("Generator 3 didn't place all big islands."));
  }

  islandmass = (islandmass * 11) / 8;
  /*!PS: I'd like to mult by 3/2, but starters might make trouble then */
  if (islandmass < 2) {
    islandmass = 2;
  }

  while (checkmass > islandmass && ++j < 1500) {
    if (j < 1000) {
      size = fc_rand((islandmass + 1) / 2 + 1) + islandmass / 2;
    } else {
      size = fc_rand((islandmass + 1) / 2 + 1);
    }
    if (size < 2) {
      size = 2;
    }

    make_island(size, (pstate->isleindex - 2 <= player_count()) ? 1 : 0,
                pstate, DMSIS);
  }

  make_plains();
  destroy_placed_map();
  delete[] height_map;
  height_map = nullptr;

  if (j == 1500) {
    qInfo(_("Generator 3 left %li landmass unplaced."), checkmass);
  } else if (checkmass > wld.map.xsize + wld.map.ysize) {
    qDebug("%ld mass left unplaced", checkmass);
  }
}

/**
   Generator for placing a couple of players to each island.
 */
static void map_island_generate_2or3()
{
  int bigweight = 70;
  int spares = 1;
  int i;
  long int totalweight;
  struct gen234_state state;
  struct gen234_state *pstate = &state;

  // no islands with mass >> sqr(min(xsize,ysize))

  if (player_count() < 2 || wld.map.server.landpercent > 80) {
    qDebug("ISLAND generator: falling back to startpos=SINGLE");
    wld.map.server.startpos = MAPSTARTPOS_SINGLE;
    return;
  }

  if (wld.map.server.landpercent > 60) {
    bigweight = 30;
  } else if (wld.map.server.landpercent > 40) {
    bigweight = 50;
  } else {
    bigweight = 70;
  }

  spares = (wld.map.server.landpercent - 5) / 30;

  pstate->totalmass =
      (((wld.map.ysize - 6 - spares) * wld.map.server.landpercent
        * (wld.map.xsize - spares))
       / 100);

  /*!PS: The weights NEED to sum up to totalweight (dammit) */
  totalweight = (30 + bigweight) * player_count();

  initworld(pstate);

  i = player_count() / 2;
  if ((player_count() % 2) == 1) {
    make_island(bigweight * 3 * pstate->totalmass / totalweight, 3, pstate,
                DMSIS);
  } else {
    i++;
  }
  while ((--i) > 0) {
    make_island(bigweight * 2 * pstate->totalmass / totalweight, 2, pstate,
                DMSIS);
  }
  for (i = player_count(); i > 0; i--) {
    make_island(20 * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }
  for (i = player_count(); i > 0; i--) {
    make_island(10 * pstate->totalmass / totalweight, 0, pstate, DMSIS);
  }
  make_plains();
  destroy_placed_map();
  delete[] height_map;
  height_map = nullptr;

  if (checkmass > wld.map.xsize + wld.map.ysize + totalweight) {
    qDebug("%ld mass left unplaced", checkmass);
  }
}

/**
 * Generate a map with the ISLAND family of generators.
 */
void map_island_generate()
{
  // initialise terrain selection lists used by make_island()
  island_terrain_init();

  // 2 or 3 players per isle?
  if (MAPSTARTPOS_2or3 == wld.map.server.startpos
      || MAPSTARTPOS_ALL == wld.map.server.startpos) {
    map_island_generate_2or3();
  }
  if (MAPSTARTPOS_DEFAULT == wld.map.server.startpos
      || MAPSTARTPOS_SINGLE == wld.map.server.startpos) {
    // Single player per isle.
    map_island_generate_single();
  }
  if (MAPSTARTPOS_VARIABLE == wld.map.server.startpos) {
    // "Variable" single player.
    map_island_generate_variable();
  }

  // free terrain selection lists used by make_island()
  island_terrain_free();
}

#undef DMSIS

/**
   Returns a random position in the rectangle denoted by the given state.
 */
static struct tile *
get_random_map_position_from_state(const struct gen234_state *const pstate)
{
  int xrnd, yrnd;

  fc_assert_ret_val((pstate->e - pstate->w) > 0, nullptr);
  fc_assert_ret_val((pstate->e - pstate->w) < wld.map.xsize, nullptr);
  fc_assert_ret_val((pstate->s - pstate->n) > 0, nullptr);
  fc_assert_ret_val((pstate->s - pstate->n) < wld.map.ysize, nullptr);

  xrnd = pstate->w + fc_rand(pstate->e - pstate->w);
  yrnd = pstate->n + fc_rand(pstate->s - pstate->n);

  return native_pos_to_tile(&(wld.map), xrnd, yrnd);
}

/**
   Allocate and initialize new terrain_select structure.
 */
static struct terrain_select *tersel_new(int weight,
                                         enum mapgen_terrain_property target,
                                         enum mapgen_terrain_property prefer,
                                         enum mapgen_terrain_property avoid,
                                         int temp_condition,
                                         int wet_condition)
{
  struct terrain_select *ptersel = new terrain_select;

  ptersel->weight = weight;
  ptersel->target = target;
  ptersel->prefer = prefer;
  ptersel->avoid = avoid;
  ptersel->temp_condition = temp_condition;
  ptersel->wet_condition = wet_condition;

  return ptersel;
}

/**
   Free resources allocated for terrain_select structure.
 */
static void tersel_free(struct terrain_select *ptersel)
{
  delete ptersel;
  ptersel = nullptr;
}

/**
   Fill an island with different types of terrains; rivers have extra code.
 */
static void fill_island(int coast, long int *bucket,
                        const struct terrain_select_list *tersel_list,
                        const struct gen234_state *const pstate)
{
  int i, k, capac, total_weight = 0;
  int ntersel = terrain_select_list_size(tersel_list);
  long int failsafe;

  if (*bucket <= 0) {
    return;
  }

  // must have at least one terrain selection given in tersel_list
  fc_assert_ret(ntersel != 0);

  capac = pstate->totalmass;
  i = *bucket / capac;
  i++;
  *bucket -= i * capac;

  k = i;
  failsafe = i * (pstate->s - pstate->n) * (pstate->e - pstate->w);
  if (failsafe < 0) {
    failsafe = -failsafe;
  }

  terrain_select_list_iterate(tersel_list, ptersel)
  {
    total_weight += ptersel->weight;
  }
  terrain_select_list_iterate_end;

  if (total_weight <= 0) {
    return;
  }

  while (i > 0 && (failsafe--) > 0) {
    struct tile *ptile = get_random_map_position_from_state(pstate);

    if (tile_continent(ptile) != pstate->isleindex || !not_placed(ptile)) {
      continue;
    }

    struct terrain_select *ptersel =
        terrain_select_list_get(tersel_list, fc_rand(ntersel));

    if (fc_rand(total_weight) > ptersel->weight) {
      continue;
    }

    if (!tmap_is(ptile, ptersel->temp_condition)
        || !test_wetness(ptile,
                         static_cast<wetness_c>(ptersel->wet_condition))) {
      continue;
    }

    struct terrain *pterrain =
        pick_terrain(ptersel->target, ptersel->prefer, ptersel->avoid);

    /* the first condition helps make terrain more contiguous,
       the second lets it avoid the coast: */
    if ((i * 3 > k * 2 || fc_rand(100) < 50
         || is_terrain_near_tile(ptile, pterrain, false))
        && (!is_terrain_class_card_near(ptile, TC_OCEAN)
            || fc_rand(100) < coast)) {
      tile_set_terrain(ptile, pterrain);
      map_set_placed(ptile);

      log_debug("[fill_island] placed terrain '%s' at (%2d,%2d)",
                terrain_rule_name(pterrain), TILE_XY(ptile));
    }

    if (!not_placed(ptile)) {
      i--;
    }
  }
}

/**
   Returns TRUE if ptile is suitable for a river mouth.
 */
static bool island_river_mouth_suitability(const struct tile *ptile,
                                           const struct extra_type *priver)
{
  int num_card_ocean, pct_adj_ocean, num_adj_river;

  num_card_ocean =
      count_terrain_class_near_tile(ptile, C_CARDINAL, C_NUMBER, TC_OCEAN);
  pct_adj_ocean =
      count_terrain_class_near_tile(ptile, C_ADJACENT, C_PERCENT, TC_OCEAN);
  num_adj_river = count_river_type_tile_card(ptile, priver, false);

  return (num_card_ocean == 1 && pct_adj_ocean <= 35 && num_adj_river == 0);
}

/**
   Returns TRUE if there is a river in a cardinal direction near the tile
   and the tile is suitable for extending it.
 */
static bool island_river_suitability(const struct tile *ptile,
                                     const struct extra_type *priver)
{
  int pct_adj_ocean, num_card_ocean, pct_adj_river, num_card_river;

  num_card_river = count_river_type_tile_card(ptile, priver, false);
  num_card_ocean =
      count_terrain_class_near_tile(ptile, C_CARDINAL, C_NUMBER, TC_OCEAN);
  pct_adj_ocean =
      count_terrain_class_near_tile(ptile, C_ADJACENT, C_PERCENT, TC_OCEAN);
  pct_adj_river = count_river_type_near_tile(ptile, priver, true);

  return (num_card_river == 1 && num_card_ocean == 0 && pct_adj_ocean < 20
          && pct_adj_river < 35
          /* The following expression helps with straightness,
           * ocean avoidance, and reduces forking. */
          && (pct_adj_river + pct_adj_ocean * 2) < fc_rand(25) + 25);
}

/**
   Fill an island with rivers.
 */
static void fill_island_rivers(int coast, long int *bucket,
                               const struct gen234_state *const pstate)
{
  long int failsafe, capac, i, k;
  struct tile *ptile;

  if (*bucket <= 0) {
    return;
  }
  if (river_type_count <= 0) {
    return;
  }

  capac = pstate->totalmass;
  i = *bucket / capac;
  i++;
  *bucket -= i * capac;

  // generate 75% more rivers than generator 1
  i = (i * 175) / 100;

  k = i;
  failsafe = i * (pstate->s - pstate->n) * (pstate->e - pstate->w) * 5;
  if (failsafe < 0) {
    failsafe = -failsafe;
  }

  while (i > 0 && failsafe-- > 0) {
    struct extra_type *priver;

    ptile = get_random_map_position_from_state(pstate);
    if (tile_continent(ptile) != pstate->isleindex
        || tile_has_river(ptile)) {
      continue;
    }

    priver = river_types[fc_rand(river_type_count)];

    if (test_wetness(ptile, WC_DRY) && fc_rand(100) < 50) {
      // rivers don't like dry locations
      continue;
    }

    if ((island_river_mouth_suitability(ptile, priver)
         && (fc_rand(100) < coast || i == k))
        || island_river_suitability(ptile, priver)) {
      tile_add_extra(ptile, priver);
      i--;
    }
  }
}

/**
   Return TRUE if the ocean position is near land.  This is used in the
   creation of islands, so it differs logically from near_safe_tiles().
 */
static bool is_near_land(struct tile *ptile)
{
  // Note this function may sometimes be called on land tiles.
  adjc_iterate(&(wld.map), ptile, tile1)
  {
    if (!is_ocean(tile_terrain(tile1))) {
      return true;
    }
  }
  adjc_iterate_end;

  return false;
}

/**
   Finds a place and drop the island created when called with islemass != 0
 */
static bool place_island(struct gen234_state *pstate)
{
  int i = 0, xcur, ycur, nat_x, nat_y;
  struct tile *ptile;

  ptile = rand_map_pos(&(wld.map));
  index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));

  // this helps a lot for maps with high landmass
  for (ycur = pstate->n, xcur = pstate->w;
       ycur < pstate->s && xcur < pstate->e; ycur++, xcur++) {
    struct tile *tile0 = native_pos_to_tile(&(wld.map), xcur, ycur);
    struct tile *tile1 = native_pos_to_tile(
        &(wld.map), xcur + nat_x - pstate->w, ycur + nat_y - pstate->n);

    if (!tile0 || !tile1) {
      return false;
    }
    if (hmap(tile0) != 0 && is_near_land(tile1)) {
      return false;
    }
  }

  for (ycur = pstate->n; ycur < pstate->s; ycur++) {
    for (xcur = pstate->w; xcur < pstate->e; xcur++) {
      struct tile *tile0 = native_pos_to_tile(&(wld.map), xcur, ycur);
      struct tile *tile1 = native_pos_to_tile(
          &(wld.map), xcur + nat_x - pstate->w, ycur + nat_y - pstate->n);

      if (!tile0 || !tile1) {
        return false;
      }
      if (hmap(tile0) != 0 && is_near_land(tile1)) {
        return false;
      }
    }
  }

  for (ycur = pstate->n; ycur < pstate->s; ycur++) {
    for (xcur = pstate->w; xcur < pstate->e; xcur++) {
      if (hmap(native_pos_to_tile(&(wld.map), xcur, ycur)) != 0) {
        struct tile *tile1 = native_pos_to_tile(
            &(wld.map), xcur + nat_x - pstate->w, ycur + nat_y - pstate->n);

        checkmass--;
        if (checkmass <= 0) {
          qCritical("mapgen.c: mass doesn't sum up.");
          return i != 0;
        }

        tile_set_terrain(tile1, T_UNKNOWN);
        map_unset_placed(tile1);

        tile_set_continent(tile1, pstate->isleindex);
        i++;
      }
    }
  }

  pstate->s += nat_y - pstate->n;
  pstate->e += nat_x - pstate->w;
  pstate->n = nat_y;
  pstate->w = nat_x;

  return i != 0;
}

/**
   Returns the number of cardinally adjacent tiles have a non-zero elevation.
 */
static int count_card_adjc_elevated_tiles(struct tile *ptile)
{
  int count = 0;

  cardinal_adjc_iterate(&(wld.map), ptile, tile1)
  {
    if (hmap(tile1) != 0) {
      count++;
    }
  }
  cardinal_adjc_iterate_end;

  return count;
}

/**
   finds a place and drop the island created when called with islemass != 0
 */
static bool create_island(int islemass, struct gen234_state *pstate)
{
  int i, nat_x, nat_y;
  long int tries = islemass * (2 + islemass / 20) + 99;
  bool j;
  struct tile *ptile =
      native_pos_to_tile(&(wld.map), wld.map.xsize / 2, wld.map.ysize / 2);

  memset(height_map, '\0', MAP_INDEX_SIZE * sizeof(*height_map));
  hmap(native_pos_to_tile(&(wld.map), wld.map.xsize / 2,
                          wld.map.ysize / 2)) = 1;

  index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
  pstate->n = nat_y - 1;
  pstate->w = nat_x - 1;
  pstate->s = nat_y + 2;
  pstate->e = nat_x + 2;
  i = islemass - 1;
  while (i > 0 && tries-- > 0) {
    ptile = get_random_map_position_from_state(pstate);
    index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));

    if ((!near_singularity(ptile) || fc_rand(50) < 25) && hmap(ptile) == 0
        && count_card_adjc_elevated_tiles(ptile) > 0) {
      hmap(ptile) = 1;
      i--;
      if (nat_y >= pstate->s - 1 && pstate->s < wld.map.ysize - 2) {
        pstate->s++;
      }
      if (nat_x >= pstate->e - 1 && pstate->e < wld.map.xsize - 2) {
        pstate->e++;
      }
      if (nat_y <= pstate->n && pstate->n > 2) {
        pstate->n--;
      }
      if (nat_x <= pstate->w && pstate->w > 2) {
        pstate->w--;
      }
    }
    if (i < islemass / 10) {
      int xcur, ycur;

      for (ycur = pstate->n; ycur < pstate->s; ycur++) {
        for (xcur = pstate->w; xcur < pstate->e; xcur++) {
          ptile = native_pos_to_tile(&(wld.map), xcur, ycur);
          if (hmap(ptile) == 0 && i > 0
              && count_card_adjc_elevated_tiles(ptile) == 4) {
            hmap(ptile) = 1;
            i--;
          }
        }
      }
    }
  }
  if (tries <= 0) {
    qCritical("create_island ended early with %d/%d.", islemass - i,
              islemass);
  }

  tries = map_num_tiles() / 4; // on a 40x60 map, there are 2400 places
  while (!(j = place_island(pstate)) && (--tries) > 0) {
    // nothing
  }
  return j;
}
