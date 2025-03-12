// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_grid.h"

#include "citydlg_g.h"
#include "client_main.h"
#include "climap.h"
#include "control.h"
#include "movement.h"
#include "sprite_g.h"
#include "tilespec.h"
#include "views/view_map_common.h" // mapdeco

#include <QPixmap>

namespace freeciv {

layer_grid::layer_grid(struct tileset *ts, mapview_layer layer)
    : freeciv::layer(ts, layer), m_unavailable(nullptr), m_nonnative(nullptr)
{
}

void layer_grid::load_sprites()
{
  // This must correspond to enum edge_type.
  const auto edge_name =
      std::array{QStringLiteral("ns"), QStringLiteral("we"),
                 QStringLiteral("ud"), QStringLiteral("lr")};

  m_unavailable = load_sprite({"grid.unavailable"}, true);
  m_nonnative = load_sprite({"grid.nonnative"}, false);

  for (int i = 0; i < EDGE_COUNT; i++) {
    int be;

    if (i == EDGE_UD && tileset_hex_width(tileset()) == 0) {
      continue;
    } else if (i == EDGE_LR && tileset_hex_height(tileset()) == 0) {
      continue;
    }

    auto name = QStringLiteral("grid.main.%1").arg(edge_name[i]);
    m_main[i] = load_sprite({name}, true);

    name = QStringLiteral("grid.city.%1").arg(edge_name[i]);
    m_city[i] = load_sprite({name}, true);

    name = QStringLiteral("grid.worked.%1").arg(edge_name[i]);
    m_worked[i] = load_sprite({name}, true);

    name = QStringLiteral("grid.selected.%1").arg(edge_name[i]);
    m_selected[i] = load_sprite({name}, true);

    for (be = 0; be < 2; be++) {
      name = QStringLiteral("grid.borders.%1").arg(edge_name[i][be]);
      m_basic_borders[i][be] = load_sprite({name}, true);
    }
  }
}

/**
 * Initializes border data for a player.
 */
void layer_grid::initialize_player(const player *player)
{
  // Get the color. In pregame, players have no color so we just use red.
  const auto c = player_has_color(tileset(), player)
                     ? get_player_color(tileset(), player)
                     : Qt::red;

  QPixmap color(tileset_tile_width(tileset()),
                tileset_tile_height(tileset()));
  color.fill(c);

  for (int i = 0; i < EDGE_COUNT; i++) {
    for (int j = 0; j < 2; j++) {
      if (m_basic_borders[i][j]) {
        m_borders[player_index(player)][i][j].reset(crop_sprite(
            &color, 0, 0, tileset_tile_width(tileset()),
            tileset_tile_height(tileset()), m_basic_borders[i][j], 0, 0));
      }
    }
  }
}

/**
 * Frees border data for a player.
 */
void layer_grid::free_player(int player_id)
{
  for (auto &edge : m_borders.at(player_id)) {
    for (auto &pix : edge) {
      pix = nullptr;
    }
  }
}

/**
 * Fill in the grid sprites for the given tile, city, and unit.
 */
std::vector<drawn_sprite>
layer_grid::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                              const tile_corner *pcorner,
                              const unit *punit) const
{
  // LAYER_GRID1 is used for isometric tilesets only
  if (tileset_is_isometric(tileset()) && type() != LAYER_GRID1) {
    return {};
  }

  // LAYER_GRID2 is used for non-isometric tilesets only
  if (!tileset_is_isometric(tileset()) && type() != LAYER_GRID2) {
    return {};
  }

  const auto citymode = is_any_city_dialog_open();

  std::vector<drawn_sprite> sprs;

  if (pedge) {
    bool known[NUM_EDGE_TILES], city[NUM_EDGE_TILES];
    bool unit[NUM_EDGE_TILES], worked[NUM_EDGE_TILES];

    for (int i = 0; i < NUM_EDGE_TILES; i++) {
      int dummy_x, dummy_y;
      const struct tile *tile = pedge->tile[i];
      struct player *powner = tile ? tile_owner(tile) : nullptr;

      known[i] = tile && client_tile_get_known(tile) != TILE_UNKNOWN;
      unit[i] = false;
      if (tile && !citymode) {
        for (const auto pfocus_unit : get_units_in_focus()) {
          if (unit_drawn_with_city_outline(pfocus_unit, false)) {
            struct tile *utile = unit_tile(pfocus_unit);
            int radius = game.info.init_city_radius_sq
                         + get_target_bonus_effects(
                             nullptr, unit_owner(pfocus_unit), nullptr,
                             nullptr, nullptr, utile, nullptr, nullptr,
                             nullptr, nullptr, nullptr, EFT_CITY_RADIUS_SQ);

            if (city_tile_to_city_map(&dummy_x, &dummy_y, radius, utile,
                                      tile)) {
              unit[i] = true;
              break;
            }
          }
        }
      }
      worked[i] = false;

      city[i] = (tile
                 && (powner == nullptr || client_player() == nullptr
                     || powner == client.conn.playing)
                 && player_in_city_map(client.conn.playing, tile));
      if (city[i]) {
        if (citymode) {
          /* In citymode, we only draw worked tiles for this city - other
           * tiles may be marked as unavailable. */
          worked[i] = (tile_worked(tile) == citymode);
        } else {
          worked[i] = (nullptr != tile_worked(tile));
        }
      }
      // Draw city grid for main citymap
      if (tile && citymode
          && city_base_to_city_map(&dummy_x, &dummy_y, citymode, tile)) {
        sprs.emplace_back(tileset(), m_selected[pedge->type]);
      }
    }
    if (mapdeco_is_highlight_set(pedge->tile[0])
        || mapdeco_is_highlight_set(pedge->tile[1])) {
      sprs.emplace_back(tileset(), m_selected[pedge->type]);
    } else {
      if (gui_options->draw_map_grid) {
        if (worked[0] || worked[1]) {
          sprs.emplace_back(tileset(), m_worked[pedge->type]);
        } else if (city[0] || city[1]) {
          sprs.emplace_back(tileset(), m_city[pedge->type]);
        } else if (known[0] || known[1]) {
          sprs.emplace_back(tileset(), m_main[pedge->type]);
        }
      }
      if (gui_options->draw_city_outlines) {
        if (XOR(city[0], city[1])) {
          sprs.emplace_back(tileset(), m_city[pedge->type]);
        }
        if (XOR(unit[0], unit[1])) {
          sprs.emplace_back(tileset(), m_worked[pedge->type]);
        }
      }
    }

    if (gui_options->draw_borders && BORDERS_DISABLED != game.info.borders
        && known[0] && known[1]) {
      struct player *owner0 = tile_owner(pedge->tile[0]);
      struct player *owner1 = tile_owner(pedge->tile[1]);

      if (owner0 != owner1) {
        if (owner0) {
          int plrid = player_index(owner0);
          sprs.emplace_back(tileset(),
                            m_borders[plrid][pedge->type][0].get());
        }
        if (owner1) {
          int plrid = player_index(owner1);
          sprs.emplace_back(tileset(),
                            m_borders[plrid][pedge->type][1].get());
        }
      }
    }
  } else if (nullptr != ptile
             && TILE_UNKNOWN != client_tile_get_known(ptile)) {
    int cx, cy;

    if (citymode
        // test to ensure valid coordinates?
        && city_base_to_city_map(&cx, &cy, citymode, ptile)
        && !client_city_can_work_tile(citymode, ptile)) {
      sprs.emplace_back(tileset(), m_unavailable);
    }

    if (gui_options->draw_native && citymode == nullptr) {
      bool native = true;
      for (const auto pfocus : get_units_in_focus()) {
        if (!is_native_tile(unit_type_get(pfocus), ptile)) {
          native = false;
          break;
        }
      }

      if (!native) {
        if (m_nonnative != nullptr) {
          sprs.emplace_back(tileset(), m_nonnative);
        } else {
          sprs.emplace_back(tileset(), m_unavailable);
        }
      }
    }
  }

  return sprs;
}

} // namespace freeciv
