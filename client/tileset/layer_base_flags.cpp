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

#include "layer_base_flags.h"

#include "extras.h"
#include "game.h" // For extra_type_iterate
#include "nation.h"
#include "tilespec.h"

namespace freeciv {

/**
 * @class layer_base_flags
 *
 * Map layer that draws flags for bases that have @ref EF_SHOW_FLAG set.
 */

layer_base_flags::layer_base_flags(struct tileset *ts, const QPoint &offset)
    : freeciv::layer(ts, LAYER_BASE_FLAGS), m_offset(offset)
{
}

std::vector<drawn_sprite> layer_base_flags::fill_sprite_array(
    const tile *ptile, const tile_edge *pedge, const tile_corner *pcorner,
    const unit *punit) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);
  Q_UNUSED(punit);

  if (ptile == nullptr) {
    return {};
  }

  const auto eowner = extra_owner(ptile);
  if (eowner == nullptr) {
    return {};
  }

  extra_type_iterate(pextra)
  {
    if (tile_has_extra(ptile, pextra)
        && extra_has_flag(pextra, EF_SHOW_FLAG)) {
      bool hidden = false;

      extra_type_list_iterate(pextra->hiders, phider)
      {
        if (tile_has_extra(ptile, phider)) {
          hidden = true;
          break;
        }
      }
      extra_type_list_iterate_end;

      if (!hidden) {
        // Draw a flag for this extra
        return {drawn_sprite(
            tileset(),
            get_nation_flag_sprite(tileset(), nation_of_player(eowner)),
            true, m_offset)};
      }
    }
  }
  extra_type_iterate_end;

  return {};
}

} // namespace freeciv
