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
#pragma once

#include "fc_types.h"
#include "layer.h"

#include <QPixmap>

#include <array>
#include <memory>

namespace freeciv {

class layer_background : public layer {
public:
  explicit layer_background(struct tileset *ts);
  virtual ~layer_background() = default;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner, const unit *punit,
                    const city *pcity,
                    const unit_type *putype) const override;

  void initialize_player(const player *player) override;
  void free_player(int player_id) override;

private:
  std::unique_ptr<QPixmap> create_player_sprite(const QColor &pcolor) const;

  std::array<std::unique_ptr<QPixmap>, MAX_NUM_PLAYER_SLOTS>
      m_player_background;
};

} // namespace freeciv
