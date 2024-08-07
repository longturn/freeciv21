/*
 * SPDX-FileCopyrightText: 2022-2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "layer_units.h"

#include "control.h"
#include "movement.h"
#include "options.h"
#include "rgbcolor.h"
#include "tilespec.h"

/**
 * \class freeciv::layer_units
 * \brief Draws units on the map.
 */

namespace freeciv {

/**
 * Constructor
 */
layer_units::layer_units(struct tileset *ts, mapview_layer layer,
                         const QPoint &activity_offset,
                         const QPoint &select_offset,
                         const QPoint &unit_offset,
                         const QPoint &unit_flag_offset)
    : freeciv::layer(ts, layer), m_activity_offset(activity_offset),
      m_select_offset(select_offset), m_unit_offset(unit_offset),
      m_unit_flag_offset(unit_flag_offset)
{
}

/**
 * Loads all static sprites needed by this layer (activities etc).
 */
void layer_units::load_sprites()
{
  m_activities[ACTIVITY_MINE] = load_sprite(tileset(), {"unit.plant"}, true);
  m_activities[ACTIVITY_PLANT] = m_activities[ACTIVITY_MINE];
  m_activities[ACTIVITY_IRRIGATE] =
      load_sprite(tileset(), {"unit.irrigate"}, true);
  m_activities[ACTIVITY_CULTIVATE] = m_activities[ACTIVITY_IRRIGATE];
  m_activities[ACTIVITY_PILLAGE] =
      load_sprite(tileset(), {"unit.pillage"}, true);
  m_activities[ACTIVITY_FORTIFIED] =
      load_sprite(tileset(), {"unit.fortified"}, true);
  m_activities[ACTIVITY_FORTIFYING] =
      load_sprite(tileset(), {"unit.fortifying"}, true);
  m_activities[ACTIVITY_SENTRY] =
      load_sprite(tileset(), {"unit.sentry"}, true);
  m_activities[ACTIVITY_GOTO] = load_sprite(tileset(), {"unit.goto"}, true);
  m_activities[ACTIVITY_TRANSFORM] =
      load_sprite(tileset(), {"unit.transform"}, true);
  m_activities[ACTIVITY_CONVERT] =
      load_sprite(tileset(), {"unit.convert"}, true);

  m_auto_attack = load_sprite(tileset(), {"unit.auto_attack"}, true);
  m_auto_explore = load_sprite(tileset(), {"unit.auto_explore"}, true);
  m_auto_settler = load_sprite(tileset(), {"unit.auto_settler"}, true);
  m_connect = load_sprite(tileset(), {"unit.connect"}, true);
  m_loaded = load_sprite(tileset(), {"unit.loaded"}, true);
  m_lowfuel = load_sprite(tileset(), {"unit.lowfuel"}, true);
  m_patrol = load_sprite(tileset(), {"unit.patrol"}, true);
  m_stack = load_sprite(tileset(), {"unit.stack"}, true);
  m_tired = load_sprite(tileset(), {"unit.tired"}, true);
  m_action_decision_want =
      load_sprite(tileset(), {"unit.action_decision_want"}, false);

  static_assert(MAX_NUM_BATTLEGROUPS < NUM_TILES_DIGITS);
  for (int i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    QStringList buffer = {QStringLiteral("unit.battlegroup_%1").arg(i),
                          QStringLiteral("city.size_%1").arg(i + 1)};
    m_battlegroup[i] = load_sprite(tileset(), {buffer}, true);
  }

  for (int i = 0; i <= 100; i++) {
    auto name = QStringLiteral("unit.hp_%1").arg(i);
    if (auto sprite = load_sprite(tileset(), name); sprite) {
      m_hp_bar.push_back(sprite);
    }
  }

  for (int i = 0; i < MAX_VET_LEVELS; i++) {
    /* Veteran level sprites are optional.  For instance "green" units
     * usually have no special graphic. */
    auto name = QStringLiteral("unit.vet_%1").arg(i);
    m_veteran_level[i] = load_sprite(tileset(), name);
  }

  for (int i = 0;; i++) {
    auto buffer = QStringLiteral("unit.select%1").arg(QString::number(i));
    auto sprite = load_sprite(tileset(), buffer);
    if (!sprite) {
      break;
    }
    m_select.push_back(sprite);
  }
}

/**
 * Loads the extra activity and remove activity sprites.
 */
void layer_units::initialize_extra(const extra_type *extra,
                                   const QString &tag, extrastyle_id style)
{
  fc_assert(extra->id == m_extra_activities.size());

  if (!fc_strcasecmp(extra->activity_gfx, "none")) {
    m_extra_activities.push_back(nullptr);
  } else {
    QStringList tags = {
        extra->activity_gfx,
        extra->act_gfx_alt,
        extra->act_gfx_alt2,
    };
    m_extra_activities.push_back(load_sprite(tileset(), tags, true));
  }

  if (!fc_strcasecmp(extra->rmact_gfx, "none")) {
    m_extra_rm_activities.push_back(nullptr);
  } else {
    QStringList tags = {
        extra->rmact_gfx,
        extra->rmact_gfx_alt,
    };
    m_extra_rm_activities.push_back(load_sprite(tileset(), tags, true));
  }
}

std::vector<drawn_sprite>
layer_units::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                               const tile_corner *pcorner,
                               const unit *punit) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);

  // Should we draw anything in the first place?
  if (!do_draw_unit(ptile, punit)) {
    return {};
  }

  // Only draw the focused unit on LAYER_FOCUS_UNIT
  if ((type() == LAYER_FOCUS_UNIT) != unit_is_in_focus(punit)) {
    return {};
  }

  // Now we draw
  std::vector<drawn_sprite> sprs;
  const auto full_offset = tileset_full_tile_offset(tileset());

  const auto type = unit_type_get(punit);

  // Selection animation. The blinking unit is handled separately, inside
  // get_drawable_unit().
  if (ptile && unit_is_in_focus(punit) && !m_select.empty()) {
    sprs.emplace_back(tileset(), m_select[focus_unit_state()], true,
                      m_select_offset);
  }

  // Flag
  if (!ptile || !tile_city(ptile)) {
    if (!gui_options->solid_color_behind_units) {
      sprs.emplace_back(tileset(),
                        get_unit_nation_flag_sprite(tileset(), punit), true,
                        full_offset + m_unit_flag_offset);
    } else {
      // Taken care of in layer_background.
    }
  }

  // Add the sprite for the unit type.
  {
    const auto rgb = punit->owner ? punit->owner->rgb : nullptr;
    const auto color = rgb ? QColor(rgb->r, rgb->g, rgb->b) : QColor();
    const auto uspr =
        get_unittype_sprite(tileset(), type, punit->facing, color);
    sprs.emplace_back(tileset(), uspr, true, full_offset + m_unit_offset);
  }

  // Loaded
  if (unit_transported(punit)) {
    sprs.emplace_back(tileset(), m_loaded, true, full_offset);
  }

  // Activity
  if (auto sprite =
          get_activity_sprite(punit->activity, punit->activity_target)) {
    sprs.emplace_back(tileset(), sprite, true,
                      full_offset + m_activity_offset);
  }

  // Automated units
  add_automated_sprite(sprs, punit, full_offset);

  // Goto/patrol/connect
  add_orders_sprite(sprs, punit, full_offset);

  // Wants a decision?
  if (m_action_decision_want && should_ask_server_for_actions(punit)) {
    sprs.emplace_back(tileset(), m_action_decision_want, true,
                      full_offset + m_activity_offset);
  }

  // Battlegroup
  if (punit->battlegroup != BATTLEGROUP_NONE) {
    sprs.emplace_back(tileset(), m_battlegroup[punit->battlegroup], true,
                      full_offset);
  }

  // Show a low-fuel graphic if the unit has 2 or fewer moves left.
  if (m_lowfuel && utype_fuel(type) && punit->fuel == 1
      && punit->moves_left <= 2 * SINGLE_MOVE) {
    sprs.emplace_back(tileset(), m_lowfuel, true, full_offset);
  }

  // Out of moves
  if (m_tired && punit->moves_left < SINGLE_MOVE && type->move_rate > 0) {
    // Show a "tired" graphic if the unit has fewer than one move remaining,
    // except for units for which it's full movement.
    sprs.emplace_back(tileset(), m_tired, true, full_offset);
  }

  // Stacks
  if ((ptile && unit_list_size(ptile->units) > 1)
      || punit->client.occupied) {
    sprs.emplace_back(tileset(), m_stack, true, full_offset);
  }

  // Veteran level
  if (m_veteran_level[punit->veteran]) {
    sprs.emplace_back(tileset(), m_veteran_level[punit->veteran], true,
                      full_offset);
  }

  // HP
  {
    auto ihp = ((m_hp_bar.size() - 1) * punit->hp) / type->hp;
    ihp = CLIP(0, ihp, m_hp_bar.size() - 1); // Safety
    sprs.emplace_back(tileset(), m_hp_bar[ihp], true, full_offset);
  }

  return sprs;
}

/**
 * Resets data about extras
 */
void layer_units::reset_ruleset()
{
  m_extra_activities.clear();
  m_extra_rm_activities.clear();
}

/**
 * Returns the sprite used to represent a given activity on the map.
 */
QPixmap *layer_units::get_activity_sprite(unit_activity activity,
                                          const extra_type *target) const
{
  // Nicely inconsistent
  switch (activity) {
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT:
    return m_extra_rm_activities[extra_index(target)];
    break;
  case ACTIVITY_BASE:
  case ACTIVITY_GEN_ROAD:
    return m_extra_activities[extra_index(target)];
    break;
  default:
    break;
  }

  return target ? m_extra_activities[extra_index(target)]
                : m_activities[activity];
}

/**
 * Adds the sprite used to represent an automated unit on the map to sprs.
 */
void layer_units::add_automated_sprite(std::vector<drawn_sprite> &sprs,
                                       const unit *punit,
                                       const QPoint &full_offset) const
{
  QPixmap *sprite = nullptr;
  QPoint offset;

  switch (punit->ssa_controller) {
  case SSA_NONE:
    break;
  case SSA_AUTOSETTLER:
    sprite = m_auto_settler;
    break;
  case SSA_AUTOEXPLORE:
    sprite = m_auto_explore;
    // Specified as an activity in the tileset.
    offset = m_activity_offset;
    break;
  default:
    sprite = m_auto_attack; // FIXME What's this hack?
    break;
  }

  if (sprite) {
    sprs.emplace_back(tileset(), sprite, true, full_offset + offset);
  }
}

/**
 * Adds the sprite used to represent unit orders to sprs.
 */
void layer_units::add_orders_sprite(std::vector<drawn_sprite> &sprs,
                                    const unit *punit,
                                    const QPoint &full_offset) const
{
  if (unit_has_orders(punit)) {
    if (punit->orders.repeat) {
      sprs.emplace_back(tileset(), m_patrol, true, full_offset);
    } else if (punit->activity != ACTIVITY_IDLE) {
      sprs.emplace_back(tileset(), m_connect);
    } else {
      sprs.emplace_back(tileset(), m_activities[ACTIVITY_GOTO], true,
                        full_offset + m_activity_offset);
    }
  }
}

} // namespace freeciv
