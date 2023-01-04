/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 * SPDX-FileCopyrightText: Copyright (c) 1996-2022 Freeciv21 and Freeciv
 * contributors
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "improvement_seller.h"

#include "citydlg_common.h"
#include "client_main.h"
#include "game.h"
#include "hudwidget.h"
#include "improvement.h"
#include "log.h"

#include <QMenu>

namespace freeciv {

/**
 * @class improvement_seller
 * @brief
 * Helper class to safely sell a city improvement.
 *
 * This class adds an action to a menu (via @ref add_to_menu) to sell a city
 * improvement. When the item is activated, the potential gain is displayed
 * and the user is prompted for confirmation. If the user confirms, the
 * improvement is sold.
 *
 * The seller can also be used directly with its @ref operator().
 *
 * This class handles all corner cases where the city may be disbanded or the
 * improvement disappear for some reason.
 */

/**
 * Constructor.
 */
improvement_seller::improvement_seller(QWidget *parent, int city_id,
                                       int improvement_id)
    : m_city(city_id), m_improvement(improvement_id), m_parent(parent)
{
}

/**
 * Asks for confirmation then sells the improvement.
 */
void improvement_seller::operator()()
{
  if (!can_client_issue_orders()) {
    return; // Kicked
  }

  const auto city = game_city_by_number(m_city);
  const auto building = improvement_by_number(m_improvement);

  fc_assert_ret(city != nullptr);
  fc_assert_ret(building != nullptr);

  // Cannot sell it now
  if (test_player_sell_building_now(client.conn.playing, city, building)
      != TR_SUCCESS) {
    return;
  }

  const auto price = impr_sell_gold(building);
  const auto buf =
      QString(PL_("Sell %1 for %2 gold?", "Sell %1 for %2 gold?", price))
          .arg(city_improvement_name_translation(city, building),
               QString::number(price));

  auto ask = new hud_message_box(m_parent);
  ask->setAttribute(Qt::WA_DeleteOnClose);

  ask->set_text_title(buf, (_("Sell Improvement?")));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Sell"));

  int city_id = m_city, improvement_id = m_improvement; // For lambda capture
  QObject::connect(ask, &hud_message_box::accepted, [=] {
    auto city = game_city_by_number(city_id);
    if (!city || !can_client_issue_orders()) {
      return;
    }
    city_sell_improvement(city, improvement_id);
  });

  ask->show();
}

/**
 * @brief Adds a menu item to sell an improvement in a city.
 *
 * The @c parent parameter is used as the parent for the confirmation popup.
 * It may not be null. The @c parent must outlive the menu.
 */
QAction *improvement_seller::add_to_menu(QWidget *parent, QMenu *menu,
                                         const city *city,
                                         int improvement_id)
{
  fc_assert_ret_val(parent != nullptr, nullptr);

  auto action =
      menu->addAction(_("Sell Improvement"),
                      improvement_seller(parent, city->id, improvement_id));

  auto building = improvement_by_number(improvement_id);
  action->setEnabled(
      building && can_client_issue_orders()
      && test_player_sell_building_now(client.conn.playing, city, building)
             == TR_SUCCESS);

  return action;
}

} // namespace freeciv
