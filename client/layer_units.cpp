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

std::vector<drawn_sprite> layer_units::fill_sprite_array(
    const tile *ptile, const tile_edge *pedge, const tile_corner *pcorner,
    const unit *punit, const city *pcity, const unit_type *putype) const
{
  std::vector<drawn_sprite> sprs;

  /* Unit drawing is disabled when the view options are turned off,
   * but only where we're drawing on the mapview. */
  bool do_draw_unit =
      (punit
       && (gui_options.draw_units || !ptile
           || (gui_options.draw_focus_unit && unit_is_in_focus(punit))));

  if (do_draw_unit && XOR(type() == LAYER_UNIT, unit_is_in_focus(punit))) {
    const bool stacked = ptile && (unit_list_size(ptile->units) > 1);
    const bool backdrop = !pcity;
    fill_unit_sprite_array(tileset(), sprs, ptile, punit, stacked, backdrop);
  } else if (putype != nullptr && type() == LAYER_UNIT) {
    // Only the sprite for the unit type.
    fill_unit_type_sprite_array(tileset(), sprs, putype,
                                direction8_invalid());
  }

  return sprs;
}

} // namespace freeciv
