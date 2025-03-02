// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_editor.h"

#include "editor.h"
#include "map.h"
#include "sprite_g.h"
#include "tilespec.h"

#include <memory>

namespace freeciv {

layer_editor::layer_editor(struct tileset *ts)
    : freeciv::layer(ts, LAYER_EDITOR)
{
}

void layer_editor::load_sprites()
{
  // FIXME: Use better sprites.
  auto color = load_sprite({"colors.overlay_0"}, true);
  auto unworked = load_sprite({"mask.unworked_tile"}, true);

  auto W = tileset_tile_width(tileset()), H = tileset_tile_height(tileset());
  auto mask = std::unique_ptr<QPixmap>(
      crop_sprite(color, 0, 0, W, H, get_mask_sprite(tileset()), 0, 0));
  auto selected = std::unique_ptr<QPixmap>(
      crop_sprite(mask.get(), 0, 0, W, H, unworked, 0, 0));
  m_selected = *selected;

  // FIXME: Use a more representative sprite.
  m_starting_position = load_sprite({"user.attention"}, true);
}

std::vector<drawn_sprite>
layer_editor::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                                const tile_corner *pcorner,
                                const unit *punit) const
{
  if (!ptile || !editor_is_active()) {
    return {};
  }

  std::vector<drawn_sprite> sprs;

  if (editor_tile_is_selected(ptile)) {
    sprs.emplace_back(tileset(), &m_selected);
  }

  if (map_startpos_get(ptile)) {
    sprs.emplace_back(tileset(), m_starting_position);
  }

  return sprs;
}

} // namespace freeciv
