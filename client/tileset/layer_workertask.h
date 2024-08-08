// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer_abstract_activities.h"
#include "unit.h"

#include <QPoint>

namespace freeciv {

class layer_workertask : public layer_abstract_activities {
public:
  explicit layer_workertask(struct tileset *ts, mapview_layer layer,
                            const QPoint &activity_offset);
  virtual ~layer_workertask() = default;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

private:
  QPoint m_activity_offset;
};

} // namespace freeciv
