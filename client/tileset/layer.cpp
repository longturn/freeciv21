/*__            ___                 ***************************************
/   \          /   \         Copyright (c) 2021-2023 Freeciv21 contributors.
\_   \        /  __/                         This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include "control.h"
#include "tilespec.h"

#include "layer.h"
#include "log.h"

namespace freeciv {

std::vector<drawn_sprite>
layer::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                         const tile_corner *pcorner, const unit *punit) const
{
  return ::fill_sprite_array(m_ts, m_layer, ptile, pedge, pcorner, punit);
}

/**
 * @brief Whether a unit should be drawn.
 * @param ptile The tile where to draw (can be null)
 * @param punit The unit that should be drawn (can be null)
 */
bool layer::do_draw_unit(const tile *ptile, const unit *punit) const
{
  if (!punit) {
    // Can't draw a non-existing unit.
    return false;
  }

  // There is a unit.

  if (!ptile) {
    // No tile (so not on the map) => always draw.
    return true;
  }

  // Handle options to turn off drawing units.
  if (gui_options->draw_units) {
    return true;
  } else if (gui_options->draw_focus_unit && unit_is_in_focus(punit)) {
    return true;
  } else {
    return false;
  }
}

/**
 * @brief Whether a solid background should be drawn on a tile instead of its
 *        terrain.
 *
 * Query this function to know whether you should refrain from drawing
 * "terrain-like" sprites (terrain, darkness, water follow this setting).
 *
 * @returns `true` when the solid background is enabled and a unit or city is
 *          drawn on the tile.
 *
 * @param ptile The tile where to draw (can be null)
 * @param punit The unit that could be drawn (can be null)
 * @param pcity The city that could be drawn (can be null)
 */
bool layer::solid_background(const tile *ptile, const unit *punit,
                             const city *pcity) const
{
  if (!gui_options->solid_color_behind_units) {
    // Solid background turned off (the default).
    return false;
  }

  return do_draw_unit(ptile, punit) || (gui_options->draw_cities && pcity);
}

} // namespace freeciv
