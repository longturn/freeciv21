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
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);

  // Should we draw anything in the first place?
  if (!do_draw_unit(ptile, punit)) {
    return {};
  }

  // Only draw the focused unit on LAYER_FOCUS_UNIT
  if (type() != LAYER_FOCUS_UNIT && unit_is_in_focus(punit)) {
    return {};
  }

  std::vector<drawn_sprite> sprs;
  fill_unit_sprite_array(tileset(), sprs, ptile, punit);
  return sprs;
}

} // namespace freeciv
