// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_roads.h"

#include "bitvector.h"
#include "extras.h"
#include "fc_types.h"
#include "layer.h"
#include "terrain.h"
#include "tilespec.h"
#include "unit.h"

#include <bitset>

/**
 * \class freeciv::layer_roads
 * \brief Draws "road" extras on the map.
 *
 * Road extras are those with style RoadAllSeparate, RoadParityCombined, and
 * RoadAllCombined.
 */

namespace freeciv {

/**
 * Constructor
 */
layer_roads::layer_roads(struct tileset *ts)
    : freeciv::layer(ts, LAYER_ROADS)
{
}

/**
 * Collects all extras to be drawn.
 */
void layer_roads::initialize_extra(const extra_type *extra,
                                   const QString &tag, extrastyle_id style)
{
  // Make sure they are all sized properly.
  m_all_combined.resize(terrain_count());
  m_all_separate.resize(terrain_count());
  m_parity_combined.resize(terrain_count());

  // This would get a bit repetitive without a macro
#define INIT(vector, func)                                                  \
  /* Make sure we have enough space */                                      \
  terrain_type_iterate(terrain)                                             \
  {                                                                         \
    /* Add an entry for this extra */                                       \
    auto &data = vector[terrain_index(terrain)].emplace_back();             \
    /* Initializes it */                                                    \
    initialize_corners(data, extra, tag, terrain);                          \
    func(data, tag, terrain);                                               \
  }                                                                         \
  terrain_type_iterate_end;

  if (style == ESTYLE_ROAD_ALL_COMBINED) {
    INIT(m_all_combined, initialize_all_combined);
  } else if (style == ESTYLE_ROAD_ALL_SEPARATE) {
    INIT(m_all_separate, initialize_all_separate);
  } else if (style == ESTYLE_ROAD_PARITY_COMBINED) {
    INIT(m_parity_combined, initialize_parity_combined);
  }
#undef INIT
}

/**
 * Initializes "corner" sprite data.
 */
void layer_roads::initialize_corners(corner_sprites &data,
                                     const extra_type *extra,
                                     const QString &tag,
                                     const terrain *terrain)
{
  data.extra = extra;

  // Load special corner road sprites
  for (int i = 0; i < tileset_num_valid_dirs(tileset()); ++i) {
    auto dir = tileset_valid_dirs(tileset())[i];
    if (!is_cardinal_tileset_dir(tileset(), dir)) {
      QString dtn = QStringLiteral("_c_%1").arg(dir_get_tileset_name(dir));
      data.corners[dir] =
          load_sprite(make_tag_terrain_list(tag, dtn, terrain), false);
    }
  }
}

/**
 * Initializes sprite data for RoadAllCombined.
 */
void layer_roads::initialize_all_combined(all_combined_data &data,
                                          const QString &tag,
                                          const terrain *terrain)
{
  // Just go around clockwise, with all combinations.
  auto count = 1 << tileset_num_valid_dirs(tileset());
  for (int i = 0; i < count; ++i) {
    QString idx_str =
        QStringLiteral("_%1").arg(valid_index_str(tileset(), i));
    data.sprites[i] =
        load_sprite(make_tag_terrain_list(tag, idx_str, terrain), true);
  }
}

/**
 * Initializes sprite data for RoadAllSeparate.
 */
void layer_roads::initialize_all_separate(all_separate_data &data,
                                          const QString &tag,
                                          const terrain *terrain)
{
  // Load the isolated sprite
  data.isolated =
      load_sprite(make_tag_terrain_list(tag, "_isolated", terrain), true);

  // Load the directional sprite options, one per direction
  for (int i = 0; i < tileset_num_valid_dirs(tileset()); ++i) {
    auto dir = tileset_valid_dirs(tileset())[i];
    QString dir_name = QStringLiteral("_%1").arg(dir_get_tileset_name(dir));
    data.sprites[i] =
        load_sprite(make_tag_terrain_list(tag, dir_name, terrain), true);
  }
}

/**
 * Initializes sprite data for RoadAllSeparate.
 */
void layer_roads::initialize_parity_combined(parity_combined_data &data,
                                             const QString &tag,
                                             const terrain *terrain)
{
  // Load the isolated sprite
  data.isolated =
      load_sprite(make_tag_terrain_list(tag, "_isolated", terrain), true);

  int num_index = 1 << (tileset_num_valid_dirs(tileset()) / 2);

  // Load the directional sprites
  // The comment below exemplifies square tiles:
  // additional sprites for each road type: 16 each for cardinal and diagonal
  // directions. Each set of 16 provides a NSEW-indexed sprite to provide
  // connectors for all rails in the cardinal/diagonal directions.  The 0
  // entry is unused (the "isolated" sprite is used instead). */
  for (int i = 1; i < num_index; i++) {
    QString cs = QStringLiteral("_c_");
    QString ds = QStringLiteral("_d_");

    for (int j = 0; j < tileset_num_valid_dirs(tileset()) / 2; j++) {
      int value = (i >> j) & 1;
      cs += QStringLiteral("%1%2")
                .arg(dir_get_tileset_name(
                    tileset_valid_dirs(tileset())[2 * j]))
                .arg(value);
      ds += QStringLiteral("%1%2")
                .arg(dir_get_tileset_name(
                    tileset_valid_dirs(tileset())[2 * j + 1]))
                .arg(value);
    }

    data.sprites.first[i] =
        load_sprite(make_tag_terrain_list(tag, cs, terrain), true);
    data.sprites.second[i] =
        load_sprite(make_tag_terrain_list(tag, ds, terrain), true);
  }
}

/* Define some helper functions to decide where to draw roads. */
namespace {
/**
 * Checks if the extra is hidden by another extra.
 */
bool is_hidden(const extra_type *extra, const bv_extras extras)
{
  extra_type_list_iterate(extra->hiders, hider)
  {
    if (BV_ISSET(extras, extra_index(hider))) {
      return true;
    }
  }
  extra_type_list_iterate_end;

  return false;
}

/**
 * Checks if an extra connects from a tile to the given direction.
 */
bool connects(const struct tileset *t, const extra_type *extra,
              direction8 dir, const bv_extras extras,
              const bv_extras extras_near, const terrain *terrain_near)
{
  // If the extra isn't on the starting tile, it doesn't connect.
  if (!BV_ISSET(extras, extra_index(extra))) {
    return false;
  }

  // Some extras (e.g. rivers) only connect in cardinal directions.
  if (is_cardinal_only_road(extra)
      && is_cardinal_tileset_dir(t, static_cast<direction8>(dir))) {
    return false;
  }

  // Check extras we integrate with. An extra always integrates with itself.
  extra_type_list_iterate(extra_road_get(extra)->integrators, iextra)
  {
    if (BV_ISSET(extras_near, extra_index(iextra))) {
      return true;
    }
  }
  extra_type_list_iterate_end;

  // The extra may connect to adjacent land tiles.
  return extra_has_flag(extra, EF_CONNECT_LAND) && terrain_near != T_UNKNOWN
         && terrain_type_terrain_class(terrain_near) != TC_OCEAN;
}

/**
 * Checks if an extra should be drawn in a direction; that is it connects to
 * the other tile and and is not hidden by another extra.
 */
bool should_draw(const struct tileset *t, const extra_type *extra,
                 direction8 dir, const bv_extras extras,
                 const bv_extras extras_near, terrain *terrain_near)
{
  // Only draw if the extra connects to the other tile.
  if (!connects(t, extra, dir, extras, extras_near, terrain_near)) {
    return false;
  }

  // Only draw if the connection is not hidden by another extra.
  extra_type_list_iterate(extra->hiders, hider)
  {
    if (connects(t, hider, dir, extras, extras_near, terrain_near)) {
      return false;
    }
  }
  extra_type_list_iterate_end;

  // Passes all the tests, draw it.
  return true;
}

/**
 * Returns data needed to draw roads on a tile. This is the directions in
 * which roads should be drawn, and whether an "isolated" road should be
 * drawn.
 */
std::tuple<std::bitset<DIR8_MAGIC_MAX>, bool>
road_data(const struct tileset *t, const tile *ptile,
          const extra_type *extra)
{
  const auto extras = *tile_extras(ptile);

  // Don't bother doing anything if the extra isn't on the tile in the first
  // place.
  if (!BV_ISSET(extras, extra_index(extra))) {
    return make_tuple(std::bitset<DIR8_MAGIC_MAX>(), false);
  }

  const auto terrain = tile_terrain(ptile);
  struct terrain *terrain_near[DIR8_MAGIC_MAX] = {nullptr};
  bv_extras extras_near[DIR8_MAGIC_MAX];
  build_tile_data(ptile, terrain, terrain_near, extras_near);

  // Check connections to adjacent tiles.
  std::bitset<DIR8_MAGIC_MAX> draw;
  for (int i = 0; i < tileset_num_valid_dirs(t); ++i) {
    auto dir = tileset_valid_dirs(t)[i];
    draw[i] = should_draw(t, extra, static_cast<direction8>(dir), extras,
                          extras_near[dir], terrain_near[dir]);
  }

  // Draw the isolated sprite if we don't draw anything in any direction and
  // it's not hidden by something else.
  auto isolated = !draw.any()
                  && (!tile_city(ptile) || !gui_options->draw_cities)
                  && !is_hidden(extra, extras);

  return make_tuple(draw, isolated);
}

} // anonymous namespace

/**
 * Returns the sprites to draw roads.
 */
std::vector<drawn_sprite>
layer_roads::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                               const tile_corner *pcorner,
                               const unit *punit) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);

  // Should we draw anything in the first place?
  if (!ptile) {
    return {};
  }

  const auto terrain = tile_terrain(ptile);
  if (!tile_terrain(ptile)) {
    return {};
  }

  // Now we draw
  std::vector<drawn_sprite> sprs;

  // Loop over all extras (picking the data for the correct terrain) and draw
  // them.
  for (const auto &data : m_all_combined[terrain_index(terrain)]) {
    if (is_extra_drawing_enabled(data.extra)) {
      fill_corners(sprs, data, ptile);
      fill_all_combined(sprs, data, ptile);
    }
  }
  for (const auto &data : m_all_separate[terrain_index(terrain)]) {
    if (is_extra_drawing_enabled(data.extra)) {
      fill_corners(sprs, data, ptile);
      fill_all_separate(sprs, data, ptile);
    }
  }
  for (const auto &data : m_parity_combined[terrain_index(terrain)]) {
    if (is_extra_drawing_enabled(data.extra)) {
      fill_corners(sprs, data, ptile);
      fill_parity_combined(sprs, data, ptile);
    }
  }

  return sprs;
}

/**
 * Fills "corner" sprites that help complete diagonal roads where they
 * overlap with adjacent tiles. Only relevant for overhead square tilesets.
 */
void layer_roads::fill_corners(std::vector<drawn_sprite> &sprs,
                               const corner_sprites &data,
                               const tile *ptile) const
{
  if (is_cardinal_only_road(data.extra)) {
    return;
  }
  if (tileset_hex_height(tileset()) != 0
      || tileset_hex_width(tileset()) != 0) {
    return;
  }

  /* Roads going diagonally adjacent to this tile need to be
   * partly drawn on this tile. */

  const auto terrain = tile_terrain(ptile);
  struct terrain *terrain_near[DIR8_MAGIC_MAX] = {nullptr};
  bv_extras extras_near[DIR8_MAGIC_MAX];
  build_tile_data(ptile, terrain, terrain_near, extras_near);

  const auto num_valid_dirs = tileset_num_valid_dirs(tileset());
  const auto valid_dirs = tileset_valid_dirs(tileset());

  /* Draw the corner sprite if:
   * - There is a diagonal connection between two adjacent tiles.
   * - There is no diagonal connection that intersects this connection.
   *
   * That is, if we are drawing tile X below:
   *
   *    +--+--+
   *    |NW|N |
   *    +--+--+
   *    | W|X |
   *    +--+--+
   *
   * We draw a corner on X if:
   * - W and N are connected;
   * - X and NW are not.
   * This gets a bit more complicated with hiders, thus we call
   * should_draw().
   */
  for (int i = 0; i < num_valid_dirs; ++i) {
    enum direction8 dir = valid_dirs[i];

    if (!is_cardinal_tileset_dir(tileset(), dir)) {
      // Draw corner sprites for this non-cardinal direction (= NW on the
      // schema).

      // = W on the schema
      auto cardinal_before =
          valid_dirs[(i + num_valid_dirs - 1) % num_valid_dirs];
      // = N on the schema
      auto cardinal_after = valid_dirs[(i + 1) % num_valid_dirs];

      /* should_draw() needs to know the direction in which the road would go
       * to account for cardinal-only roads and hiders. We assume that all
       * roads are bidirectional, that is going from N to W is the same as
       * going from W to N. This may break with roads connecting  to land,
       * but it is a rare occurrence.
       *
       * We already know the non-cardinal direction where we do not want a
       * connection (NW above). To compute the other one, remark that going
       * from the W tile to the N tile is going NE, or N+1 counting
       * clockwise. This is what we compute here: */
      auto relative_dir = valid_dirs[(i + 2) % num_valid_dirs];

      if (data.corners[dir]
          && !should_draw(tileset(), data.extra, dir, *tile_extras(ptile),
                          extras_near[dir], terrain_near[dir])
          && should_draw(tileset(), data.extra, relative_dir,
                         extras_near[cardinal_before],
                         extras_near[cardinal_after],
                         terrain_near[cardinal_after])) {
        sprs.emplace_back(tileset(), data.corners[dir]);
      }
    }
  }
}

/**
 * Fill sprites for extras with type RoadAllCombined. It is a very simple
 * method that lets us simply retrieve entire finished tiles, with a bitwise
 * index of the presence of roads in each direction.
 */
void layer_roads::fill_all_combined(std::vector<drawn_sprite> &sprs,
                                    const all_combined_data &data,
                                    const tile *ptile) const
{
  auto [draw, isolated] = road_data(tileset(), ptile, data.extra);

  // Draw the sprite
  if (draw.any() || isolated) {
    sprs.emplace_back(tileset(), data.sprites[draw.to_ulong()]);
  }
}

/**
 * Fill sprites for extras with type RoadAllSeparate. We simply draw one
 * sprite for every connection. This means we only need a few sprites, but a
 * lot of drawing is necessary and it generally doesn't look very good.
 */
void layer_roads::fill_all_separate(std::vector<drawn_sprite> &sprs,
                                    const all_separate_data &data,
                                    const tile *ptile) const
{
  auto [draw, isolated] = road_data(tileset(), ptile, data.extra);

  // Draw the sprites
  for (int i = 0; i < tileset_num_valid_dirs(tileset()); ++i) {
    auto dir = tileset_valid_dirs(tileset())[i];
    if (draw[dir]) {
      sprs.emplace_back(tileset(), data.sprites[dir]);
    }
  }

  // Draw the isolated sprite if needed
  if (isolated) {
    sprs.emplace_back(tileset(), data.isolated);
  }
}

/**
 * Fill sprites for extras with type RoadAllSeparate. We draw one sprite for
 * cardinal road connections, one sprite for diagonal road connections. This
 * means we need about 4x more sprites than in style 0, but up to 4x less
 * drawing is needed. The drawing quality may also be improved.
 */
void layer_roads::fill_parity_combined(std::vector<drawn_sprite> &sprs,
                                       const parity_combined_data &data,
                                       const tile *ptile) const
{
  auto [draw, isolated] = road_data(tileset(), ptile, data.extra);

  // Pick up the sprites
  int even_tileno = 0, odd_tileno = 0;
  for (int i = 0; i < tileset_num_valid_dirs(tileset()) / 2; i++) {
    auto even = 2 * i;
    auto odd = 2 * i + 1;

    if (draw[even]) {
      even_tileno |= 1 << i;
    }
    if (draw[odd]) {
      odd_tileno |= 1 << i;
    }
  }

  // Draw the cardinal/even roads first
  if (even_tileno != 0) {
    sprs.emplace_back(tileset(), data.sprites.first[even_tileno]);
  }
  if (odd_tileno != 0) {
    sprs.emplace_back(tileset(), data.sprites.second[odd_tileno]);
  }

  // Draw the isolated sprite if needed
  if (isolated) {
    sprs.emplace_back(tileset(), data.isolated);
  }
}

void layer_roads::reset_ruleset()
{
  m_all_combined.clear();
  m_all_separate.clear();
  m_parity_combined.clear();
}

} // namespace freeciv
