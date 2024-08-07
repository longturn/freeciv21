// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "layer.h"

#include <QPixmap>

namespace freeciv {

class layer_editor : public layer {
public:
  explicit layer_editor(struct tileset *ts);
  virtual ~layer_editor() = default;

  void load_sprites() override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

private:
  QPixmap m_selected, *m_starting_position;
};

} // namespace freeciv
