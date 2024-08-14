// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer.h"

#include <array>

namespace freeciv {

class layer_water : public layer {
  struct extra_data {
    using terrain_data = std::array<QPixmap *, MAX_INDEX_CARDINAL>;

    const extra_type *extra;
    std::vector<terrain_data> sprites;
  };

public:
  explicit layer_water(struct tileset *ts);
  virtual ~layer_water() = default;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void reset_ruleset() override;

private:
  void fill_irrigation_sprite_array(const struct tileset *t,
                                    std::vector<drawn_sprite> &sprs,
                                    bv_extras textras,
                                    bv_extras *textras_near,
                                    const terrain *pterrain,
                                    const city *pcity) const;

  std::vector<extra_data> m_cardinals, m_outlets, m_rivers;
};

} // namespace freeciv
