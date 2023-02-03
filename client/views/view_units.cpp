/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
 */

/*
 * \file This file contains functions to generate the table based GUI for
 * the units view.
 */

#include "views/view_units.h"
// client
#include "client_main.h"
#include "fc_client.h"
#include "hudwidget.h"
#include "page_game.h"
#include "repodlgs_common.h"
#include "tileset/sprite.h"
#include "top_bar.h"
//#include "fcintl.h"

/**
 * Returns an array of units with upkeep. Number of units in
 * the array is added to num_entries_used.
void get_units_view_data()
{
  int count, cost, partial_cost, num_entries_used, total_cost;

  num_entries_used = 0;
  total_cost = 0;

  if (nullptr == client.conn.playing) {
    return;
  }

  unit_type_iterate(unittype)
  {
    cost = utype_upkeep_cost(unittype, client.conn.playing, O_GOLD);

    if (cost == 0) {
      // Short-circuit all of the following checks.
      continue;
    }

    count = 0;
    partial_cost = 0;

    city_list_iterate(client.conn.playing->cities, pcity)
    {
      unit_list_iterate(pcity->units_supported, punit)
      {
        if (unit_type_get(punit) == unittype) {
          count++;
          partial_cost += punit->upkeep[O_GOLD];
        }
      }
      unit_list_iterate_end;
    }
    city_list_iterate_end;

    if (count == 0) {
      continue;
    }

    (total_cost) += partial_cost;

    entries[num_entries_used].type = unittype;
    entries[num_entries_used].count = count;
    entries[num_entries_used].cost = cost;
    entries[num_entries_used].total_cost = partial_cost;
    (num_entries_used)++;
  }
  unit_type_iterate_end;
}
 */

/**
   Constructor for units view
 */
units_view::units_view() : QWidget()
{
  ui.setupUi(this);

  QStringList slist;
  slist << _("Unit Type") << _("Upgradable ★") << _("In Progress ⚒")
        << _("Active ⚔") << _("Shield Upkeep") << _("Food Upkeep")
        << _("Gold Upkeep") << QLatin1String("");
  ui.units_widget->setColumnCount(slist.count());
  ui.units_widget->setHorizontalHeaderLabels(slist);
  ui.units_widget->setSortingEnabled(0);
  ui.upg_but->setText(_("Upgrade"));
  ui.upg_but->setDisabled("True");
  ui.find_but->setText(_("Find Nearest"));
  ui.find_but->setDisabled("True");
  ui.disband_but->setText(_("Disband All"));
  ui.disband_but->setDisabled("True");

  /*connect(ui.upg_but, &QAbstractButton::pressed, this,
          &units_view::upgrade_units);
  connect(ui.find_but, &QAbstractButton::pressed, this,
          &units_view::find_nearest);
  connect(ui.disband_but, &QAbstractButton::pressed, this,
          &units_view::disband_units);
  connect(ui.units_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &units_view::selection_changed);*/
  setLayout(ui.units_layout);
  queen()->gimmePlace(this, QStringLiteral("UNI"));
  index = queen()->addGameTab(this);
}

/**
   Destructor for units view
 */
units_view::~units_view() { queen()->removeRepoDlg(QStringLiteral("UNI")); }

/**
   Initializes place in tab for units view
 */
void units_view::init() { queen()->game_tab_widget->setCurrentIndex(index); }

/**
   Refresh all widgets for units view
 */
void units_view::update_view()
{
  struct unit_view_entry unit_entries[U_LAST];
  int entries_used, i, j, h, total_gold, total_shield, total_food,
      total_count;
  QString buf;
  QTableWidgetItem *item;
  QTableWidgetItem *item_totals;
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  h = fm.height() + 10;

  ui.units_widget->setRowCount(0);
  ui.units_widget->clearContents();

  total_count = 0;
  total_gold = 0;
  total_shield = 0;
  total_food = 0;
  max_row = 0;

  get_units_view_data(unit_entries, &entries_used);
  for (i = 0; i < entries_used; i++) {
    struct unit_view_entry *pentry = unit_entries + i;
    struct unit_type *putype = pentry->type;
    cid id;

    auto sprite = get_unittype_sprite(tileset, putype, direction8_invalid());
    id = cid_encode_unit(putype);

    ui.units_widget->insertRow(max_row);
    for (j = 0; j < 7; j++) {
      item = new QTableWidgetItem;
      switch (j) {
      case 0:
        // Unit type image sprite
        if (sprite != nullptr) {
          item->setData(Qt::DecorationRole, sprite->scaledToHeight(h));
        }
        item->setData(Qt::UserRole, id);
        item->setTextAlignment(Qt::AlignLeft);
        item->setText(utype_name_translation(putype));
        break;
      case 1:
        // Upgradable
        item->setData(Qt::DisplayRole, 0);
        break;
      case 2:
        // In Progress
        item->setData(Qt::DisplayRole, 0);
        break;
      case 3:
        // Active
        item->setData(Qt::DisplayRole, pentry->count);
        total_count += pentry->count;
        break;
      case 4:
        // Shield upkeep
        if (pentry->shield_cost == 0) {
          item->setText("-");
          item->setTextAlignment(Qt::AlignCenter);
        } else {
          item->setData(Qt::DisplayRole, pentry->shield_cost);
        }
        total_shield += pentry->shield_cost;
        break;
      case 5:
        // Food upkeep
        if (pentry->food_cost == 0) {
          item->setText("-");
          item->setTextAlignment(Qt::AlignCenter);
        } else {
          item->setData(Qt::DisplayRole, pentry->food_cost);
        }
        total_food += pentry->food_cost;
        break;
      case 6:
        // Gold upkeep
        if (pentry->gold_cost == 0) {
          item->setText("-");
          item->setTextAlignment(Qt::AlignCenter);
        } else {
          item->setData(Qt::DisplayRole, pentry->gold_cost);
        }
        total_gold += pentry->gold_cost;
        break;
      }
      item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
      ui.units_widget->setItem(max_row, j, item);
    }
    max_row++;
  }

  // max_row ++;
  ui.units_widget->setRowCount(max_row);
  ui.units_widget->insertRow(max_row);
  for (j = 0; j < 7; j++) {
    item_totals = new QTableWidgetItem;
    switch (j) {
    case 0:
      // Unit type
      item_totals->setTextAlignment(Qt::AlignRight);
      item_totals->setText(_("---------------\nTotals:"));
      break;
    case 1:
      // Upgradable
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(_("-------\n0"));
      // item->setData(Qt::DisplayRole, 0);
      break;
    case 2:
      // In Progress
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(_("-------\n0"));
      // item->setData(Qt::DisplayRole, 0);
      break;
    case 3:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("-------\n%1")).arg(total_count));
      // item_totals->setData(Qt::DisplayRole, total_count);
      break;
    case 4:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("-------\n%1")).arg(total_shield));
      // item_totals->setData(Qt::DisplayRole, total_shield);
      break;
    case 5:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("-------\n%1")).arg(total_food));
      // item_totals->setData(Qt::DisplayRole, total_food);
      break;
    case 6:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("-------\n%1")).arg(total_gold));
      // item_totals->setData(Qt::DisplayRole, total_gold);
      break;
    }
    ui.units_widget->setItem(max_row, j, item_totals);
  }

  buf = QString(_("Units View: There are %1 active units costing %2 Gold, "
                  "%3 Food, and %4 Shields in total."))
            .arg(QString::number(total_count), QString::number(total_gold),
                 QString::number(total_food), QString::number(total_shield));
  ui.units_label->setText(buf);
  ui.units_widget->resizeRowsToContents();
  ui.units_widget->resizeColumnsToContents();
}

/**
   Action for selection changed in units view
void units_view::selection_changed(const QItemSelection &sl,
                                   const QItemSelection &ds)
{
  QTableWidgetItem *itm;
  int i;
  QVariant qvar;
  struct universal selected;
  const struct impr_type *pimprove;
  ui.bdisband->setEnabled(false);
  ui.bsell->setEnabled(false);
  ui.bredun->setEnabled(false);

  if (sl.isEmpty()) {
    return;
  }

  curr_row = sl.indexes().at(0).row();
  if (curr_row >= 0 && curr_row <= max_row) {
    itm = ui.eco_widget->item(curr_row, 0);
    qvar = itm->data(Qt::UserRole);
    uid = qvar.toInt();
    selected = cid_decode(uid);
    switch (selected.kind) {
    case VUT_IMPROVEMENT:
      pimprove = selected.value.building;
      counter = ui.eco_widget->item(curr_row, 3)->text().toInt();
      if (can_sell_building(pimprove)) {
        ui.bsell->setEnabled(true);
      }
      itm = ui.eco_widget->item(curr_row, 2);
      i = itm->text().toInt();
      if (i > 0) {
        ui.bredun->setEnabled(true);
      }
      break;
    case VUT_UTYPE:
      counter = ui.eco_widget->item(curr_row, 3)->text().toInt();
      ui.bdisband->setEnabled(true);
      break;
    default:
      qCritical("Not supported type: %d.", selected.kind);
    }
  }
}
 */

/**
   Disband pointed units (in units view)
void units_view::disband_units()
{
  struct universal selected;
  QString buf;
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  Unit_type_id utype;

  selected = cid_decode(uid);
  utype = utype_number(selected.value.utype);
  buf = QString(_("Do you really wish to disband every %1 (%2 total)?"))
            .arg(utype_name_translation(utype_by_number(utype)),
                 QString::number(counter));

  ask->set_text_title(buf, _("Disband Units?"));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Disband"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, [=]() {
    struct unit_type *putype = utype_by_number(utype);
    char buf[1024];
    hud_message_box *result;

    if (putype) {
      disband_all_units(putype, false, buf, sizeof(buf));
      result = new hud_message_box(king()->central_wdg);
      result->set_text_title(buf, _("Disband Results"));
      result->setStandardButtons(QMessageBox::Ok);
      result->setAttribute(Qt::WA_DeleteOnClose);
      result->show();
    }
  });
}
 */

/**
   Find nearest unit
void units_view::find_nearest()
{


}
 */

/**
   Upgrade Units
void units_view::upgrade_units()
{
  QString b, c;
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  int price;
  const struct unit_type *upgrade;
  const Unit_type_id type = utype_number(utype);

  upgrade = can_upgrade_unittype(client_player(), utype);
  price = unit_upgrade_price(client_player(), utype, upgrade);
  b = QString::asprintf(PL_("Treasury contains %d gold.",
                            "Treasury contains %d gold.",
                            client_player()->economic.gold),
                        client_player()->economic.gold);
  c = QString::asprintf(PL_("Upgrade as many %s to %s as possible "
                            "for %d gold each?\n%s",
                            "Upgrade as many %s to %s as possible "
                            "for %d gold each?\n%s",
                            price),
                        utype_name_translation(utype),
                        utype_name_translation(upgrade), price,
                        b.toUtf8().data());
  ask->set_text_title(c, _("Upgrade Obsolete Units?"));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Upgrade"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted,
          [=]() { dsend_packet_unit_type_upgrade(&client.conn, type); });
  ask->show();
}
 */

/**
   Update the units view.
 */
void units_view_dialog_update(void *unused)
{
  int i;
  units_view *uv;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("UNI"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("UNI"));
    if (queen()->game_tab_widget->currentIndex() == i) {
      w = queen()->game_tab_widget->widget(i);
      uv = reinterpret_cast<units_view *>(w);
      uv->update_view();
    }
  }
  queen()->updateSidebarTooltips();
}

/**
 * Display the unis view. Typically triggered by F2.
 */
void units_view_dialog_popup()
{
  int i;
  units_view *uv;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("UNI"))) {
    uv = new units_view;
    uv->init();
    uv->update_view();
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("UNI"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    if (w->isVisible()) {
      top_bar_show_map();
      return;
    }
    uv = reinterpret_cast<units_view *>(w);
    uv->update_view();
    queen()->game_tab_widget->setCurrentWidget(uv);
  }
}

/**
   Closes units view
 */
void popdown_units_view()
{
  int i;
  units_view *uv;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("UNI"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("UNI"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    uv = reinterpret_cast<units_view *>(w);
    uv->deleteLater();
  }
}
