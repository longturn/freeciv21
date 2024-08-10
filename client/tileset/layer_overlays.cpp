// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_overlays.h"

#include "citydlg_g.h"
#include "climap.h"
#include "log.h"
#include "sprite_g.h"
#include "tilespec.h"
#include "views/view_map_common.h"

namespace freeciv {

layer_overlays::layer_overlays(struct tileset *ts)
    : freeciv::layer(ts, LAYER_OVERLAYS)
{
}

void layer_overlays::load_sprites()
{
  // Tile output counters
  for (int i = 0;; ++i) {
    auto buffer = QStringLiteral("city.t_food_%1").arg(i);
    if (auto sprite = load_sprite({buffer})) {
      m_food.push_back(sprite);
    } else {
      break;
    }
  }
  for (int i = 0;; ++i) {
    auto buffer = QStringLiteral("city.t_shields_%1").arg(i);
    if (auto sprite = load_sprite({buffer})) {
      m_prod.push_back(sprite);
    } else {
      break;
    }
  }
  for (int i = 0;; ++i) {
    auto buffer = QStringLiteral("city.t_trade_%1").arg(i);
    if (auto sprite = load_sprite({buffer})) {
      m_trade.push_back(sprite);
    } else {
      break;
    }
  }

  // Overlay colors
  std::vector<QPixmap *> colors;
  for (int i = 0;; ++i) {
    auto buffer = QStringLiteral("colors.overlay_%1").arg(i);
    auto sprite = load_sprite({buffer});
    if (!sprite) {
      break;
    }
    colors.push_back(sprite);
  }
  if (colors.empty()) {
    tileset_error(tileset(), LOG_FATAL,
                  _("Missing overlay-color sprite colors.overlay_0."));
  }

  auto worked_base = load_sprite({"mask.worked_tile"}, true);
  auto unworked_base = load_sprite({"mask.unworked_tile"}, true);

  // Chop up and build the overlay graphics.
  auto W = tileset_tile_width(tileset()), H = tileset_tile_height(tileset());
  for (const auto &color : colors) {
    auto color_mask = std::unique_ptr<QPixmap>(
        crop_sprite(color, 0, 0, W, H, get_mask_sprite(tileset()), 0, 0));
    m_worked.emplace_back(
        crop_sprite(color_mask.get(), 0, 0, W, H, worked_base, 0, 0));
    m_unworked.emplace_back(
        crop_sprite(color_mask.get(), 0, 0, W, H, unworked_base, 0, 0));
  }
}

/**
 * Fill in the given sprite array with any needed overlays sprites.
 */
std::vector<drawn_sprite>
layer_overlays::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                                  const tile_corner *pcorner,
                                  const unit *punit) const
{
  std::vector<drawn_sprite> sprs;

  if (!ptile || client_tile_get_known(ptile) == TILE_UNKNOWN) {
    return {};
  }

  unit *psettler = nullptr;
  const auto citymode = is_any_city_dialog_open();
  auto pcity =
      citymode ? citymode : find_city_or_settler_near_tile(ptile, &psettler);

  // The code below does not work if pcity is invisible. Make sure it is not.
  fc_assert_action(!pcity || pcity->tile, pcity = nullptr);

  int city_x, city_y;
  if (pcity && city_base_to_city_map(&city_x, &city_y, pcity, ptile)) {
    const auto pwork = tile_worked(ptile);
    if (!citymode && pcity->client.colored) {
      // Add citymap overlay for a city.
      int idx = pcity->client.color_index % m_worked.size();
      if (pwork && pwork == pcity) {
        sprs.emplace_back(tileset(), m_worked[idx].get());
      } else if (city_can_work_tile(pcity, ptile)) {
        sprs.emplace_back(tileset(), m_unworked[idx].get());
      }
    } else if (pwork && pwork == pcity
               && (citymode || gui_options->draw_city_output)) {
      // Add on the tile output sprites.
      const int ox = tileset_is_isometric(tileset())
                         ? tileset_tile_width(tileset()) / 3
                         : 0;
      const int oy = tileset_is_isometric(tileset())
                         ? -tileset_tile_height(tileset()) / 3
                         : 0;

      if (!m_food.empty()) {
        int food = city_tile_output_now(pcity, ptile, O_FOOD);
        food = CLIP(0, food / game.info.granularity, m_food.size() - 1);
        sprs.emplace_back(tileset(), m_food[food], true, ox, oy);
      }
      if (!m_prod.empty()) {
        int shields = city_tile_output_now(pcity, ptile, O_SHIELD);
        shields =
            CLIP(0, shields / game.info.granularity, m_prod.size() - 1);
        sprs.emplace_back(tileset(), m_prod[shields], true, ox, oy);
      }
      if (!m_trade.empty()) {
        int trade = city_tile_output_now(pcity, ptile, O_TRADE);
        trade = CLIP(0, trade / game.info.granularity, m_trade.size() - 1);
        sprs.emplace_back(tileset(), m_trade[trade], true, ox, oy);
      }
    }
  } else if (psettler && psettler->client.colored) {
    // Add citymap overlay for a unit.
    auto idx = psettler->client.color_index % m_unworked.size();
    sprs.emplace_back(tileset(), m_unworked[idx].get());
  }

  // Crosshair sprite
  if (mapdeco_is_crosshair_set(ptile)) {
    sprs.emplace_back(tileset(), get_attention_crosshair_sprite(tileset()));
  }

  return sprs;
}

} // namespace freeciv
