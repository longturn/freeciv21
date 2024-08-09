// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer.h"

namespace freeciv {

class layer_roads : public layer {
public:
  explicit layer_roads(struct tileset *ts);
  virtual ~layer_roads() = default;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void reset_ruleset() override;

private:
  std::vector<const extra_type *> m_all_combined, m_all_separate,
      m_parity_combined;
};

} // namespace freeciv
