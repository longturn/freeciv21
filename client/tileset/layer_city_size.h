// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "layer.h"

#include <QPoint>

class QPixmap;

namespace freeciv {

class layer_city_size : public layer {
public:
  explicit layer_city_size(struct tileset *ts, const QPoint &offset);
  virtual ~layer_city_size() = default;

  void load_sprites() override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

private:
  mutable bool m_warned = false; ///< Did we warn the user?

  QPoint m_offset;
  std::array<QPixmap *, NUM_TILES_DIGITS> m_units, m_tens, m_hundreds;
};

} // namespace freeciv
