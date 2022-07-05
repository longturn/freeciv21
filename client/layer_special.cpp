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

#include "layer_special.h"

#include "extras.h"
#include "game.h" // For extra_type_iterate
#include "sprite_g.h"
#include "tilespec.h"

namespace freeciv {

layer_special::layer_special(struct tileset *ts, mapview_layer layer)
    : freeciv::layer(ts, layer), m_sprites{nullptr}
{
  fc_assert(layer == LAYER_SPECIAL1 || layer == LAYER_SPECIAL2
            || layer == LAYER_SPECIAL3);
}

void layer_special::set_sprite(const extra_type *extra, const QString &tag,
                               int offset_x, int offset_y)
{
  fc_assert_ret(extra != nullptr);

  auto sprite = load_sprite(tileset(), tag);
  if (sprite) {
    m_sprites.at(extra->id) = std::make_unique<drawn_sprite>(
        tileset(), sprite, true, offset_x, offset_y);
  }
}

std::vector<drawn_sprite>
layer_special::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                                 const tile_corner *pcorner,
                                 const unit *punit, const city *pcity) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);
  Q_UNUSED(punit);
  Q_UNUSED(pcity);

  if (ptile == nullptr) {
    return {};
  }

  auto sprites = std::vector<drawn_sprite>();

  extra_type_iterate(pextra)
  {
    if (tile_has_extra(ptile, pextra) && is_extra_drawing_enabled(pextra)
        && m_sprites[extra_index(pextra)]) {
      // Check whether the extra is hidden by some other extra.
      bool hidden = false;
      extra_type_list_iterate(pextra->hiders, phider)
      {
        if (BV_ISSET(ptile->extras, extra_index(phider))) {
          hidden = true;
          break;
        }
      }
      extra_type_list_iterate_end;

      if (!hidden) {
        sprites.push_back(*m_sprites[extra_index(pextra)]);
      }
    }
  }
  extra_type_iterate_end;

  return sprites;
}

void layer_special::reset_ruleset()
{
  // The array is indexed by the extra id, which depends on the ruleset.
  // Clear it.
  for (auto &sprite : m_sprites) {
    sprite = nullptr;
  }
}

} // namespace freeciv
