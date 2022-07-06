/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "layer_units.h"

#include "control.h"
#include "options.h"
#include "tilespec.h"

/**
 * \class freeciv::layer_units
 * \brief Draws units on the map.
 */

namespace freeciv {

/**
 * Constructor
 */
layer_units::layer_units(struct tileset *ts, mapview_layer layer)
    : freeciv::layer(ts, layer)
{
}

std::vector<drawn_sprite>
layer_units::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                               const tile_corner *pcorner,
                               const unit *punit) const
{
  // We need to have something to draw
  if (!punit) {
    return {};
  }

  std::vector<drawn_sprite> sprs;

  /* Unit drawing is disabled when the view options are turned off,
   * but only where we're drawing on the mapview. */
  bool do_draw_unit =
      gui_options.draw_units
      || (gui_options.draw_focus_unit && unit_is_in_focus(punit));

  if (do_draw_unit && XOR(type() == LAYER_UNIT, unit_is_in_focus(punit))) {
    const bool backdrop = !tile_city(ptile);
    fill_unit_sprite_array(tileset(), sprs, ptile, punit, backdrop);
  }

  return sprs;
}

} // namespace freeciv
