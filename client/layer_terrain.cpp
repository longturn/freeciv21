/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 2021 Freeciv21 contributors.
\_   \        /  __/                        This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | \ \  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include "layer_terrain.h"

#include "climap.h"
#include "extras.h"
#include "game.h" // For extra_type_iterate
#include "map.h"
#include "rand.h"
#include "sprite_g.h"
#include "tilespec.h"

/**
 * \class freeciv::layer_terrain
 * \brief Draws terrain sprites on the map.
 *
 * Terrain layers are a core feature of the map. They are used to draw
 * sprites representing terrain. Up to three layers can be used. Within each
 * layer, one can choose how each terrain will be drawn.
 *
 * The configuration is based on *tags* that are used when constructing
 * sprite identifiers. Tags represent sets of related sprites that, drawn in
 * a specific way, construct the visual representation of a tile. The tag
 * used to represent a terrain is taken from its `graphic_str` or
 * `graphic_alt` properties.
 *
 * Each tag has a set of properties that influence how sprites are drawn. The
 * offsets at which the sprites are drawn is set using \ref set_tag_offsets.
 * The tiles can be drawn in two ways: either a single sprite is used for the
 * whole tile (\ref CELL_WHOLE), or one sprite is used to draw each corner of
 * the tile
 * (\ref CELL_CORNER). This is set with \ref set_tag_sprite_type. "Matching"
 * with adjacent terrain types provides a powerful mechanism for
 * sophisticated effects. For isometric tilesets, it is also possible to use
 * a mask to blend adjacent tiles together with \ref enable_blending.
 *
 * Matching
 * --------
 *
 * The sprites used to draw a tile can depend on adjacent terrains, allowing
 * the representation of continuous coasts, ridges and other edges.
 *
 * To use tag matching, one first defines a number of terrain groups ("match
 * type" in `spec` files). Groups are created with \ref
 * create_matching_group. Every tag must be in a group, set with  \ref
 * set_tag_matching_group. The first letter of group names must be unique
 * within a layer. Each tag can then be matched against any number of groups.
 * There will be one sprite for each combination of groups next to the tile
 * of interest.
 *
 * The simplest matching is no matching, in which case the sprite used
 * doesn't depend on adjacent terrain. This is available for both \ref
 * CELL_WHOLE and \ref CELL_CORNER, although there is little use for the
 * second. The sprite names for \ref CELL_WHOLE are formed like
 * `t.l0.grassland1`, where 0 is the layer number, `grassland` appears in the
 * name of the `tilespec` section, and 1 is an index (when there are several
 * sprites with indices 1, 2, 3, ..., one is picked at random). For
 * \ref CELL_CORNER, the names are like `t.l0.grassland_cell_u`, where `u`
 * ("up") indicates the direction of the corner. It can be `u` ("up"), `d`
 * ("down"), `r` ("right"), or `l` ("left").
 *
 * When a tag is matched against one group, there are two possibilities:
 *
 *  * The matched group is the same as the tag group. For \ref CELL_WHOLE,
 * this requires sprites with names like `t.l0.grassland_n1e0s0w0`, where the
 * `n1` indicates that the terrain in the north direction is in the same
 * group as the tile that is being drawn, and the 0's indicate that other
 * terrains are different. Sprites must be provided for all 16  combinations
 * of 0's and 1's. Amplio2 forests and hills are drawn this way.
 *
 *    For \ref CELL_CORNER, this requires 24 sprites with names like
 *    `t.l0.grassland_cell_u010`. `t.l0.grassland_cell_u` is like in the no
 *    match case, and `010` indicates which sides of the corner match the
 *    terrain being drawn. Amplio2 ice uses this method.
 *
 *  * The matched group is different from the tag group (only supported for
 *    \ref CELL_CORNER). There are again 24 sprites, this time with names
 * like `t.l0.grassland_cell_u_a_a_b`. The first letter, in this example `u`,
 * is the direction of the corner. The other three indicate which terrains
 * are found on the three external sides of the corner. They are the first
 * letter of the name of a matching group: the group being matched against if
 * the adjacent terrain is of that group, and otherwise the group of the
 * sprite being drawn. The coasts of Amplio2 lakes use this method.
 *
 * When more than one group is used, which is only supported for
 * \ref CELL_CORNER, the sprites are named like `t.l0.cellgroup_a_b_c_d`.
 * Each sprite represents the junction of four tiles with the group names
 * first letters `a`, `b`, `c`, and `d`. Each sprite is split in four to
 * provide four corner sprites. Amplio2 coasts are drawn this way.
 */

namespace {
static const char direction4letters[5] = "udrl";
}

namespace freeciv {

layer_terrain::layer_terrain(struct tileset *ts, int number)
    : freeciv::layer(ts, LAYER_TERRAIN1), m_number(number)
{
}

/**
 * \brief Creates a matching group with the given name.
 * \returns Whether the operation succeeded.
 */
bool layer_terrain::create_matching_group(const QString &name)
{
  if (name.isEmpty()) {
    tileset_error(tileset(), LOG_ERROR,
                  _("[layer%d] match_types: names cannot be empty."),
                  m_number);
    return false;
  }

  if (m_matching_groups.count(name.at(0)) != 0) {
    tileset_error(
        tileset(), LOG_ERROR,
        _("[layer%d] match_types: \"%s\" initial ('%c') is not unique."),
        m_number, qUtf8Printable(name), qUtf8Printable(name.at(0)));
    return false;
  }

  m_matching_groups[name.at(0)] =
      matching_group{static_cast<int>(m_matching_groups.size()), name};

  return true;
}

/**
 * \brief Makes a terrain tag available for use by this layer.
 *
 * This function only makes the tag available. Its properties must be set
 * using the `set_tag_*` functions.
 *
 * \returns True if the tag did not exist.
 */
bool layer_terrain::add_tag(const QString &tag, const QString &sprite_name)
{
  bool ok = true;
  if (m_terrain_tag_info.count(tag) > 0) {
    tileset_error(tileset(), LOG_ERROR,
                  "Multiple [tile] sections containing terrain tag \"%s\".",
                  qUtf8Printable(tag));
    ok = false;
  }

  m_terrain_tag_info[tag].sprite_name = sprite_name; // Creates the data
  return ok;
}

/**
 * \brief Sets the type of sprite used to draw the specified tag.
 *
 * The tag must have been created using \ref add_tag.
 */
bool layer_terrain::set_tag_sprite_type(const QString &tag, sprite_type type)
{
  // FIXME The ancient code did not handle CELL_CORNER for "tall" terrain or
  //       sprite offsets. Does the new code work support that?
  m_terrain_tag_info.at(tag).type = type;
  return true;
}

/**
 * \brief Sets the offsets used to draw the specified tag.
 *
 * The tag must have been created using \ref add_tag.
 */
bool layer_terrain::set_tag_offsets(const QString &tag, int offset_x,
                                    int offset_y)
{
  m_terrain_tag_info.at(tag).offset_x = offset_x;
  m_terrain_tag_info.at(tag).offset_y = offset_y;
  return true;
}

/**
 * \brief Sets the matching group for the specified tag.
 *
 * The tag must have been created using \ref add_tag.
 */
bool layer_terrain::set_tag_matching_group(const QString &tag,
                                           const QString &group_name)
{
  if (auto g = group(group_name); g != nullptr) {
    m_terrain_tag_info.at(tag).group = g;
    // Tags always match with themselves
    m_terrain_tag_info.at(tag).matches_with.push_back(g);
    return true;
  } else {
    return false;
  }
}

/**
 * \brief Sets the specified tag to be matched against a group.
 *
 * The tag must have been created using \ref add_tag.
 */
bool layer_terrain::set_tag_matches_with(const QString &tag,
                                         const QString &group_name)
{
  if (auto g = group(group_name); g != nullptr) {
    m_terrain_tag_info.at(tag).matches_with.push_back(g);
    return true;
  } else {
    return false;
  }
}

/**
 * \brief Enable blending on this layer for the given terrain tag.
 */
void layer_terrain::enable_blending(const QString &tag)
{
  if (!tileset_is_isometric(tileset())) {
    tileset_error(tileset(), LOG_ERROR,
                  _("Blending is only supported for isometric tilesets"));
    return;
  }
  // Create the entry
  m_terrain_tag_info.at(tag).blend = true;
}

/**
 * \brief Sets up the structure to draw the specified terrain.
 */
void layer_terrain::initialize_terrain(const terrain *terrain)
{
  // Find the good terrain_info
  terrain_info info;
  if (m_terrain_tag_info.count(terrain->graphic_str) > 0) {
    info = m_terrain_tag_info[terrain->graphic_str];
  } else if (m_terrain_tag_info.count(terrain->graphic_alt) > 0) {
    info = m_terrain_tag_info[terrain->graphic_alt];
  } else if (m_number == 0) {
    // All terrains should be present in layer 0...
    tileset_error(tileset(), LOG_WARN,
                  _("Terrain \"%s\": no graphic tile \"%s\" or \"%s\"."),
                  terrain_rule_name(terrain), terrain->graphic_str,
                  terrain->graphic_alt);
    return;
  } else {
    // Terrain is not drawn by this layer
    return;
  }

  // Determine the match style
  switch (info.matches_with.size()) {
  case 0:
  case 1:
    info.style = MATCH_NONE;
    break;
  case 2:
    if (info.matches_with.front() == info.matches_with.back()) {
      info.style = MATCH_SAME;
    } else {
      info.style = MATCH_PAIR;
    }
    break;
  default:
    info.style = MATCH_FULL;
    break;
  }

  // Build graphics data
  // TODO Use inheritance?
  switch (info.type) {
  case CELL_WHOLE:
    switch (info.style) {
    case MATCH_NONE:
      initialize_cell_whole_match_none(terrain, info);
      break;
    case MATCH_SAME:
      initialize_cell_whole_match_same(terrain, info);
      break;
    case MATCH_PAIR:
    case MATCH_FULL:
      tileset_error(
          tileset(), LOG_ERROR,
          _("Not implemented CELL_WHOLE + MATCH_FULL/MATCH_PAIR."));
      return;
    }
    break;
  case CELL_CORNER:
    switch (info.style) {
    case MATCH_NONE:
      initialize_cell_corner_match_none(terrain, info);
      break;
    case MATCH_SAME:
      initialize_cell_corner_match_same(terrain, info);
      break;
    case MATCH_PAIR:
      initialize_cell_corner_match_pair(terrain, info);
      break;
    case MATCH_FULL:
      initialize_cell_corner_match_full(terrain, info);
      break;
    }
    break;
  }

  // Blending
  initialize_blending(terrain, info);

  // Copy it to the terrain index
  m_terrain_info[terrain_index(terrain)] = info;
}

/**
 * \brief Sets up terrain information for \ref CELL_WHOLE and `MATCH_SAME`.
 */
void layer_terrain::initialize_cell_whole_match_none(const terrain *terrain,
                                                     terrain_info &info)
{
  QString buffer;

  // Load whole sprites for this tile.
  for (int i = 0;; i++) {
    buffer = QStringLiteral("t.l%1.%2%3")
                 .arg(m_number)
                 .arg(info.sprite_name)
                 .arg(i + 1);
    auto sprite = load_sprite(tileset(), buffer, true, false);
    if (!sprite) {
      break;
    }
    info.sprites.push_back(sprite);
  }

  // check for base sprite, allowing missing sprites above base
  if (m_number == 0 && info.sprites.empty()) {
    // TRANS: 'base' means 'base of terrain gfx', not 'military base'
    tileset_error(tileset(), LOG_ERROR,
                  _("Missing base sprite for tag \"%s\"."),
                  qUtf8Printable(buffer));
  }
}

/**
 * \brief Sets up terrain information for \ref CELL_WHOLE and `MATCH_SAME`.
 */
void layer_terrain::initialize_cell_whole_match_same(const terrain *terrain,
                                                     terrain_info &info)
{
  // Load 16 cardinally-matched sprites.
  for (int i = 0; i < tileset_num_index_cardinals(tileset()); i++) {
    auto buffer = QStringLiteral("t.l%1.%2_%3")
                      .arg(m_number)
                      .arg(info.sprite_name)
                      .arg(cardinal_index_str(tileset(), i));
    info.sprites.push_back(tiles_lookup_sprite_tag_alt(
        tileset(), LOG_ERROR, qUtf8Printable(buffer), "", "matched terrain",
        terrain_rule_name(terrain), true));
  }
}

/**
 * \brief Sets up terrain information for \ref CELL_CORNER and `MATCH_NONE`.
 */
void layer_terrain::initialize_cell_corner_match_none(const terrain *terrain,
                                                      terrain_info &info)
{
  // Determine how many sprites we need
  int number = NUM_CORNER_DIRS;

  // Load the sprites
  for (int i = 0; i < number; i++) {
    enum direction4 dir = static_cast<direction4>(i % NUM_CORNER_DIRS);
    auto buffer = QStringLiteral("t.l%1.%2_cell_%3")
                      .arg(m_number)
                      .arg(info.sprite_name)
                      .arg(direction4letters[dir]);
    info.sprites.push_back(tiles_lookup_sprite_tag_alt(
        tileset(), LOG_ERROR, qUtf8Printable(buffer), "", "cell terrain",
        terrain_rule_name(terrain), true));
  }
}

/**
 * \brief Sets up terrain information for \ref CELL_CORNER and `MATCH_SAME`.
 */
void layer_terrain::initialize_cell_corner_match_same(const terrain *terrain,
                                                      terrain_info &info)
{
  // Determine how many sprites we need
  int number = NUM_CORNER_DIRS * 2 * 2 * 2;

  // Load the sprites
  for (int i = 0; i < number; i++) {
    enum direction4 dir = static_cast<direction4>(i % NUM_CORNER_DIRS);
    int value = i / NUM_CORNER_DIRS;

    auto buffer = QStringLiteral("t.l%1.%2_cell_%3%4%5%6")
                      .arg(m_number)
                      .arg(info.sprite_name)
                      .arg(direction4letters[dir])
                      .arg(value & 1)
                      .arg((value >> 1) & 1)
                      .arg((value >> 2) & 1);
    info.sprites.push_back(tiles_lookup_sprite_tag_alt(
        tileset(), LOG_ERROR, qUtf8Printable(buffer), "",
        "same cell terrain", terrain_rule_name(terrain), true));
  }
}

/**
 * \brief Sets up terrain information for \ref CELL_CORNER and `MATCH_PAIR`.
 */
void layer_terrain::initialize_cell_corner_match_pair(const terrain *terrain,
                                                      terrain_info &info)
{
  // Determine how many sprites we need
  int number = NUM_CORNER_DIRS * 2 * 2 * 2;

  // Load the sprites
  for (int i = 0; i < number; i++) {
    enum direction4 dir = static_cast<direction4>(i % NUM_CORNER_DIRS);
    int value = i / NUM_CORNER_DIRS;

    QChar letters[2] = {info.group->name[0],
                        info.matches_with.back()->name[0]};
    auto buffer = QStringLiteral("t.l%1.%2_cell_%3_%4_%5_%6")
                      .arg(m_number)
                      .arg(info.sprite_name, QChar(direction4letters[dir]),
                           letters[value & 1], letters[(value >> 1) & 1],
                           letters[(value >> 2) & 1]);

    info.sprites.push_back(tiles_lookup_sprite_tag_alt(
        tileset(), LOG_ERROR, qUtf8Printable(buffer), "",
        "cell pair terrain", terrain_rule_name(terrain), true));
  }
}

/**
 * \brief Sets up terrain information for \ref CELL_CORNER and `MATCH_FULL`.
 */
void layer_terrain::initialize_cell_corner_match_full(const terrain *terrain,
                                                      terrain_info &info)
{
  // Determine how many sprites we need
  // N directions (NSEW) * 3 dimensions of matching
  // could use exp() or expi() here?
  const int count = info.matches_with.size();
  const int number = NUM_CORNER_DIRS * count * count * count;

  // Load the sprites
  for (int i = 0; i < number; i++) {
    enum direction4 dir = static_cast<direction4>(i % NUM_CORNER_DIRS);
    int value = i / NUM_CORNER_DIRS;

    const auto g1 = info.matches_with[value % count];
    value /= count;
    const auto g2 = info.matches_with[value % count];
    value /= count;
    const auto g3 = info.matches_with[value % count];

    const matching_group *n, *s, *e, *w;
    // Assume merged cells.  This should be a separate option.
    switch (dir) {
    case DIR4_NORTH:
      s = info.group;
      w = g1;
      n = g2;
      e = g3;
      break;
    case DIR4_EAST:
      w = info.group;
      n = g1;
      e = g2;
      s = g3;
      break;
    case DIR4_SOUTH:
      n = info.group;
      e = g1;
      s = g2;
      w = g3;
      break;
    case DIR4_WEST:
    default: // avoid warnings
      e = info.group;
      s = g1;
      w = g2;
      n = g3;
      break;
    };

    // Use first character of match_types, already checked for uniqueness.
    auto buffer = QStringLiteral("t.l%1.cellgroup_%2_%3_%4_%5")
                      .arg(QString::number(m_number), n->name[0], e->name[0],
                           s->name[0], w->name[0]);
    auto sprite = load_sprite(tileset(), buffer, true, false);

    if (sprite) {
      // Crop the sprite to separate this cell.
      const int W = sprite->width();
      const int H = sprite->height();
      int x[4] = {W / 4, W / 4, 0, W / 2};
      int y[4] = {H / 2, 0, H / 4, H / 4};
      int xo[4] = {0, 0, -W / 2, W / 2};
      int yo[4] = {H / 2, -H / 2, 0, 0};

      sprite = crop_sprite(sprite, x[dir], y[dir], W / 2, H / 2,
                           get_mask_sprite(tileset()), xo[dir], yo[dir],
                           1.0f, false);
      // We allocated new sprite with crop_sprite. Store its address so we
      // can free it.
      m_allocated.emplace_back(sprite);
    } else {
      tileset_error(tileset(), LOG_ERROR,
                    "Terrain graphics sprite for tag \"%s\" missing.",
                    qUtf8Printable(buffer));
    }

    info.sprites.push_back(sprite);
  }
}

/**
 * \brief Initializes blending sprites.
 */
void layer_terrain::initialize_blending(const terrain *terrain,
                                        terrain_info &info)
{
  // Get the blending info
  if (!info.blend) {
    // No blending
    return;
  }

  // try an optional special name
  auto buffer = QStringLiteral("t.blend.%1").arg(info.sprite_name);
  auto blender = tiles_lookup_sprite_tag_alt(
      tileset(), LOG_VERBOSE, qUtf8Printable(buffer), "", "blend terrain",
      terrain_rule_name(terrain), true);

  if (blender == nullptr) {
    // try an unloaded base name
    // Need to pass "1" as an argument because %21 is interpreted as argument
    // 21
    buffer = QStringLiteral("t.l%1.%2%3")
                 .arg(m_number)
                 .arg(info.sprite_name)
                 .arg(1);
    blender = tiles_lookup_sprite_tag_alt(
        tileset(), LOG_ERROR, qUtf8Printable(buffer), "",
        "base (blend) terrain", terrain_rule_name(terrain), true);
  }

  if (blender == nullptr) {
    tileset_error(
        tileset(), LOG_ERROR,
        "Cannot find sprite for blending terrain with tag %s on layer %d",
        qUtf8Printable(info.sprite_name), m_number);
    return;
  }

  // Set up blending sprites. This only works in iso-view!
  const int W = tileset_tile_width(tileset());
  const int H = tileset_tile_height(tileset());
  const int offsets[4][2] = {{W / 2, 0}, {0, H / 2}, {W / 2, H / 2}, {0, 0}};
  int dir = 0;

  for (; dir < 4; dir++) {
    info.blend_sprites[dir] =
        crop_sprite(blender, offsets[dir][0], offsets[dir][1], W / 2, H / 2,
                    get_dither_sprite(tileset()), 0, 0, 1.0f, false);
  }
}

/**
 * \implements layer::fill_sprite_array
 */
std::vector<drawn_sprite> layer_terrain::fill_sprite_array(
    const tile *ptile, const tile_edge *pedge, const tile_corner *pcorner,
    const unit *punit, const city *pcity, const unit_type *putype) const
{
  if (ptile == nullptr) {
    return {};
  }

  const auto terrain = ptile->terrain;
  if (terrain == nullptr) {
    return {};
  }

  // Don't draw terrain when the solid background is used
  if (solid_background(ptile, punit, pcity)) {
    return {};
  }

  auto sprites = std::vector<drawn_sprite>();

  // Handle scenario-defined sprites: scenarios can instruct the client to
  // draw a specific sprite at some location.
  // FIXME: this should avoid calling load_sprite since it's slow and
  // increases the refcount without limit.
  if (QPixmap * sprite;
      ptile->spec_sprite
      && (sprite =
              load_sprite(tileset(), ptile->spec_sprite, true, false))) {
    if (m_number == 0) {
      sprites.emplace_back(tileset(), sprite);
    }
    // Skip the normal drawing process.
    return sprites;
  }

  struct terrain *terrain_near[8] = {nullptr};
  bv_extras extras_near[8]; // dummy
  build_tile_data(ptile, terrain, terrain_near, extras_near);

  fill_terrain_sprite_array(sprites, ptile, terrain, terrain_near);
  fill_blending_sprite_array(sprites, ptile, terrain, terrain_near);

  return sprites;
}

/**
 * Retrieves the group structure of the provided name.
 * \param name The name to look for.
 * \returns The terrain group structure, or `nullptr` on failure.
 */
layer_terrain::matching_group *layer_terrain::group(const QString &name)
{
  if (name.isEmpty() || m_matching_groups.count(name[0]) == 0) {
    tileset_error(tileset(), LOG_ERROR, _("No matching group called %s"),
                  qUtf8Printable(name));
    return nullptr;
  }

  auto candidate = &m_matching_groups.at(name[0]);
  if (candidate->name == name) {
    return candidate;
  } else {
    // Should not happen
    return nullptr;
  }
}

/**
 * Retrieves the group number for a given terrain.
 * \param pterrain The terrain to look for. Can be null.
 * \returns The terrain group number, or -1 if not drawn in this layer.
 */
int layer_terrain::terrain_group(const terrain *pterrain) const
{
  if (!pterrain || m_terrain_info.count(terrain_index(pterrain)) == 0) {
    return -1;
  }

  return m_terrain_info.at(terrain_index(pterrain)).group->number;
}

/**
 * Helper function for fill_sprite_array.
 */
void layer_terrain::fill_terrain_sprite_array(
    std::vector<drawn_sprite> &sprs, const tile *ptile,
    const terrain *pterrain, terrain **tterrain_near) const
{
  if (m_terrain_info.find(terrain_index(pterrain)) == m_terrain_info.end()) {
    // Not drawn in this layer
    return;
  }

  const auto info = m_terrain_info.at(terrain_index(pterrain));

#define MATCH(dir) terrain_group(tterrain_near[(dir)])

  switch (info.type) {
  case CELL_WHOLE: {
    switch (info.style) {
    case MATCH_NONE: {
      if (!info.sprites.empty()) {
        /* Pseudo-random reproducable algorithm to pick a sprite. Use
         * modulo to limit the number to a handleable size [0..32000). */
        const int i =
            fc_randomly(tile_index(ptile) % 32000, info.sprites.size());
        if (Q_LIKELY(info.sprites[i] != nullptr)) {
          sprs.emplace_back(tileset(), info.sprites[i], true, info.offset_x,
                            info.offset_y);
        }
      }
      break;
    }
    case MATCH_SAME: {
      fc_assert_ret(info.matches_with.size() == 2);
      fc_assert_ret(info.sprites.size()
                    == tileset_num_index_cardinals(tileset()));
      int tileno = 0;

      for (int i = 0; i < tileset_num_cardinal_dirs(tileset()); i++) {
        enum direction8 dir = tileset_cardinal_dirs(tileset())[i];

        if (MATCH(dir) == info.matches_with.back()->number) {
          tileno |= 1 << i;
        }
      }
      if (Q_LIKELY(info.sprites[tileno] != nullptr)) {
        sprs.emplace_back(tileset(), info.sprites[tileno], true,
                          info.offset_x, info.offset_y);
      }
      break;
    }
    case MATCH_PAIR:
    case MATCH_FULL:
      fc_assert(false); // not yet defined
      break;
    };
    break;
  }
  case CELL_CORNER: {
    /* Divide the tile up into four rectangular cells.  Each of these
     * cells covers one corner, and each is adjacent to 3 different
     * tiles.  For each cell we pick a sprite based upon the adjacent
     * terrains at each of those tiles.  Thus, we have 8 different sprites
     * for each of the 4 cells (32 sprites total).
     *
     * These arrays correspond to the direction4 ordering. */
    const int W = tileset_tile_width(tileset());
    const int H = tileset_tile_height(tileset());
    const int iso_offsets[4][2] = {
        {W / 4, 0}, {W / 4, H / 2}, {W / 2, H / 4}, {0, H / 4}};
    const int noniso_offsets[4][2] = {
        {0, 0}, {W / 2, H / 2}, {W / 2, 0}, {0, H / 2}};

    // put corner cells
    for (int i = 0; i < NUM_CORNER_DIRS; i++) {
      const int count = info.matches_with.size();
      enum direction8 dir = dir_ccw(DIR4_TO_DIR8[i]);
      int x = (tileset_is_isometric(tileset()) ? iso_offsets[i][0]
                                               : noniso_offsets[i][0]);
      int y = (tileset_is_isometric(tileset()) ? iso_offsets[i][1]
                                               : noniso_offsets[i][1]);
      int m[3] = {MATCH(dir_ccw(dir)), MATCH(dir), MATCH(dir_cw(dir))};

      int array_index = 0;
      switch (info.style) {
      case MATCH_NONE:
        // We have no need for matching, just plug the piece in place.
        break;
      case MATCH_SAME:
        fc_assert_ret(info.matches_with.size() == 2);
        array_index = array_index * 2 + (m[2] != info.group->number);
        array_index = array_index * 2 + (m[1] != info.group->number);
        array_index = array_index * 2 + (m[0] != info.group->number);
        break;
      case MATCH_PAIR: {
        fc_assert_ret(info.matches_with.size() == 2);
        const auto that = info.matches_with.back()->number;
        array_index = array_index * 2 + (m[2] == that);
        array_index = array_index * 2 + (m[1] == that);
        array_index = array_index * 2 + (m[0] == that);
      } break;
      case MATCH_FULL:
      default: {
        int n[3];
        for (int j = 0; j < 3; j++) {
          for (int k = 0; k < count; k++) {
            n[j] = k; // default to last entry
            if (m[j] == info.matches_with[k]->number) {
              break;
            }
          }
        }
        array_index = array_index * count + n[2];
        array_index = array_index * count + n[1];
        array_index = array_index * count + n[0];
      } break;
      };
      array_index = array_index * NUM_CORNER_DIRS + i;

      const auto sprite = info.sprites[array_index];
      if (sprite) {
        sprs.emplace_back(tileset(), sprite, true, x, y);
      }
    }
    break;
  }
  };
#undef MATCH
}

/**
 * Helper function for fill_sprite_array.
 * Fill in the sprite array for blended terrain.
 *
 * This function assumes that m_blending contains a value for pterrain.
 */
void layer_terrain::fill_blending_sprite_array(
    std::vector<drawn_sprite> &sprs, const tile *ptile,
    const terrain *pterrain, terrain **tterrain_near) const
{
  // By how much the blending sprites need to be offset
  const int W = tileset_tile_width(tileset());
  const int H = tileset_tile_height(tileset());
  const int offsets[4][2] = {{W / 2, 0}, {0, H / 2}, {W / 2, H / 2}, {0, 0}};

  // Not drawn
  if (m_terrain_info.count(terrain_index(pterrain)) == 0) {
    return;
  }

  // Not blended
  auto info = m_terrain_info.at(terrain_index(pterrain));
  if (!info.blend) {
    return;
  }

  for (int dir = 0; dir < 4; dir++) {
    struct tile *neighbor = mapstep(&(wld.map), ptile, DIR4_TO_DIR8[dir]);
    struct terrain *other;

    // No other tile, don't "blend" at the edge of the map
    if (!neighbor) {
      continue;
    }

    // Other tile is unknown
    if (client_tile_get_known(neighbor) == TILE_UNKNOWN) {
      continue;
    }

    // No blending between identical terrains
    if (pterrain == (other = tterrain_near[DIR4_TO_DIR8[dir]])) {
      continue;
    }

    // Other terrain is not drawn
    if (m_terrain_info.count(terrain_index(other)) == 0) {
      continue;
    }

    // Other terrain is not blended
    auto other_info = m_terrain_info.at(terrain_index(other));
    if (!other_info.blend) {
      continue;
    }

    // Pick the blending sprite and add it
    if (Q_LIKELY(other_info.blend_sprites.at(dir) != nullptr)) {
      sprs.emplace_back(tileset(), other_info.blend_sprites.at(dir), true,
                        offsets[dir][0], offsets[dir][1]);
    }
  }
}

} // namespace freeciv
