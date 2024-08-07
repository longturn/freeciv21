// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "layer.h"

#include <QPixmap>

#include <memory>

namespace freeciv {

class layer_overlays : public layer {
public:
  explicit layer_overlays(struct tileset *ts);
  virtual ~layer_overlays() = default;

  void load_sprites() override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

private:
  std::vector<std::unique_ptr<QPixmap>> m_worked, m_unworked;
  std::vector<QPixmap *> m_food, m_prod, m_trade;
};

} // namespace freeciv
