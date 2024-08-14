// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_city.h"

#include "city.h"
#include "citybar.h"
#include "rgbcolor.h"
#include "style.h"
#include "tilespec.h"
#include "utils/colorizer.h"

namespace freeciv {

layer_city::layer_city(struct tileset *ts, const QPoint &city_offset,
                       const QPoint &city_flag_offset,
                       const QPoint &occupied_offset)
    : freeciv::layer(ts, LAYER_CITY1), m_city_offset(city_offset),
      m_city_flag_offset(city_flag_offset),
      m_occupied_offset(occupied_offset)
{
}

/**
 * Loads the disorder and happy sprites.
 */
void layer_city::load_sprites()
{
  m_disorder = load_sprite({QStringLiteral("city.disorder")}, true);
  m_happy = load_sprite({QStringLiteral("city.happy")}, false);
}

/**
 * Loads a set of size-dependent city sprites, with tags of the form
 * `graphic_tag_size`.
 */
layer_city::styles layer_city::load_city_size_sprites(const QString &tag,
                                                      const citystyle &style)
{
  auto gfx_in_use = style.graphic;
  auto sprites = layer_city::styles();

  for (int size = 0; size < MAX_CITY_SIZE; size++) {
    const auto buffer = QStringLiteral("%1_%2_%3")
                            .arg(gfx_in_use, tag, QString::number(size));
    if (const auto sprite = load_sprite({buffer}, true, false)) {
      sprites.push_back(std::make_unique<freeciv::colorizer>(
          *sprite, tileset_replaced_hue(tileset())));
    } else if (size == 0) {
      if (gfx_in_use == style.graphic) {
        // Try again with graphic_alt.
        size--;
        gfx_in_use = style.graphic_alt;
      } else {
        // Don't load any others if the 0 element isn't there.
        break;
      }
    } else {
      // Stop loading as soon as we don't find the next one.
      break;
    }
  }
  if (sprites.empty() && strlen(style.graphic_alt)
      && style.graphic_alt != QStringLiteral("-")) {
    tileset_error(tileset(), QtInfoMsg,
                  "Could not find any sprite matching %s_%s_0 or %s_%s_0",
                  style.graphic, qUtf8Printable(tag), style.graphic_alt,
                  qUtf8Printable(tag));
  } else if (sprites.empty()) {
    // No graphic_alt
    tileset_error(tileset(), QtInfoMsg,
                  "Could not find any sprite matching %s_%s_0",
                  style.graphic, qUtf8Printable(tag));
  } else {
    tileset_error(tileset(), QtInfoMsg, "Loaded %d %s_%s* sprites",
                  sprites.size(), gfx_in_use, qUtf8Printable(tag));
  }

  return sprites;
}

/**
 * Loads all sprites for a city style.
 */
void layer_city::initialize_city_style(const citystyle &style, int index)
{
  fc_assert_ret(index == m_tile.size());

  m_tile.push_back(load_city_size_sprites(QStringLiteral("city"), style));
  m_single_wall.push_back(
      load_city_size_sprites(QStringLiteral("wall"), style));
  m_occupied.push_back(
      load_city_size_sprites(QStringLiteral("occupied"), style));

  for (int i = 0; i < m_walls.size(); i++) {
    m_walls[i].push_back(
        load_city_size_sprites(QStringLiteral("bldg_%1").arg(i), style));
  }

  if (m_tile.back().empty()) {
    tileset_error(tileset(), LOG_FATAL,
                  _("City style \"%s\": no city graphics."),
                  city_style_rule_name(index));
  }
  if (m_occupied.back().empty()) {
    tileset_error(tileset(), LOG_FATAL,
                  _("City style \"%s\": no occupied graphics."),
                  city_style_rule_name(index));
  }
}

namespace /* anonymous */ {
/**
 * Return the sprite in the city_sprite listing that corresponds to this city
 * - based on city style and size.
 *
 * See also load_city_sprite.
 */
static const QPixmap *
get_city_sprite(const layer_city::city_sprite &city_sprite,
                const struct city *pcity)
{
  // get style and match the best tile based on city size
  int style = style_of_city(pcity);
  int img_index;

  fc_assert_ret_val(style < city_sprite.size(), nullptr);

  const auto num_thresholds = city_sprite[style].size();
  const auto &thresholds = city_sprite[style];

  if (num_thresholds == 0) {
    return nullptr;
  }

  // Get the sprite with the index defined by the effects.
  img_index = pcity->client.city_image;
  if (img_index == -100) {
    /* The server doesn't know the right value as this is from and old
     * savegame. Guess here based on *client* side information as was done in
     * versions where information was not saved to savegame - this should
     * give us right answer of what city looked like by the time it was put
     * under FoW. */
    img_index = get_city_bonus(pcity, EFT_CITY_IMAGE);
  }
  img_index = CLIP(0, img_index, num_thresholds - 1);

  const auto owner =
      pcity->owner; // city_owner asserts when there is no owner
  auto color = QColor();
  if (owner && owner->rgb) {
    color.setRgb(owner->rgb->r, owner->rgb->g, owner->rgb->b);
  }
  return thresholds[img_index]->pixmap(color);
}
} // anonymous namespace

/**
 * Fill in the given sprite array with any needed city sprites. The city size
 * is not included.
 */
std::vector<drawn_sprite>
layer_city::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                              const tile_corner *pcorner,
                              const unit *punit) const
{
  if (!gui_options->draw_cities) {
    return {};
  }

  auto pcity = tile_city(ptile);
  if (!pcity) {
    return {};
  }

  std::vector<drawn_sprite> sprs;

  // The flag, if needed
  if (!citybar_painter::current()->has_flag()) {
    sprs.emplace_back(
        tileset(), get_city_flag_sprite(tileset(), pcity), true,
        tileset_full_tile_offset(tileset()) + m_city_flag_offset);
  }

  auto more = fill_sprite_array_no_flag(pcity);
  sprs.insert(sprs.end(), more.begin(), more.end());

  return sprs;
}

/**
 * Fill in the given sprite array with any city sprites. Doesn't account for
 * options->draw_cities. The flag and city size are not included.
 */
std::vector<drawn_sprite>
layer_city::fill_sprite_array_no_flag(const city *pcity) const
{
  auto walls = pcity->client.walls;
  if (walls >= NUM_WALL_TYPES) { // Failsafe
    walls = 1;
  }

  std::vector<drawn_sprite> sprs;

  /* For iso-view the city.wall graphics include the full city, whereas for
   * non-iso view they are an overlay on top of the base city graphic. This
   * leads to the mess below. */

  // Non-isometric: city & Isometric: city without walls
  if (!tileset_is_isometric(tileset()) || walls <= 0) {
    sprs.emplace_back(tileset(), get_city_sprite(m_tile, pcity), true,
                      tileset_full_tile_offset(tileset()) + m_city_offset);
  }
  // Isometric: walls and city together
  if (tileset_is_isometric(tileset()) && walls > 0) {
    auto sprite = get_city_sprite(m_walls[walls - 1], pcity);
    if (!sprite) {
      sprite = get_city_sprite(m_single_wall, pcity);
    }

    if (sprite) {
      sprs.emplace_back(tileset(), sprite, true,
                        tileset_full_tile_offset(tileset()) + m_city_offset);
    }
  }

  // Occupied flag
  if (!citybar_painter::current()->has_units() && pcity->client.occupied) {
    sprs.emplace_back(tileset(), get_city_sprite(m_occupied, pcity), true,
                      tileset_full_tile_offset(tileset())
                          + m_occupied_offset);
  }

  // Non-isometric: add walls on top
  if (!tileset_is_isometric(tileset()) && walls > 0) {
    auto sprite = get_city_sprite(m_walls[walls - 1], pcity);
    if (!sprite) {
      sprite = get_city_sprite(m_single_wall, pcity);
    }

    if (sprite) {
      sprs.emplace_back(tileset(), sprite, true,
                        tileset_full_tile_offset(tileset()));
    }
  }

  // Happiness
  if (pcity->client.unhappy) {
    sprs.emplace_back(tileset(), m_disorder, true,
                      tileset_full_tile_offset(tileset()));
  } else if (m_happy && pcity->client.happy) {
    sprs.emplace_back(tileset(), m_happy, true,
                      tileset_full_tile_offset(tileset()));
  }

  return sprs;
}

/**
 * All sprites depend on the ruleset-defined city styles and need to be
 * reset.
 */
void layer_city::reset_ruleset()
{
  m_tile.clear();

  for (auto &wall : m_walls) {
    wall.clear();
  }
  m_single_wall.clear();
  m_occupied.clear();
}

} // namespace freeciv
