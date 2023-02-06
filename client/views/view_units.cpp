/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
 */

/*
 * \file This file contains functions to generate the table based GUI for
 * the units view (F2).
 */

// client
#include "views/view_units.h"
#include "canvas.h"
#include "client_main.h"
#include "fc_client.h"
#include "goto.h"
#include "hudwidget.h"
#include "page_game.h"
#include "repodlgs_common.h"
#include "tileset/sprite.h"
#include "top_bar.h"

// common
#include "movement.h"
#include "text.h"
#include "unittype.h"

// server
//# include "packets_gen.h"

/**
 * Constructor for units view
 */
units_view::units_view() : QWidget()
{
  ui.setupUi(this);

  // Configure the units table
  QStringList slist;
  slist << _("Unit Type") << _("★ Upgradable") << _("⚒ In Progress")
        << _("⚔ Active") << _("Shield Upkeep") << _("Food Upkeep")
        << _("Gold Upkeep") << QLatin1String("");
  ui.units_label->setText(QString(_("Units:")));
  ui.units_widget->setColumnCount(slist.count());
  ui.units_widget->setHorizontalHeaderLabels(slist);
  ui.units_widget->setSortingEnabled(false);
  ui.upg_but->setText(_("Upgrade"));
  ui.upg_but->setDisabled(true);
  ui.find_but->setText(_("Find Nearest"));
  ui.find_but->setDisabled(true);
  ui.disband_but->setText(_("Disband All"));
  ui.disband_but->setDisabled(true);

  // Configure the unitwaittime table
  slist.clear();
  slist << _("Type") << _("Location") << _("Mp") << _("Time left")
        << QLatin1String("");
  ui.uwt_widget->setColumnCount(slist.count());
  ui.uwt_widget->setHorizontalHeaderLabels(slist);
  ui.uwt_widget->setSortingEnabled(false);
  ui.uwt_label->setText("Units Waiting:");

  // Add shield icon for shield upkeep column
  const QPixmap *spr =
      tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.shield",
                                  "citybar.shields", "", "", false);
  QImage img = spr->toImage();
  QRect crop = zealous_crop_rect(img);
  QImage cropped_img = img.copy(crop);
  QPixmap pix = QPixmap::fromImage(cropped_img);
  ui.units_widget->horizontalHeaderItem(4)->setIcon(pix);

  // Add food icon for food upkeep column
  spr = tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.food",
                                    "citybar.food", "", "", false);
  img = spr->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  pix = QPixmap::fromImage(cropped_img);
  ui.units_widget->horizontalHeaderItem(5)->setIcon(pix);

  // Add gold icon for gold upkeep column
  spr = tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.gold",
                                    "citybar.trade", "", "", false);
  img = spr->toImage();
  crop = zealous_crop_rect(img);
  cropped_img = img.copy(crop);
  pix = QPixmap::fromImage(cropped_img);
  ui.units_widget->horizontalHeaderItem(6)->setIcon(pix);

  connect(ui.upg_but, &QAbstractButton::pressed, this,
          &units_view::upgrade_units);
  connect(ui.find_but, &QAbstractButton::pressed, this,
          &units_view::find_nearest);
  connect(ui.disband_but, &QAbstractButton::pressed, this,
          &units_view::disband_units);
  connect(ui.units_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &units_view::selection_changed);
  setLayout(ui.units_layout);
  queen()->gimmePlace(this, QStringLiteral("UNI"));
  index = queen()->addGameTab(this);
}

/**
 * Destructor for units view
 */
units_view::~units_view() { queen()->removeRepoDlg(QStringLiteral("UNI")); }

/**
 * Initializes place in tab for units view
 */
void units_view::init() { queen()->game_tab_widget->setCurrentIndex(index); }

/**
 * Refresh all widgets for units view
 */
void units_view::update_view()
{
  struct unit_view_entry unit_entries[U_LAST];
  int entries_used, i, j, h, total_gold, total_shield, total_food,
      total_count, in_progress, upg_count;
  QTableWidgetItem *item;
  QTableWidgetItem *item_totals;
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  h = fm.height() + 10;

  ui.units_widget->setRowCount(0);
  ui.units_widget->clearContents();

  total_count = 0;
  in_progress = 0;
  total_gold = 0;
  total_shield = 0;
  total_food = 0;
  max_row = 0;
  upg_count = 0;

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
        // Unit type image sprite and name
        if (sprite != nullptr) {
          item->setData(Qt::DecorationRole, sprite->scaledToHeight(h));
        }
        item->setData(Qt::UserRole, id);
        item->setTextAlignment(Qt::AlignLeft);
        item->setText(utype_name_translation(putype));
        break;
      case 1:
        // # Upgradable
        if (pentry->upg) {
          item->setData(Qt::DisplayRole, "★");
          upg_count++;
        } else {
          item->setData(Qt::DisplayRole, "-");
        }
        break;
      case 2:
        // # In Progress
        item->setData(Qt::DisplayRole, pentry->in_prod);
        in_progress += pentry->in_prod;
        break;
      case 3:
        // # Active
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

  // Add a "footer" to the table showing the totals.
  ui.units_widget->setRowCount(max_row);
  ui.units_widget->insertRow(max_row);
  for (j = 0; j < 7; j++) {
    item_totals = new QTableWidgetItem;
    switch (j) {
    case 0:
      // Unit type
      item_totals->setTextAlignment(Qt::AlignRight);
      item_totals->setText(_("--------------------\nTotals:"));
      break;
    case 1:
      // Upgradable
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(upg_count));
      break;
    case 2:
      // In Progress
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(in_progress));
      break;
    case 3:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_count));
      break;
    case 4:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_shield));
      break;
    case 5:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_food));
      break;
    case 6:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_gold));
      break;
    }
    ui.units_widget->setItem(max_row, j, item_totals);
  }

  ui.units_widget->resizeRowsToContents();
  ui.units_widget->resizeColumnsToContents();

  update_waiting();
}

/**
 * Function to load the units waiting table
 */
void units_view::update_waiting()
{
  int units_count = 0;

  ui.uwt_widget->clearContents();

  if (!client_has_player()) {
    return;
  }

  unit_list_iterate(client_player()->units, punit)
  {
    if (!can_unit_move_now(punit) && punit->ssa_controller == SSA_NONE) {
      QTableWidgetItem *item =
          new QTableWidgetItem(utype_name_translation(punit->utype));
      item->setData(Qt::UserRole,
                    QVariant::fromValue(static_cast<void *>(punit)));
      ui.uwt_widget->setItem(units_count, 0, item);

      int pcity_near_dist;
      struct city *pcity_near = get_nearest_city(punit, &pcity_near_dist);
      ui.uwt_widget->setItem(
          units_count, 1,
          new QTableWidgetItem(
              get_nearest_city_text(pcity_near, pcity_near_dist),
              pcity_near_dist));

      ui.uwt_widget->setItem(
          units_count, 2,
          new QTableWidgetItem(move_points_text(punit->moves_left, false),
                               punit->moves_left));

      time_t dt = time(nullptr) - punit->action_timestamp;
      if (dt < 0 && !can_unit_move_now(punit)) {
        char buf[64];
        format_time_duration(-dt, buf, sizeof(buf));
        ui.uwt_widget->setItem(units_count, 3,
                               new QTableWidgetItem(buf, dt));
      }

      ++units_count;
    }
  }
  unit_list_iterate_end;

  ui.uwt_widget->setRowCount(units_count);
  ui.uwt_widget->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);

  // Hide the label and widget if there is no units affected by wait time
  if (units_count == 0) {
    ui.uwt_label->setHidden(true);
    ui.uwt_widget->setHidden(true);
  }
}

/**
 * Action for selection changed in units view
 */
void units_view::selection_changed(const QItemSelection &sl,
                                   const QItemSelection &ds)
{
  QTableWidgetItem *itm;
  QVariant qvar;
  struct universal selected;
  QString upg;
  // const struct unit_view_entry *pentry;

  ui.upg_but->setDisabled(true);
  ui.disband_but->setDisabled(true);
  ui.find_but->setDisabled(true);

  if (sl.isEmpty()) {
    return;
  }

  curr_row = sl.indexes().at(0).row();
  if (curr_row >= 0 && curr_row <= max_row) {
    itm = ui.units_widget->item(curr_row, 0);
    qvar = itm->data(Qt::UserRole);
    uid = qvar.toInt();
    selected = cid_decode(uid);
    counter = ui.units_widget->item(curr_row, 3)->text().toInt();
    if (counter > 0) {
      ui.disband_but->setDisabled(false);
      ui.find_but->setDisabled(false);
    }
    upg = ui.units_widget->item(curr_row, 1)->text();
    if (upg != "-") {
      ui.upg_but->setDisabled(false);
    }
  }
}

/**
 * Disband pointed units (in units view)
 */
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
  ask->show();
}

/**
 * Find nearest unit
 */
void units_view::find_nearest()
{
  // go look at old fc gtk and see how its done there
}

/**
 * Upgrade Units
 */
void units_view::upgrade_units()
{
  QString b, c;
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  struct universal selected;
  int price;
  const struct unit_type *upgrade;
  const struct unit_type *utype;

  selected = cid_decode(uid);
  utype = selected.value.utype;

  Unit_type_id type = utype_number(utype);

  upgrade = can_upgrade_unittype(client_player(), utype);
  price = unit_upgrade_price(client_player(), utype, upgrade);
  qCritical() << upgrade << price;
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

/**
 * Update the units view.
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
 * Closes units view
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
