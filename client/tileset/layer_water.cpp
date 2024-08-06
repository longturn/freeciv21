// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_water.h"

#include "colors_common.h"
#include "control.h"
#include "extras.h"
#include "fc_types.h"
#include "tilespec.h"

namespace freeciv {

layer_water::layer_water(struct tileset *ts)
    : freeciv::layer(ts, LAYER_WATER)
{
}

/**
 * Collects all extras to be drawn (the ones with ESTYLE_CARDINALS or
 * ESTYLE_RIVER).
 */
void layer_water::initialize_extra(const extra_type *extra,
                                   const QString &tag, extrastyle_id style)
{
  if (style == ESTYLE_CARDINALS) {
    extra_data data;
    data.extra = extra;

    terrain_type_iterate(terrain)
    {
      extra_data::terrain_data sprites;

      // We use direction-specific irrigation and farmland graphics, if they
      // are available. If not, we just fall back to the basic irrigation
      // graphics.
      QStringList alt_tags = make_tag_terrain_list(tag, "", terrain);
      for (int i = 0; i < tileset_num_index_cardinals(tileset()); i++) {
        QStringList tags = make_tag_terrain_list(
            tag, cardinal_index_str(tileset(), i), terrain);
        sprites[i] = load_sprite(tileset(), tags + alt_tags, true);
      }

      data.sprites.push_back(sprites);
    }
    terrain_type_iterate_end;

    m_cardinals.push_back(data);
  } else if (style == ESTYLE_RIVER) {
    extra_data outlet, river;
    river.extra = extra;
    outlet.extra = extra;

    terrain_type_iterate(terrain)
    {
      extra_data::terrain_data outlet_sprites;
      extra_data::terrain_data river_sprites;

      for (int i = 0; i < tileset_num_index_cardinals(tileset()); i++) {
        auto suffix =
            QStringLiteral("_s_%1").arg(cardinal_index_str(tileset(), i));
        river_sprites[i] = load_sprite(
            tileset(), make_tag_terrain_list(tag, suffix, terrain), true);
      }

      for (int i = 0; i < tileset_num_cardinal_dirs(tileset()); i++) {
        auto suffix = QStringLiteral("_outlet_%1")
                          .arg(dir_get_tileset_name(
                              tileset_cardinal_dirs(tileset())[i]));
        outlet_sprites[i] = load_sprite(
            tileset(), make_tag_terrain_list(tag, suffix, terrain), true);
      }

      outlet.sprites.push_back(outlet_sprites);
      river.sprites.push_back(river_sprites);
    }
    terrain_type_iterate_end;

    m_outlets.emplace_back(outlet);
    m_rivers.emplace_back(river);
  }
}

namespace /* anonymous */ {
/**
 * Return the index of the sprite to be used for irrigation or farmland in
 * this tile.
 *
 * We assume that the current tile has farmland or irrigation.  We then
 * choose a sprite (index) based upon which cardinally adjacent tiles have
 * either farmland or irrigation (the two are considered interchangable for
 * this).
 */
static int get_irrigation_index(const struct tileset *t,
                                const extra_type *pextra,
                                bv_extras *textras_near)
{
  int tileno = 0, i;

  for (i = 0; i < tileset_num_cardinal_dirs(t); i++) {
    enum direction8 dir = tileset_cardinal_dirs(t)[i];

    if (BV_ISSET(textras_near[dir], extra_index(pextra))) {
      tileno |= 1 << i;
    }
  }

  return tileno;
}
} // anonymous namespace

/**
 * Fill in the farmland/irrigation sprite for the tile.
 */
void layer_water::fill_irrigation_sprite_array(
    const struct tileset *t, std::vector<drawn_sprite> &sprs,
    bv_extras textras, bv_extras *textras_near, const terrain *pterrain,
    const city *pcity) const
{
  // We don't draw the irrigation if there's a city (it just gets overdrawn
  // anyway, and ends up looking bad).
  if (!(pcity && gui_options->draw_cities)) {
    for (auto cdata : m_cardinals) {
      if (is_extra_drawing_enabled(cdata.extra)) {
        int eidx = extra_index(cdata.extra);

        if (BV_ISSET(textras, eidx)) {
          bool hidden = false;

          extra_type_list_iterate(cdata.extra->hiders, phider)
          {
            if (BV_ISSET(textras, extra_index(phider))) {
              hidden = true;
              break;
            }
          }
          extra_type_list_iterate_end;

          if (!hidden) {
            int idx = get_irrigation_index(t, cdata.extra, textras_near);

            sprs.emplace_back(t,
                              cdata.sprites[terrain_index(pterrain)][idx]);
          }
        }
      }
    }
  }
}

std::vector<drawn_sprite>
layer_water::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                               const tile_corner *pcorner,
                               const unit *punit) const
{
  if (!ptile) {
    return {};
  }

  const auto terrain = tile_terrain(ptile);
  if (!terrain) {
    return {};
  }

  const auto pcity = tile_city(ptile);
  const auto extras = *tile_extras(ptile);

  struct terrain *terrain_near[8] = {nullptr}; // dummy
  bv_extras extras_near[8];
  build_tile_data(ptile, terrain, terrain_near, extras_near);

  std::vector<drawn_sprite> sprs;

  if (!solid_background(ptile, punit, pcity)
      && terrain_type_terrain_class(terrain) == TC_OCEAN) {
    for (int dir = 0; dir < tileset_num_cardinal_dirs(tileset()); dir++) {
      int didx = tileset_cardinal_dirs(tileset())[dir];

      for (auto outlet : m_outlets) {
        int idx = extra_index(outlet.extra);

        if (BV_ISSET(extras_near[didx], idx)) {
          sprs.emplace_back(tileset(),
                            outlet.sprites[terrain_index(terrain)][dir]);
        }
      }
    }
  }

  fill_irrigation_sprite_array(tileset(), sprs, extras, extras_near, terrain,
                               pcity);

  if (!solid_background(ptile, punit, pcity)) {
    for (const auto &river : m_rivers) {
      int idx = extra_index(river.extra);

      if (BV_ISSET(extras, idx)) {
        // Draw rivers on top of irrigation.
        int tileno = 0;
        for (int i = 0; i < tileset_num_cardinal_dirs(tileset()); i++) {
          enum direction8 cdir = tileset_cardinal_dirs(tileset())[i];

          if (terrain_near[cdir] == nullptr
              || terrain_type_terrain_class(terrain_near[cdir]) == TC_OCEAN
              || BV_ISSET(extras_near[cdir], idx)) {
            tileno |= 1 << i;
          }
        }

        sprs.emplace_back(tileset(),
                          river.sprites[terrain_index(terrain)][tileno]);
      }
    }
  }

  return sprs;
}

void layer_water::reset_ruleset()
{
  m_cardinals.clear();
  m_outlets.clear();
  m_rivers.clear();
}

} // namespace freeciv
