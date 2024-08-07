// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer.h"
#include "unit.h"

#include <QPoint>

namespace freeciv {

class layer_workertask : public layer {
public:
  explicit layer_workertask(struct tileset *ts, mapview_layer layer,
                            const QPoint &activity_offset);
  virtual ~layer_workertask() = default;

  void load_sprites() override;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void reset_ruleset() override;

private:
  QPoint m_activity_offset;
  std::array<QPixmap *, ACTIVITY_LAST> m_activities = {nullptr};
  std::vector<QPixmap *> m_extra_activities, m_extra_rm_activities;
};

} // namespace freeciv
