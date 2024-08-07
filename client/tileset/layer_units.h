/*
 * SPDX-FileCopyrightText: 2022-2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include "fc_types.h"
#include "layer.h"
#include "unit.h"

#include <QPoint>

namespace freeciv {

class layer_units : public layer {
public:
  explicit layer_units(struct tileset *ts, mapview_layer layer,
                       const QPoint &activity_offset,
                       const QPoint &select_offset,
                       const QPoint &unit_offset,
                       const QPoint &unit_flag_offset);
  virtual ~layer_units() = default;

  void load_sprites() override;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void reset_ruleset() override;

  // What follows is a bit hacky, but we can't do better until we have more
  // general animation support.

  /**
   * Returns the number of steps in the focused unit animation.
   */
  auto focus_unit_state_count() const { return m_select.size(); }

  /**
   * Returns the current state of the focused unit animation.
   */
  int focus_unit_state() const { return m_focus_unit_state; }

  /**
   * Returns the current state of the focused unit animation.
   */
  int &focus_unit_state() { return m_focus_unit_state; }

private:
  QPixmap *get_activity_sprite(unit_activity activity,
                               const extra_type *target) const;
  void add_automated_sprite(std::vector<drawn_sprite> &sprs,
                            const unit *punit,
                            const QPoint &full_offset) const;
  void add_orders_sprite(std::vector<drawn_sprite> &sprs, const unit *punit,
                         const QPoint &full_offset) const;

  int m_focus_unit_state = 0; ///< State of the focused unit animation.

  QPixmap *m_auto_attack, *m_auto_settler, *m_auto_explore, *m_connect,
      *m_loaded, *m_lowfuel, *m_patrol, *m_stack, *m_tired,
      *m_action_decision_want;
  std::vector<QPixmap *> m_hp_bar, m_select;
  std::array<QPixmap *, ACTIVITY_LAST> m_activities = {nullptr};
  std::array<QPixmap *, MAX_NUM_BATTLEGROUPS> m_battlegroup = {nullptr};
  std::array<QPixmap *, MAX_VET_LEVELS> m_veteran_level = {nullptr};

  QPoint m_activity_offset, m_select_offset, m_unit_offset,
      m_unit_flag_offset;

  std::vector<QPixmap *> m_extra_activities, m_extra_rm_activities;
};

} // namespace freeciv
