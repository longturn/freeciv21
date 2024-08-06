// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "fair_islands.h"

// common
#include "fc_types.h"
#include "nation.h"
#include "map.h"

// server/generator
#include "mapgen.h"
#include "mapgen_utils.h"

// utility
#include "rand.h"

/**
  Fair island generator types.
 */
enum fair_tile_flag {
  FTF_NONE = 0,
  FTF_ASSIGNED = 1 << 0,
  FTF_OCEAN = 1 << 1,
  FTF_STARTPOS = 1 << 2,
  FTF_NO_RESOURCE = 1 << 3,
  FTF_HAS_HUT = 1 << 4,
  FTF_NO_HUT = 1 << 5
};

struct fair_tile {
  enum fair_tile_flag flags;
  struct terrain *pterrain;
  struct extra_type *presource;
  bv_extras extras;
  int startpos_team_id;
};

typedef void (*fair_geometry_func)(int *x, int *y);
struct fair_geometry_data {
  fair_geometry_func transform[4];
  int transform_num;
};

/**
   Free a map.
 */
static inline void fair_map_destroy(struct fair_tile *pmap)
{
  delete[] pmap;
}

/**
   Get the coordinates of tile 'ptile'.
 */
static inline void fair_map_tile_pos(struct fair_tile *pmap,
                                     struct fair_tile *ptile, int *x, int *y)
{
  index_to_map_pos(x, y, ptile - pmap);
}

/**
   Get the tile at the position ('x', 'y').
 */
static inline struct fair_tile *fair_map_pos_tile(struct fair_tile *pmap,
                                                  int x, int y)
{
  int nat_x, nat_y;

  MAP_TO_NATIVE_POS(&nat_x, &nat_y, x, y);

  // Wrap in X and Y directions, as needed.
  if (nat_x < 0 || nat_x >= wld.map.xsize) {
    if (current_topo_has_flag(TF_WRAPX)) {
      nat_x = FC_WRAP(nat_x, wld.map.xsize);
    } else {
      return nullptr;
    }
  }
  if (nat_y < 0 || nat_y >= wld.map.ysize) {
    if (current_topo_has_flag(TF_WRAPY)) {
      nat_y = FC_WRAP(nat_y, wld.map.ysize);
    } else {
      return nullptr;
    }
  }

  return pmap + native_pos_to_index(nat_x, nat_y);
}

/**
   Get the next tile in direction 'dir'.
 */
static inline struct fair_tile *fair_map_tile_step(struct fair_tile *pmap,
                                                   struct fair_tile *ptile,
                                                   enum direction8 dir)
{
  int x, y, dx, dy;

  fair_map_tile_pos(pmap, ptile, &x, &y);
  DIRSTEP(dx, dy, dir);
  return fair_map_pos_tile(pmap, x + dx, y + dy);
}

/**
   Returns whether 'ptile' is at least at 'dist' tiles (in real distance)
   to the border. Note is also take in account map wrapping.
 */
static inline bool fair_map_tile_border(struct fair_tile *pmap,
                                        struct fair_tile *ptile, int dist)
{
  int nat_x, nat_y;

  index_to_native_pos(&nat_x, &nat_y, ptile - pmap);

  if (!current_topo_has_flag(TF_WRAPX)
      && (nat_x < dist || nat_x >= wld.map.xsize - dist)) {
    return true;
  }

  if (MAP_IS_ISOMETRIC) {
    dist *= 2;
  }

  if (!current_topo_has_flag(TF_WRAPY)
      && (nat_y < dist || nat_y >= wld.map.ysize - dist)) {
    return true;
  }

  return false;
}

/**
   Compare two iter_index values for doing closest team placement.
 */
static int fair_team_placement_closest(const void *a, const void *b)
{
  const struct iter_index *index1 = static_cast<const iter_index *>(a),
                          *index2 = static_cast<const iter_index *>(b);

  return index1->dist - index2->dist;
}

/**
   Compare two iter_index values for doing horizontal team placement.
 */
static int fair_team_placement_horizontal(const void *a, const void *b)
{
  const struct iter_index *index1 = static_cast<const iter_index *>(a),
                          *index2 = static_cast<const iter_index *>(b);
  // Map vector to natural vector (Y axis).
  int diff = (MAP_IS_ISOMETRIC ? abs(index1->dx + index1->dy)
                                     - abs(index2->dx + index2->dy)
                               : abs(index1->dy) - abs(index2->dy));

  return (diff != 0 ? diff : index1->dist - index2->dist);
}

/**
   Compare two iter_index values for doing vertical team placement.
 */
static int fair_team_placement_vertical(const void *a, const void *b)
{
  const struct iter_index *index1 = static_cast<const iter_index *>(a),
                          *index2 = static_cast<const iter_index *>(b);
  // Map vector to natural vector (X axis).
  int diff = (MAP_IS_ISOMETRIC ? abs(index1->dx - index1->dy)
                                     - abs(index2->dx - index2->dy)
                               : abs(index1->dx) - abs(index2->dx));

  return (diff != 0 ? diff : index1->dist - index2->dist);
}

/**
   Symetry matrix.
 */
static void fair_do_symetry1(int *x, int *y) { *x = -*x; }

/**
   Symetry matrix.
 */
static void fair_do_symetry2(int *x, int *y) { *y = -*y; }

/**
   Symetry matrix for hexagonal topologies.
 */
static void fair_do_hex_symetry1(int *x, int *y) { *x = -(*x + *y); }

/**
   Symetry matrix for hexagonal topologies.
 */
static void fair_do_hex_symetry2(int *x, int *y)
{
  *x = -*x;
  *y = -*y;
}

/**
   Symetry matrix for hexgonal-isometric topology.
 */
static void fair_do_iso_hex_symetry1(int *x, int *y) { *y = *x - *y; }

#define fair_do_iso_hex_symetry2 fair_do_rotation

/**
   Rotation matrix, also symetry matrix for hexagonal-isometric topology.
 */
static void fair_do_rotation(int *x, int *y)
{
  int z = *x;

  *x = *y;
  *y = z;
}

/**
   Rotation matrix for hexgonal topology.
 */
static void fair_do_hex_rotation(int *x, int *y)
{
  int z = *x + *y;

  *x = -*y;
  *y = z;
}

/**
   Rotation matrix for hexgonal-isometric topology.
 */
static void fair_do_iso_hex_rotation(int *x, int *y)
{
  int z = *x - *y;

  *y = *x;
  *x = z;
}

/**
   Perform transformations defined into 'data' to position ('x', 'y').
 */
static void fair_do_geometry(const struct fair_geometry_data *data, int *x,
                             int *y)
{
  int i;

  for (i = 0; i < data->transform_num; i++) {
    data->transform[i](x, y);
  }
}

/**
   Push random transformations to 'data'.
 */
static void fair_geometry_rand(struct fair_geometry_data *data)
{
  int i = 0;

  if (!current_topo_has_flag(TF_HEX)) {
    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_symetry1;
    }
    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_symetry2;
    }
    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_rotation;
    }
  } else if (!current_topo_has_flag(TF_ISO)) {
    int steps;

    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_hex_symetry1;
    }
    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_hex_symetry2;
    }
    // Rotations have 2 steps on hexgonal topologies.
    for (steps = fc_rand(99) % 3; steps > 0; steps--) {
      data->transform[i++] = fair_do_hex_rotation;
    }
  } else {
    int steps;

    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_iso_hex_symetry1;
    }
    if (fc_rand(100) < 50) {
      data->transform[i++] = fair_do_iso_hex_symetry2;
    }
    // Rotations have 2 steps on hexgonal topologies.
    for (steps = fc_rand(99) % 3; steps > 0; steps--) {
      data->transform[i++] = fair_do_iso_hex_rotation;
    }
  }
  fc_assert(i <= ARRAY_SIZE(data->transform));
  data->transform_num = i;
}

/**
   Copy 'psource' on 'ptarget' at position ('tx', 'ty'), performing
   transformations defined into 'data'. Assign start positions for team
   'startpos_team_id'. Return TRUE if we have copied the map, FALSE if the
   copy was not possible.
 */
static bool fair_map_copy(struct fair_tile *ptarget, int tx, int ty,
                          struct fair_tile *psource,
                          const struct fair_geometry_data *data,
                          int startpos_team_id)
{
  int sdx = wld.map.xsize / 2, sdy = wld.map.ysize / 2;
  struct fair_tile *smax_tile = psource + MAP_INDEX_SIZE;
  struct fair_tile *pstile, *pttile;
  int x, y;

  // Check.
  for (pstile = psource; pstile < smax_tile; pstile++) {
    if (pstile->flags == FTF_NONE) {
      continue;
    }

    // Do translation and eventually other transformations.
    fair_map_tile_pos(psource, pstile, &x, &y);
    x -= sdx;
    y -= sdy;
    fair_do_geometry(data, &x, &y);
    x += tx;
    y += ty;
    pttile = fair_map_pos_tile(ptarget, x, y);
    if (pttile == nullptr) {
      return false; // Limit of the map.
    }
    if (pttile->flags & FTF_ASSIGNED) {
      if (pstile->flags & FTF_ASSIGNED || !(pttile->flags & FTF_OCEAN)
          || !(pstile->flags & FTF_OCEAN)) {
        return false; // Already assigned for another usage.
      }
    } else if (pttile->flags & FTF_OCEAN && !(pstile->flags & FTF_OCEAN)) {
      return false; // We clearly want a sea tile here.
    }
    if ((pttile->flags & FTF_NO_RESOURCE && pstile->presource != nullptr)
        || (pstile->flags & FTF_NO_RESOURCE
            && pttile->presource != nullptr)) {
      return false; // Resource disallowed there.
    }
    if ((pttile->flags & FTF_NO_HUT && pstile->flags & FTF_HAS_HUT)
        || (pstile->flags & FTF_NO_HUT && pttile->flags & FTF_HAS_HUT)) {
      return false; // Resource disallowed there.
    }
  }

  // Copy.
  for (pstile = psource; pstile < smax_tile; pstile++) {
    if (pstile->flags == FTF_NONE) {
      continue;
    }

    // Do translation and eventually other transformations.
    fair_map_tile_pos(psource, pstile, &x, &y);
    x -= sdx;
    y -= sdy;
    fair_do_geometry(data, &x, &y);
    x += tx;
    y += ty;
    pttile = fair_map_pos_tile(ptarget, x, y);
    fc_assert_ret_val(pttile != nullptr, false);
    pttile->flags =
        static_cast<fair_tile_flag>(pttile->flags | pstile->flags);
    if (pstile->pterrain != nullptr) {
      pttile->pterrain = pstile->pterrain;
      pttile->presource = pstile->presource;
      pttile->extras = pstile->extras;
    }
    if (pstile->flags & FTF_STARTPOS) {
      pttile->startpos_team_id = startpos_team_id;
    }
  }
  return true; // Looks ok.
}

/**
   Attempts to copy 'psource' to 'ptarget' at a random position, with random
   geometric effects.
 */
static bool fair_map_place_island_rand(struct fair_tile *ptarget,
                                       struct fair_tile *psource)
{
  struct fair_geometry_data geometry;
  int i, r, x, y;

  fair_geometry_rand(&geometry);

  // Try random positions.
  for (i = 0; i < 10; i++) {
    r = fc_rand(MAP_INDEX_SIZE);
    index_to_map_pos(&x, &y, r);
    if (fair_map_copy(ptarget, x, y, psource, &geometry, -1)) {
      return true;
    }
  }

  // Try hard placement.
  r = fc_rand(MAP_INDEX_SIZE);
  for (i = (r + 1) % MAP_INDEX_SIZE; i != r; i = (i + 1) % MAP_INDEX_SIZE) {
    index_to_map_pos(&x, &y, i);
    if (fair_map_copy(ptarget, x, y, psource, &geometry, -1)) {
      return true;
    }
  }

  // Impossible placement.
  return false;
}

/**
   Attempts to copy 'psource' to 'ptarget' as close as possible of position
   'x', 'y' for players of the team 'team_id'.
 */
static bool fair_map_place_island_team(
    struct fair_tile *ptarget, int tx, int ty, struct fair_tile *psource,
    const struct iter_index *outwards_indices, int startpos_team_id)
{
  struct fair_geometry_data geometry;
  int i, x, y;

  fair_geometry_rand(&geometry);

  /* Iterate positions, beginning by a random index of the outwards
   * indices. */
  for (i = fc_rand(wld.map.num_iterate_outwards_indices / 200);
       i < wld.map.num_iterate_outwards_indices; i++) {
    x = tx + outwards_indices[i].dx;
    y = ty + outwards_indices[i].dy;
    if (normalize_map_pos(&(wld.map), &x, &y)
        && fair_map_copy(ptarget, x, y, psource, &geometry,
                         startpos_team_id)) {
      return true;
    }
  }

  // Impossible placement.
  return false;
}

/**
   Add resources on 'pmap'.
 */
static void fair_map_make_resources(struct fair_tile *pmap)
{
  struct fair_tile *pftile, *pftile2;
  struct extra_type **r;
  int i, j;

  for (i = 0; i < MAP_INDEX_SIZE; i++) {
    pftile = pmap + i;
    if (pftile->flags == FTF_NONE || pftile->flags & FTF_NO_RESOURCE
        || fc_rand(1000) >= wld.map.server.riches) {
      continue;
    }

    if (pftile->flags & FTF_OCEAN) {
      bool land_around = false;

      for (j = 0; j < wld.map.num_valid_dirs; j++) {
        pftile2 = fair_map_tile_step(pmap, pftile, wld.map.valid_dirs[j]);
        if (pftile2 != nullptr && pftile2->flags & FTF_ASSIGNED
            && !(pftile2->flags & FTF_OCEAN)) {
          land_around = true;
          break;
        }
      }
      if (!land_around) {
        continue;
      }
    }

    j = 0;
    for (r = pftile->pterrain->resources; *r != nullptr; r++) {
      if (fc_rand(++j) == 0) {
        pftile->presource = *r;
      }
    }
    /* Note that 'pftile->presource' might be nullptr if there is no suitable
     * resource for the terrain. */
    if (pftile->presource != nullptr) {
      pftile->flags =
          static_cast<fair_tile_flag>(pftile->flags | FTF_NO_RESOURCE);
      for (j = 0; j < wld.map.num_valid_dirs; j++) {
        pftile2 = fair_map_tile_step(pmap, pftile, wld.map.valid_dirs[j]);
        if (pftile2 != nullptr) {
          pftile2->flags =
              static_cast<fair_tile_flag>(pftile2->flags | FTF_NO_RESOURCE);
        }
      }

      BV_SET(pftile->extras, extra_index(pftile->presource));
    }
  }
}

/**
   Add huts on 'pmap'.
 */
static void fair_map_make_huts(struct fair_tile *pmap)
{
  struct fair_tile *pftile;
  struct tile *pvtile = tile_virtual_new(nullptr);
  struct extra_type *phut;
  int i, j, k;

  for (i = wld.map.server.huts * map_num_tiles() / 1000, j = 0;
       i > 0 && j < map_num_tiles() * 2; j++) {
    k = fc_rand(MAP_INDEX_SIZE);
    pftile = pmap + k;
    while (pftile->flags & FTF_NO_HUT) {
      pftile++;
      if (pftile - pmap == MAP_INDEX_SIZE) {
        pftile = pmap;
      }
      if (pftile - pmap == k) {
        break;
      }
    }
    if (pftile->flags & FTF_NO_HUT) {
      break; // Cannot make huts anymore.
    }

    i--;
    if (pftile->pterrain == nullptr) {
      continue; // Not an used tile.
    }

    pvtile->index = pftile - pmap;
    tile_set_terrain(pvtile, pftile->pterrain);
    tile_set_resource(pvtile, pftile->presource);
    pvtile->extras = pftile->extras;

    phut = rand_extra_for_tile(pvtile, EC_HUT, true);
    if (phut != nullptr) {
      tile_add_extra(pvtile, phut);
      pftile->extras = pvtile->extras;
      pftile->flags =
          static_cast<fair_tile_flag>(pftile->flags | FTF_HAS_HUT);
      square_iterate(&(wld.map), index_to_tile(&(wld.map), pftile - pmap), 3,
                     ptile)
      {
        pmap[tile_index(ptile)].flags = static_cast<fair_tile_flag>(
            pmap[tile_index(ptile)].flags | FTF_NO_HUT);
      }
      square_iterate_end;
    }
  }

  tile_virtual_destroy(pvtile);
}

/**
   Generate a map where an island would be placed in the center.
 */
static struct fair_tile *fair_map_island_new(int size, int startpos_num)
{
  enum {
    FT_GRASSLAND,
    FT_FOREST,
    FT_DESERT,
    FT_HILL,
    FT_MOUNTAIN,
    FT_SWAMP,
    FT_COUNT
  };
  struct {
    int count;
    enum mapgen_terrain_property target;
    enum mapgen_terrain_property prefer;
    enum mapgen_terrain_property avoid;
  } terrain[FT_COUNT] = {
      {0, MG_TEMPERATE, MG_GREEN, MG_MOUNTAINOUS},
      {0, MG_FOLIAGE, MG_TEMPERATE, MG_UNUSED},
      {0, MG_DRY, MG_TEMPERATE, MG_GREEN},
      {0, MG_MOUNTAINOUS, MG_GREEN, MG_UNUSED},
      {0, MG_MOUNTAINOUS, MG_UNUSED, MG_GREEN},
      {0, MG_WET, MG_TEMPERATE, MG_FOLIAGE},
  };

  struct fair_tile *pisland;
  struct fair_tile *land_tiles[1000];
  struct fair_tile *pftile, *pftile2, *pftile3;
  int fantasy;
  const int sea_around_island =
      (startpos_num > 0 ? CITY_MAP_DEFAULT_RADIUS : 1);
  const int sea_around_island_sq =
      (startpos_num > 0 ? CITY_MAP_DEFAULT_RADIUS_SQ : 2);
  int i, j, k;

  size = CLIP(startpos_num, size, ARRAY_SIZE(land_tiles));
  fantasy = (size * 2) / 5;
  pisland = new fair_tile[MAP_INDEX_SIZE]();
  pftile = fair_map_pos_tile(pisland, wld.map.xsize / 2, wld.map.ysize / 2);
  fc_assert(!fair_map_tile_border(pisland, pftile, sea_around_island));
  pftile->flags = static_cast<fair_tile_flag>(pftile->flags | FTF_ASSIGNED);
  land_tiles[0] = pftile;
  i = 1;

  log_debug("Generating an island with %d land tiles [fantasy=%d].", size,
            fantasy);

  // Make land.
  while (i < fantasy) {
    pftile = land_tiles[fc_rand(i)];

    for (j = 0; j < wld.map.num_valid_dirs; j++) {
      pftile2 = fair_map_tile_step(pisland, pftile, wld.map.valid_dirs[j]);
      fc_assert(pftile2 != nullptr);
      if (fair_map_tile_border(pisland, pftile2, sea_around_island)) {
        continue;
      }

      if (pftile2->flags == FTF_NONE) {
        pftile2->flags = FTF_ASSIGNED;
        land_tiles[i++] = pftile2;
        if (i == fantasy) {
          break;
        }
      }
    }
  }
  while (i < size) {
    pftile = land_tiles[i - fc_rand(fantasy) - 1];
    pftile2 = fair_map_tile_step(
        pisland, pftile,
        wld.map.cardinal_dirs[fc_rand(wld.map.num_cardinal_dirs)]);
    fc_assert(pftile2 != nullptr);
    if (fair_map_tile_border(pisland, pftile2, sea_around_island)) {
      continue;
    }

    if (pftile2->flags == FTF_NONE) {
      pftile2->flags = FTF_ASSIGNED;
      land_tiles[i++] = pftile2;
    }
  }
  fc_assert(i == size);

  // Add start positions.
  for (i = 0; i < startpos_num;) {
    pftile = land_tiles[fc_rand(size - fantasy)];
    fc_assert(pftile->flags & FTF_ASSIGNED);
    if (!(pftile->flags & FTF_STARTPOS)) {
      pftile->flags =
          static_cast<fair_tile_flag>(pftile->flags | FTF_STARTPOS);
      i++;
    }
  }

  // Make terrain.
  terrain[FT_GRASSLAND].count = size - startpos_num;
  terrain[FT_FOREST].count = ((forest_pct + jungle_pct) * size) / 100;
  terrain[FT_DESERT].count = (desert_pct * size) / 100;
  terrain[FT_HILL].count = (mountain_pct * size) / 150;
  terrain[FT_MOUNTAIN].count = (mountain_pct * size) / 300;
  terrain[FT_SWAMP].count = (swamp_pct * size) / 100;

  j = FT_GRASSLAND;
  for (i = 0; i < size; i++) {
    pftile = land_tiles[i];

    if (pftile->flags & FTF_STARTPOS) {
      pftile->pterrain = pick_terrain_by_flag(TER_STARTER);
    } else {
      if (terrain[j].count == 0 || fc_rand(100) < 70) {
        do {
          j = fc_rand(FT_COUNT);
        } while (terrain[j].count == 0);
      }
      pftile->pterrain = pick_terrain(terrain[j].target, terrain[j].prefer,
                                      terrain[j].avoid);
      terrain[j].count--;
    }
  }

  // Make sea around the island.
  for (i = 0; i < size; i++) {
    circle_iterate(&(wld.map),
                   index_to_tile(&(wld.map), land_tiles[i] - pisland),
                   sea_around_island_sq, ptile)
    {
      pftile = pisland + tile_index(ptile);

      if (pftile->flags == FTF_NONE) {
        pftile->flags = FTF_OCEAN;
        // No ice around island
        pftile->pterrain =
            pick_ocean(TERRAIN_OCEAN_DEPTH_MINIMUM
                           + fc_rand(TERRAIN_OCEAN_DEPTH_MAXIMUM / 2),
                       false);
        if (startpos_num > 0) {
          pftile->flags =
              static_cast<fair_tile_flag>(pftile->flags | FTF_ASSIGNED);
        }
      }
    }
    circle_iterate_end;
  }

  // Make rivers.
  if (river_type_count > 0) {
    struct extra_type *priver;
    struct fair_tile *pend;
    int n = ((river_pct * size * wld.map.num_cardinal_dirs
              * wld.map.num_cardinal_dirs)
             / 200);
    int length_max = 3, length, l;
    enum direction8 dir;
    int extra_idx;
    int dirs_num;
    bool cardinal_only;
    bool connectable_river_around, ocean_around;
    int river_around;
    bool finished;

    for (i = 0; i < n; i++) {
      pftile = land_tiles[fc_rand(size)];
      if (!terrain_has_flag(pftile->pterrain, TER_CAN_HAVE_RIVER)) {
        continue;
      }

      priver = river_types[fc_rand(river_type_count)];
      extra_idx = extra_index(priver);
      if (BV_ISSET(pftile->extras, extra_idx)) {
        continue;
      }
      cardinal_only = is_cardinal_only_road(priver);

      river_around = 0;
      connectable_river_around = false;
      ocean_around = false;
      for (j = 0; j < wld.map.num_valid_dirs; j++) {
        pftile2 = fair_map_tile_step(pisland, pftile, wld.map.valid_dirs[j]);
        if (pftile2 == nullptr) {
          continue;
        }

        if (pftile2->flags & FTF_OCEAN) {
          ocean_around = true;
          break;
        } else if (BV_ISSET(pftile2->extras, extra_idx)) {
          river_around++;
          if (!cardinal_only || is_cardinal_dir(wld.map.valid_dirs[j])) {
            connectable_river_around = true;
          }
        }
      }
      if (ocean_around || river_around > 1
          || (river_around == 1 && !connectable_river_around)) {
        continue;
      }

      if (connectable_river_around) {
        log_debug("Adding river at (%d, %d)",
                  index_to_map_pos_x(pftile - pisland),
                  index_to_map_pos_y(pftile - pisland));
        BV_SET(pftile->extras, extra_idx);
        continue;
      }

      // Check a river in one direction.
      pend = nullptr;
      length = -1;
      dir = direction8_invalid();
      dirs_num = 0;
      for (j = 0; j < wld.map.num_valid_dirs; j++) {
        if (cardinal_only && !is_cardinal_dir(wld.map.valid_dirs[j])) {
          continue;
        }

        finished = false;
        pftile2 = pftile;
        for (l = 2; l < length_max; l++) {
          pftile2 =
              fair_map_tile_step(pisland, pftile2, wld.map.valid_dirs[j]);
          if (pftile2 == nullptr
              || !terrain_has_flag(pftile2->pterrain, TER_CAN_HAVE_RIVER)) {
            break;
          }

          river_around = 0;
          connectable_river_around = false;
          ocean_around = false;
          for (k = 0; k < wld.map.num_valid_dirs; k++) {
            if (wld.map.valid_dirs[k]
                == DIR_REVERSE(wld.map.valid_dirs[j])) {
              continue;
            }

            pftile3 =
                fair_map_tile_step(pisland, pftile2, wld.map.valid_dirs[k]);
            if (pftile3 == nullptr) {
              continue;
            }

            if (pftile3->flags & FTF_OCEAN) {
              if (!cardinal_only || is_cardinal_dir(wld.map.valid_dirs[k])) {
                ocean_around = true;
              }
            } else if (BV_ISSET(pftile3->extras, extra_idx)) {
              river_around++;
              if (!cardinal_only || is_cardinal_dir(wld.map.valid_dirs[k])) {
                connectable_river_around = true;
              }
            }
          }
          if (river_around > 1 && !connectable_river_around) {
            break;
          } else if (ocean_around || connectable_river_around) {
            finished = true;
            break;
          }
        }
        if (finished && fc_rand(++dirs_num) == 0) {
          dir = wld.map.valid_dirs[j];
          pend = pftile2;
          length = l;
        }
      }
      if (pend == nullptr) {
        continue;
      }

      log_debug("Make river from (%d, %d) to (%d, %d) [dir=%s, length=%d]",
                index_to_map_pos_x(pftile - pisland),
                index_to_map_pos_y(pftile - pisland),
                index_to_map_pos_x(pend - pisland),
                index_to_map_pos_y(pend - pisland), direction8_name(dir),
                length);
      for (;;) {
        BV_SET(pftile->extras, extra_idx);
        length--;
        if (pftile == pend) {
          fc_assert(length == 0);
          break;
        }
        pftile = fair_map_tile_step(pisland, pftile, dir);
        fc_assert(pftile != nullptr);
      }
    }
  }

  if (startpos_num > 0) {
    /* Islands with start positions must have the same resources and the
     * same huts. Other ones don't matter. */

    // Make resources.
    if (wld.map.server.riches > 0) {
      fair_map_make_resources(pisland);
    }

    // Make huts.
    if (wld.map.server.huts > 0) {
      fair_map_make_huts(pisland);
    }

    /* Make sure there will be no more resources and huts on assigned
     * tiles. */
    for (i = 0; i < MAP_INDEX_SIZE; i++) {
      pftile = pisland + i;
      if (pftile->flags & FTF_ASSIGNED) {
        pftile->flags = static_cast<fair_tile_flag>(
            pftile->flags | (FTF_NO_RESOURCE | FTF_NO_HUT));
      }
    }
  }

  return pisland;
}

/**
   Build a map using generator 'FAIR'.
 */
bool map_generate_fair_islands()
{
  struct terrain *deepest_ocean =
      pick_ocean(TERRAIN_OCEAN_DEPTH_MAXIMUM, false);
  struct fair_tile *pmap, *pisland;
  int playermass, islandmass1, islandmass2, islandmass3;
  int min_island_size = wld.map.server.tinyisles ? 1 : 2;
  int players_per_island = 1;
  int teams_num = 0, team_players_num = 0, single_players_num = 0;
  int i, iter = CLIP(1, 100000 / map_num_tiles(), 10);
  bool done = false;

  teams_iterate(pteam)
  {
    i = player_list_size(team_members(pteam));
    fc_assert(0 < i);
    if (i == 1) {
      single_players_num++;
    } else {
      teams_num++;
      team_players_num += i;
    }
  }
  teams_iterate_end;
  fc_assert(team_players_num + single_players_num == player_count());

  // Take in account the 'startpos' setting.
  if (wld.map.server.startpos == MAPSTARTPOS_DEFAULT
      && wld.map.server.team_placement == TEAM_PLACEMENT_CONTINENT) {
    wld.map.server.startpos = MAPSTARTPOS_ALL;
  }

  switch (wld.map.server.startpos) {
  case MAPSTARTPOS_2or3: {
    bool maybe2 = (0 == player_count() % 2);
    bool maybe3 = (0 == player_count() % 3);

    if (wld.map.server.team_placement != TEAM_PLACEMENT_DISABLED) {
      teams_iterate(pteam)
      {
        i = player_list_size(team_members(pteam));
        if (i > 1) {
          if (0 != i % 2) {
            maybe2 = false;
          }
          if (0 != i % 3) {
            maybe3 = false;
          }
        }
      }
      teams_iterate_end;
    }

    if (maybe3) {
      players_per_island = 3;
    } else if (maybe2) {
      players_per_island = 2;
    }
  } break;
  case MAPSTARTPOS_ALL:
    if (wld.map.server.team_placement == TEAM_PLACEMENT_CONTINENT) {
      teams_iterate(pteam)
      {
        i = player_list_size(team_members(pteam));
        if (i > 1) {
          if (players_per_island == 1) {
            players_per_island = i;
          } else if (i != players_per_island) {
            /* Every team doesn't have the same number of players. Cannot
             * consider this option. */
            players_per_island = 1;
            wld.map.server.team_placement = TEAM_PLACEMENT_CLOSEST;
            break;
          }
        }
      }
      teams_iterate_end;
    }
    break;
  case MAPSTARTPOS_DEFAULT:
  case MAPSTARTPOS_SINGLE:
  case MAPSTARTPOS_VARIABLE:
    break;
  }
  if (players_per_island == 1) {
    wld.map.server.startpos = MAPSTARTPOS_SINGLE;
  }

  whole_map_iterate(&(wld.map), ptile)
  {
    tile_set_terrain(ptile, deepest_ocean);
    tile_set_continent(ptile, 0);
    BV_CLR_ALL(ptile->extras);
    tile_set_owner(ptile, nullptr, nullptr);
    ptile->extras_owner = nullptr;
  }
  whole_map_iterate_end;

  i = 0;
  if (HAS_POLES) {
    make_polar();

    whole_map_iterate(&(wld.map), ptile)
    {
      if (tile_terrain(ptile) != deepest_ocean) {
        i++;
      }
    }
    whole_map_iterate_end;
  }

  if (wld.map.server.mapsize == MAPSIZE_PLAYER) {
    playermass = wld.map.server.tilesperplayer - i / player_count();
  } else {
    playermass = ((map_num_tiles() * wld.map.server.landpercent - i)
                  / (player_count() * 100));
  }
  islandmass1 = (players_per_island * playermass * 7) / 10;
  if (islandmass1 < min_island_size) {
    islandmass1 = min_island_size;
  }
  islandmass2 = (playermass * 2) / 10;
  if (islandmass2 < min_island_size) {
    islandmass2 = min_island_size;
  }
  islandmass3 = playermass / 10;
  if (islandmass3 < min_island_size) {
    islandmass3 = min_island_size;
  }

  qDebug("Creating a map with fair island generator");
  log_debug("max iterations=%d", iter);
  log_debug("players_per_island=%d", players_per_island);
  log_debug("team_placement=%s",
            team_placement_name(wld.map.server.team_placement));
  log_debug("teams_num=%d, team_players_num=%d, single_players_num=%d",
            teams_num, team_players_num, single_players_num);
  log_debug("playermass=%d, islandmass1=%d, islandmass2=%d, islandmass3=%d",
            playermass, islandmass1, islandmass2, islandmass3);

  pmap = new fair_tile[MAP_INDEX_SIZE]();

  while (--iter >= 0) {
    done = true;

    whole_map_iterate(&(wld.map), ptile)
    {
      struct fair_tile *pftile = pmap + tile_index(ptile);

      if (tile_terrain(ptile) != deepest_ocean) {
        pftile->flags = static_cast<fair_tile_flag>(
            pftile->flags | (FTF_ASSIGNED | FTF_NO_HUT));
        adjc_iterate(&(wld.map), ptile, atile)
        {
          struct fair_tile *aftile = pmap + tile_index(atile);

          if (!(aftile->flags & FTF_ASSIGNED)
              && tile_terrain(atile) == deepest_ocean) {
            aftile->flags =
                static_cast<fair_tile_flag>(aftile->flags | FTF_OCEAN);
          }
        }
        adjc_iterate_end;
      }
      pftile->pterrain = tile_terrain(ptile);
      pftile->presource = tile_resource(ptile);
      pftile->extras = *tile_extras(ptile);
    }
    whole_map_iterate_end;

    // Create main player island.
    log_debug("Making main island.");
    pisland = fair_map_island_new(islandmass1, players_per_island);

    log_debug("Place main islands on the map.");
    i = 0;

    if (wld.map.server.team_placement != TEAM_PLACEMENT_DISABLED
        && team_players_num > 0) {
      // Do team placement.
      struct iter_index
          outwards_indices[wld.map.num_iterate_outwards_indices];
      int start_x[teams_num], start_y[teams_num];
      int dx = 0, dy = 0;
      int j, k;

      // Build outwards_indices.
      memcpy(outwards_indices, wld.map.iterate_outwards_indices,
             sizeof(outwards_indices));
      switch (wld.map.server.team_placement) {
      case TEAM_PLACEMENT_DISABLED:
        fc_assert(wld.map.server.team_placement != TEAM_PLACEMENT_DISABLED);
        break;
      case TEAM_PLACEMENT_CLOSEST:
      case TEAM_PLACEMENT_CONTINENT:
        for (j = 0; j < wld.map.num_iterate_outwards_indices; j++) {
          // We want square distances for comparing.
          outwards_indices[j].dist = map_vector_to_sq_distance(
              outwards_indices[j].dx, outwards_indices[j].dy);
        }
        qsort(outwards_indices, wld.map.num_iterate_outwards_indices,
              sizeof(outwards_indices[0]), fair_team_placement_closest);
        break;
      case TEAM_PLACEMENT_HORIZONTAL:
        qsort(outwards_indices, wld.map.num_iterate_outwards_indices,
              sizeof(outwards_indices[0]), fair_team_placement_horizontal);
        break;
      case TEAM_PLACEMENT_VERTICAL:
        qsort(outwards_indices, wld.map.num_iterate_outwards_indices,
              sizeof(outwards_indices[0]), fair_team_placement_vertical);
        break;
      }

      // Make start point for teams.
      if (current_topo_has_flag(TF_WRAPX)) {
        dx = fc_rand(wld.map.xsize);
      }
      if (current_topo_has_flag(TF_WRAPY)) {
        dy = fc_rand(wld.map.ysize);
      }
      for (j = 0; j < teams_num; j++) {
        start_x[j] = (wld.map.xsize * (2 * j + 1)) / (2 * teams_num) + dx;
        start_y[j] = (wld.map.ysize * (2 * j + 1)) / (2 * teams_num) + dy;
        if (current_topo_has_flag(TF_WRAPX)) {
          start_x[j] = FC_WRAP(start_x[j], wld.map.xsize);
        }
        if (current_topo_has_flag(TF_WRAPY)) {
          start_y[j] = FC_WRAP(start_y[j], wld.map.ysize);
        }
      }
      // Randomize.
      array_shuffle(start_x, teams_num);
      array_shuffle(start_y, teams_num);

      j = 0;
      teams_iterate(pteam)
      {
        int members_count = player_list_size(team_members(pteam));
        int team_id;
        int x, y;

        if (members_count <= 1) {
          continue;
        }
        team_id = team_number(pteam);

        NATIVE_TO_MAP_POS(&x, &y, start_x[j], start_y[j]);
        qDebug("Team %d (%s) will start on (%d, %d)", team_id,
               team_rule_name(pteam), x, y);

        for (k = 0; k < members_count; k += players_per_island) {
          if (!fair_map_place_island_team(pmap, x, y, pisland,
                                          outwards_indices, team_id)) {
            qDebug("Failed to place island number %d for team %d (%s).", k,
                   team_id, team_rule_name(pteam));
            done = false;
            break;
          }
        }
        if (!done) {
          break;
        }
        i += k;
        j++;
      }
      teams_iterate_end;

      fc_assert(!done || i == team_players_num);
    }

    if (done) {
      // Place last player islands.
      for (; i < player_count(); i += players_per_island) {
        if (!fair_map_place_island_rand(pmap, pisland)) {
          qDebug("Failed to place island number %d.", i);
          done = false;
          break;
        }
      }
      fc_assert(!done || i == player_count());
    }
    fair_map_destroy(pisland);

    if (done) {
      log_debug("Create and place small islands on the map.");
      for (i = 0; i < player_count(); i++) {
        pisland = fair_map_island_new(islandmass2, 0);
        if (!fair_map_place_island_rand(pmap, pisland)) {
          qDebug("Failed to place small island2 number %d.", i);
          done = false;
          fair_map_destroy(pisland);
          break;
        }
        fair_map_destroy(pisland);
      }
    }
    if (done) {
      for (i = 0; i < player_count(); i++) {
        pisland = fair_map_island_new(islandmass3, 0);
        if (!fair_map_place_island_rand(pmap, pisland)) {
          qDebug("Failed to place small island3 number %d.", i);
          done = false;
          fair_map_destroy(pisland);
          break;
        }
        fair_map_destroy(pisland);
      }
    }

    if (done) {
      break;
    }

    fair_map_destroy(pmap);
    pmap = new fair_tile[MAP_INDEX_SIZE]();

    // Decrease land mass, for better chances.
    islandmass1 = (islandmass1 * 99) / 100;
    if (islandmass1 < min_island_size) {
      islandmass1 = min_island_size;
    }
    islandmass2 = (islandmass2 * 99) / 100;
    if (islandmass2 < min_island_size) {
      islandmass2 = min_island_size;
    }
    islandmass3 = (islandmass3 * 99) / 100;
    if (islandmass3 < min_island_size) {
      islandmass3 = min_island_size;
    }
  }

  if (!done) {
    qDebug("Failed to create map after %d iterations.", iter);
    wld.map.server.generator = MAPGEN_ISLAND;
    return false;
  }

  // Finalize the map.
  for (i = 0; i < MAP_INDEX_SIZE; i++) {
    // Mark all tiles as assigned, for adding resources and huts.
    pmap[i].flags =
        static_cast<fair_tile_flag>(pmap[i].flags | FTF_ASSIGNED);
  }
  if (wld.map.server.riches > 0) {
    fair_map_make_resources(pmap);
  }
  if (wld.map.server.huts > 0) {
    fair_map_make_huts(pmap);
  }

  // Apply the map.
  log_debug("Applying the map...");
  whole_map_iterate(&(wld.map), ptile)
  {
    struct fair_tile *pftile = pmap + tile_index(ptile);

    fc_assert(pftile->pterrain != nullptr);
    tile_set_terrain(ptile, pftile->pterrain);
    ptile->extras = pftile->extras;
    tile_set_resource(ptile, pftile->presource);
    if (pftile->flags & FTF_STARTPOS) {
      struct startpos *psp = map_startpos_new(ptile);

      if (pftile->startpos_team_id != -1) {
        player_list_iterate(
            team_members(team_by_number(pftile->startpos_team_id)), pplayer)
        {
          startpos_allow(psp, nation_of_player(pplayer));
        }
        player_list_iterate_end;
      } else {
        startpos_allows_all(psp);
      }
    }
  }
  whole_map_iterate_end;

  wld.map.server.have_resources = true;
  wld.map.server.have_huts = true;

  fair_map_destroy(pmap);

  qDebug("Fair island map created with success!");
  return true;
}
