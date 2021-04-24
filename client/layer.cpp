/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 2021 Freeciv21 contributors.
\_   \        /  __/                        This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include "tilespec.h"

#include "layer.h"

namespace freeciv {

std::vector<drawn_sprite>
layer::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                         const tile_corner *pcorner, const unit *punit,
                         const city *pcity, const unit_type *putype) const
{
  return ::fill_sprite_array(m_ts, m_layer, ptile, pedge, pcorner, punit,
                             pcity, putype);
}

} // namespace freeciv
