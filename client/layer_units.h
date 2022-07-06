/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include "layer.h"

namespace freeciv {

class layer_units : public layer {
public:
  explicit layer_units(struct tileset *ts, mapview_layer layer);
  virtual ~layer_units() = default;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;
};

} // namespace freeciv
