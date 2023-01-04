/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 * SPDX-FileCopyrightText: Copyright (c) 1996-2022 Freeciv21 and Freeciv
 * contributors
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "unit_quick_menu.h"
#include "actions.h"
#include "citydlg.h"
#include "client_main.h"
#include "control.h"
#include "dialogs.h"
#include "game.h"
#include "page_game.h"
#include "qtg_cxxside.h"
#include "unit.h"
#include "unitlist.h"

#include <QMenu>
#include <algorithm>

namespace freeciv {

namespace /* anonymous */ {

/**
 * Creates a callback function to call @c cb with @c unit_ids turned into
 * valid unit pointers.
 */
template <class Callback>
auto unit_action_helper(const std::vector<int> &unit_ids, Callback &&cb)
{
  return [unit_ids, cb] {
    // Check playable units
    if (!can_client_issue_orders()) {
      return;
    }

    auto units = std::vector<unit *>();
    for (const auto id : unit_ids) {
      const auto unit = game_unit_by_number(id);
      // Maybe some units died
      if (unit && unit_owner(unit) == client_player()) {
        units.push_back(unit);
      }
    }

    cb(units);
  };
}

/**
 * Callback taking a single unit.
 */
template <class RetType> using unit_callback_type = RetType (*)(unit *);

/**
 * Creates a callback function to call @c cb repeatedly for each valid unit
 * in
 * @c unit_ids turned into a unit pointer.
 */
template <class RetType>
auto unit_action_helper(const std::vector<int> &unit_ids,
                        unit_callback_type<RetType> cb)
{
  return unit_action_helper(unit_ids, [cb](auto units) {
    std::for_each(units.begin(), units.end(), cb);
  });
}

/**
 * Adds an action to @c menu with the given @c text, applying @c cb to the
 * units listed in @c unit_ids. The menu item is automatically disabled if
 * the client cannot issue orders.
 */
template <class Callback>
void add_units_action(QMenu *menu, const QString &text,
                      const std::vector<int> &unit_ids, bool enabled,
                      Callback &&cb)
{
  auto action = menu->addAction(text, unit_action_helper(unit_ids, cb));
  action->setEnabled(enabled && can_client_issue_orders());
}

/**
 * Callback to activate units.
 */
void activate_callback(const std::vector<unit *> &units)
{
  unit_focus_set(nullptr); // Clear
  for (const auto unit : units) {
    unit_focus_add(unit);
  }

  if (!units.empty()) {
    queen()->city_overlay->dont_focus = true;
    popdown_city_dialog();
  }
}

/**
 * Callback to load units into transports.
 */
void load_callback(const std::vector<unit *> &units)
{
  for (const auto unit : units) {
    request_transport(unit, unit_tile(unit));
  }
}

} // anonymous namespace

/**
 * Adds a small set of common unit actions to a menu. The menu items will
 * operate on the given units.
 */
void add_quick_unit_actions(QMenu *menu, const std::vector<unit *> &units)
{
  // Get unit IDs (don't store pointers that could be deleted)
  std::vector<int> unit_ids;
  std::transform(units.begin(), units.end(), std::back_inserter(unit_ids),
                 [](auto u) { return u->id; });

  add_units_action(menu, _("Activate Unit"), unit_ids, true,
                   activate_callback);

  add_units_action(menu, _("Sentry Unit"), unit_ids,
                   can_units_do_activity(units, ACTIVITY_SENTRY),
                   request_unit_sentry);

  add_units_action(menu, action_id_name_translation(ACTION_FORTIFY),
                   unit_ids,
                   units_can_do_action(units, ACTION_FORTIFY, true),
                   request_unit_fortify);

  add_units_action(menu, action_id_name_translation(ACTION_DISBAND_UNIT),
                   unit_ids,
                   units_can_do_action(units, ACTION_DISBAND_UNIT, true),
                   popup_disband_dialog);

  add_units_action(menu, action_id_name_translation(ACTION_HOME_CITY),
                   unit_ids,
                   units_can_do_action(units, ACTION_HOME_CITY, true),
                   request_unit_change_homecity);

  add_units_action(menu, action_id_name_translation(ACTION_TRANSPORT_BOARD),
                   unit_ids, units_can_load(units), load_callback);

  add_units_action(menu, action_id_name_translation(ACTION_TRANSPORT_ALIGHT),
                   unit_ids, units_can_unload(units), request_unit_unload);

  add_units_action(menu, action_id_name_translation(ACTION_TRANSPORT_UNLOAD),
                   unit_ids, units_are_occupied(units),
                   request_unit_unload_all);

  add_units_action(menu, action_id_name_translation(ACTION_UPGRADE_UNIT),
                   unit_ids, units_can_upgrade(units), popup_upgrade_dialog);
}

} // namespace freeciv
