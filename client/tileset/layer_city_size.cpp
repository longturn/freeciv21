// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_city_size.h"

#include "citybar.h"
#include "tilespec.h"

// common
#include "city.h"
#include "fc_types.h"
#include "tile.h"

namespace freeciv {

layer_city_size::layer_city_size(struct tileset *ts, const QPoint &offset)
    : freeciv::layer(ts, LAYER_CITY2), m_offset(offset), m_units(), m_tens(),
      m_hundreds()
{
}

void layer_city_size::load_sprites()
{
  // Digit sprites. We have a cascade of fallbacks.
  QStringList patterns = {QStringLiteral("city.size_%1")};
  assign_digit_sprites(tileset(), m_units.data(), m_tens.data(),
                       m_hundreds.data(), patterns);
}

/**
 * Fill in the given sprite array with any needed city size sprites.
 */
std::vector<drawn_sprite>
layer_city_size::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                                   const tile_corner *pcorner,
                                   const unit *punit) const
{
  if (!gui_options->draw_cities || citybar_painter::current()->has_size()) {
    return {};
  }

  auto pcity = tile_city(ptile);
  if (!pcity) {
    return {};
  }

  // Now draw
  std::vector<drawn_sprite> sprs;
  auto size = city_size_get(pcity);

  // Units
  auto sprite = m_units[size % NUM_TILES_DIGITS];
  sprs.emplace_back(tileset(), sprite, false, m_offset);

  // Tens
  size /= NUM_TILES_DIGITS;
  if (size > 0 && (sprite = m_tens[size % NUM_TILES_DIGITS])) {
    sprs.emplace_back(tileset(), sprite, false, m_offset);
  }

  // Hundreds (optional)
  size /= NUM_TILES_DIGITS;
  if (size > 0 && (sprite = m_hundreds[size % NUM_TILES_DIGITS])) {
    sprs.emplace_back(tileset(), sprite, false, m_offset);
    // Divide for the warning: warn for thousands if we had a hundreds sprite
    size /= NUM_TILES_DIGITS;
  }

  // Warn if the city is too large (only once by tileset).
  if (size > 0 && !m_warned) {
    tileset_error(
        tileset(), QtWarningMsg,
        _("Tileset \"%s\" doesn't support big city sizes, such as %d. "
          "Size not displayed as expected."),
        tileset_name_get(tileset()), size);
    m_warned = true;
  }

  return sprs;
}

} // namespace freeciv
