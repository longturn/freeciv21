// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include <array>

#include "fc_types.h"
#include "layer.h"

class QPixmap;
struct tileset;

namespace freeciv {

class layer_abstract_activities : public layer {
public:
  /// Inherited constructor.
  using layer::layer;

  void load_sprites() override;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  void reset_ruleset() override;

  QPixmap *activity_sprite(unit_activity id, const extra_type *extra) const;

private:
  std::array<QPixmap *, ACTIVITY_LAST> m_activities = {nullptr};
  std::vector<QPixmap *> m_extra_activities, m_extra_rm_activities;
};

} // namespace freeciv
