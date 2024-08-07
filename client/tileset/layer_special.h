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
#pragma once

#include "fc_types.h"
#include "layer.h"

#include <QPixmap>

#include <array>
#include <memory>

namespace freeciv {

class layer_special : public layer {
public:
  explicit layer_special(struct tileset *ts, mapview_layer layer);
  virtual ~layer_special() = default;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  void set_sprite(const extra_type *extra, const QString &tag,
                  const QPoint &offset = QPoint());

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void reset_ruleset() override;

private:
  std::array<std::unique_ptr<drawn_sprite>, MAX_EXTRA_TYPES> m_sprites;
};

} // namespace freeciv
