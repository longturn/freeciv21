// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "goto.h"
#include "layer.h"

#include <array>

class QPixmap;

namespace freeciv {

class layer_goto : public layer {
public:
  explicit layer_goto(struct tileset *ts);
  virtual ~layer_goto() = default;

  void load_sprites() override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

private:
  mutable bool m_warned = false; ///< Did we warn the user?
  QPixmap *m_waypoint;

  struct tile_state_sprites {
    QPixmap *specific;
    std::array<QPixmap *, NUM_TILES_DIGITS> turns, turns_tens,
        turns_hundreds;
  };
  std::array<tile_state_sprites, GTS_COUNT> m_states;
};

} // namespace freeciv
