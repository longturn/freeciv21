/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: Freeciv21 and Freeciv contributors
 * SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
 * SPDX-FileCopyrightText: Tobias Rehbein <tobias.rehbein@web.de>
 */

/**
 * \class units_view
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
#include "turn_done_button.h"
#include "views/view_map.h"
#include "views/view_map_common.h"

// common
#include "movement.h"
#include "text.h"
#include "unittype.h"

/****************************
 * units_view class functions
 ****************************/

/**
 * Constructor for units view
 */
units_view::units_view() : QWidget()
{
  ui.setupUi(this);

  QFont font = ui.units_table->horizontalHeader()->font();
  font.setWeight(QFont::Bold);
  ui.units_table->horizontalHeader()->setFont(font);
  ui.uwt_table->horizontalHeader()->setFont(font);

  // Configure the units table
  QStringList slist;
  slist << _("Type") << _("Name") << _("★  Upgradable")
        << _("⚒  In Progress") << _("⚔  Active") << _("Shield Upkeep")
        << _("Food Upkeep") << _("Gold Upkeep");
  ui.units_label->setText(QString(_("Units:")));
  ui.units_table->setColumnCount(slist.count());
  ui.units_table->setHorizontalHeaderLabels(slist);
  ui.units_table->setSortingEnabled(false);
  ui.units_table->setAlternatingRowColors(true);
  ui.upg_but->setText(_("Upgrade"));
  ui.upg_but->setToolTip(_("Upgrade selected unit."));
  ui.upg_but->setDisabled(true);
  ui.find_but->setText(_("Find Nearest"));
  ui.find_but->setToolTip(_("Center the map on the nearest unit in relation "
                            "to where the map is now."));
  ui.find_but->setDisabled(true);
  ui.disband_but->setText(_("Disband All"));
  ui.disband_but->setToolTip(_("Disband all of the selected unit."));
  ui.disband_but->setDisabled(true);

  // Configure the unitwaittime table
  slist.clear();
  slist << _("Type") << _("Name") << _("Location") << _("Time Left")
        << _("Id");
  ui.uwt_table->setColumnCount(slist.count());
  ui.uwt_table->setColumnHidden(4, true);
  ui.uwt_table->setHorizontalHeaderLabels(slist);
  ui.uwt_table->setSortingEnabled(true);
  ui.uwt_table->setAlternatingRowColors(true);
  ui.uwt_label->setText("Units Waiting:");

  // Configure the splitter
  // Configuring the splitter to distribute its children equally, is more
  // complicated than one might expect. We need to set the child widgets
  // sizes to the same value, using the QSplitters setSizes method. As QT
  // will still enforce the minimum size policy, we have to base this value
  // on the maximum minimum size of the children.
  auto equalHeight = std::max(ui.units_widget->minimumSizeHint().height(),
                              ui.uwt_widget->minimumSizeHint().height());
  ui.splitter->setSizes({equalHeight, equalHeight});

  // Add shield icon for shield upkeep column
  const QPixmap *spr =
      tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.shield",
                                  "citybar.shields", "", "", false);
  ui.units_table->horizontalHeaderItem(5)->setIcon(crop_sprite(spr));

  // Add food icon for food upkeep column
  spr = tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.food",
                                    "citybar.food", "", "", false);
  ui.units_table->horizontalHeaderItem(6)->setIcon(crop_sprite(spr));

  // Add gold icon for gold upkeep column
  spr = tiles_lookup_sprite_tag_alt(tileset, LOG_VERBOSE, "upkeep.gold",
                                    "citybar.trade", "", "", false);
  ui.units_table->horizontalHeaderItem(7)->setIcon(crop_sprite(spr));

  connect(ui.upg_but, &QAbstractButton::pressed, this,
          &units_view::upgrade_units);
  connect(ui.find_but, &QAbstractButton::pressed, this,
          &units_view::find_nearest);
  connect(ui.disband_but, &QAbstractButton::pressed, this,
          &units_view::disband_units);
  connect(ui.units_table->selectionModel(),
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
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &units_view::update_waiting);
  timer->start(1000);

  update_units();
  update_waiting();
}

/**
   Updates the units table
*/
void units_view::update_units()
{
  // used to set the height of the unit sprite
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  int h = fm.height() + 24;

  int entries_used = 0; // Position in the units array
  struct unit_view_entry unit_entries[U_LAST];

  get_units_view_data(unit_entries, &entries_used);

  // Variables for the nested loops
  int total_count = 0;  // Sum of unit type
  int upg_count = 0;    // Sum of upgradable
  int in_progress = 0;  // Sum of in progress by unit type
  int total_gold = 0;   // Sum of gold upkeep
  int total_shield = 0; // Sum of shield upkeep
  int total_food = 0;   // Sum of food upkeep
  int max_row = ui.units_table->rowCount(); // Max rows in the table widget

  // Remove "footer" rows from units table
  if (max_row >= 1) {
    max_row--;
    ui.units_table->removeRow(max_row);
  }

  // Keep tab of all unit types currently in the table. This is used to
  // update already existing rows in the table. After updating a unit type
  // the corresponding entry is removed from the map. Once all unit types,
  // used by the player, have been processed the remaining unit types can be
  // removed from the units table.
  std::map<cid, QTableWidgetItem *> unittypes_in_table;
  for (int r = 0; r < ui.units_table->rowCount(); r++) {
    QTableWidgetItem *item = ui.units_table->item(r, 0);

    bool ok = false;
    cid key = (cid) item->data(Qt::UserRole).toInt(&ok);
    fc_assert_action(ok == true, continue);
    unittypes_in_table[key] = ui.units_table->item(r, 0);
  }

  for (int i = 0; i < entries_used; i++) {
    struct unit_view_entry *pentry = unit_entries + i;
    const struct unit_type *putype = pentry->type;
    cid id = cid_encode_unit(putype);

    auto existing_row_for_unittype = unittypes_in_table.find(id);
    bool new_row = existing_row_for_unittype == unittypes_in_table.end();
    int current_row = max_row;
    if (new_row) {
      ui.units_table->insertRow(max_row);
    } else {
      current_row = existing_row_for_unittype->second->row();
    }

    for (int j = 0; j < 8; j++) {
      QTableWidgetItem *item = new QTableWidgetItem;
      switch (j) {
      case 0:
        // Unit type image sprite
        {
          auto sprite =
              get_unittype_sprite(tileset, putype, direction8_invalid())
                  ->scaledToHeight(h);
          QLabel *lbl = new QLabel;
          lbl->setPixmap(QPixmap(sprite));
          lbl->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
          ui.units_table->setCellWidget(current_row, j, lbl);
        }
        item->setData(Qt::UserRole, id);
        break;
      case 1:
        // Unit type name
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item->setText(utype_name_translation(putype));
        break;
      case 2:
        // Is Upgradable
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        if (pentry->upg) {
          item->setData(Qt::DisplayRole, "★");
          upg_count++;
        } else {
          item->setData(Qt::DisplayRole, "-");
        }
        break;
      case 3:
        // # In Progress
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->in_prod);
        in_progress += pentry->in_prod;
        break;
      case 4:
        // # Active
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->count);
        total_count += pentry->count;
        break;
      case 5:
        // Shield upkeep
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        if (pentry->shield_cost == 0) {
          item->setText("-");
        } else {
          item->setData(Qt::DisplayRole, pentry->shield_cost);
        }
        total_shield += pentry->shield_cost;
        break;
      case 6:
        // Food upkeep
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        if (pentry->food_cost == 0) {
          item->setText("-");
        } else {
          item->setData(Qt::DisplayRole, pentry->food_cost);
        }
        total_food += pentry->food_cost;
        break;
      case 7:
        // Gold upkeep
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        if (pentry->gold_cost == 0) {
          item->setText("-");
        } else {
          item->setData(Qt::DisplayRole, pentry->gold_cost);
        }
        total_gold += pentry->gold_cost;
        break;
      }
      ui.units_table->setItem(current_row, j, item);
    }

    if (new_row) {
      max_row++;
    } else {
      unittypes_in_table.erase(id);
    }
  }

  // Remove rows no longer needed in the table.
  for (const auto &[_, value] : unittypes_in_table) {
    ui.units_table->removeRow(value->row());
    max_row--;
  }

  // Add a "footer" to the table showing the totals.
  ui.units_table->setRowCount(max_row);
  ui.units_table->insertRow(max_row);
  for (int j = 0; j < 8; j++) {
    QTableWidgetItem *item_totals = new QTableWidgetItem;
    switch (j) {
    case 1:
      // Unit type name
      item_totals->setTextAlignment(Qt::AlignRight);
      item_totals->setText(_("--------------------\nTotals:"));
      break;
    case 2:
      // Upgradable
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(upg_count));
      break;
    case 3:
      // In Progress
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(in_progress));
      break;
    case 4:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_count));
      break;
    case 5:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_shield));
      break;
    case 6:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_food));
      break;
    case 7:
      item_totals->setTextAlignment(Qt::AlignCenter);
      item_totals->setText(QString(_("------\n%1")).arg(total_gold));
      break;
    }
    ui.units_table->setItem(max_row, j, item_totals);
  }

  if (max_row == 0) {
    ui.units_widget->setHidden(true);
  } else {
    ui.units_widget->setHidden(false);
    ui.units_table->resizeRowsToContents();
    ui.units_table->resizeColumnsToContents();
    ui.units_table->horizontalHeader()->resizeSections(
        QHeaderView::ResizeToContents);
    ui.units_table->verticalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
  }
}

/**
 * Function to load the units waiting table
 */
void units_view::update_waiting()
{
  // used to set the height of the unit sprite
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  int h = fm.height() + 24;

  struct unit_waiting_entry unit_entries[U_LAST];

  int entries_used = 0;
  get_units_waiting_data(unit_entries, &entries_used);

  max_row = ui.uwt_table->rowCount();

  // Remember all units initially in the table. This allows us to distinguish
  // between known units we need to update and new units we need to add to
  // the table. As we remove all updated units from this map, the remaining
  // items are the units we need to remove from the table.
  QMap<QString, QTableWidgetItem *> ids_in_table;
  for (int r = 0; r < max_row; r++) {
    QTableWidgetItem *item = ui.uwt_table->item(r, 4);
    ids_in_table[item->text()] = item;
  }

  for (int i = 0; i < entries_used; i++) {
    struct unit_waiting_entry *pentry = unit_entries + i;
    const struct unit_type *putype = pentry->type;
    cid id = cid_encode_unit(putype);
    QString unit_id = QString("%1").arg(pentry->id);
    QString unit_waittime = format_simple_duration(abs(pentry->timer));

    if (!ids_in_table.contains(unit_id)) {
      // Create a new row for the unit
      ui.uwt_table->insertRow(max_row);
      for (int j = 0; j < 5; j++) {
        auto item = new QTableWidgetItem;
        switch (j) {
        case 0:
          // Unit type image sprite
          {
            auto sprite =
                get_unittype_sprite(tileset, putype, direction8_invalid())
                    ->scaledToHeight(h);
            QLabel *lbl = new QLabel;
            lbl->setPixmap(QPixmap(sprite));
            lbl->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
            ui.uwt_table->setCellWidget(i, j, lbl);
          }
          item->setData(Qt::UserRole, id);
          break;
        case 1:
          // Unit type name
          item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
          item->setText(utype_name_translation(putype));
          break;
        case 2:
          // # Location
          item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
          item->setText(QString(_("%1")).arg(pentry->city_name));
          break;
        case 3:
          // # Time Left
          item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
          item->setText(QString(_("%1")).arg(unit_waittime));
          break;
        case 4:
          // # Id
          item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
          item->setText(QString("%1").arg(unit_id));
          break;
        }
        ui.uwt_table->setItem(max_row, j, item);
      }
      max_row++;
    } else {
      // Unit is already in table, update the corresponding row.
      int row = ids_in_table[unit_id]->row();
      ui.uwt_table->item(row, 3)->setText(unit_waittime);
      ids_in_table.remove(unit_id);
    }
  }

  // Delete units initially in the table, but not waiting anymore.
  for (int i = 0; i < ids_in_table.values().size(); i++) {
    int row = ids_in_table.values()[i]->row();
    ui.uwt_table->removeRow(row);
    max_row--;
  }

  if (max_row == 0) {
    ui.uwt_widget->setHidden(true);
  } else {
    ui.uwt_widget->setHidden(false);
    ui.uwt_table->setRowCount(max_row);
    ui.uwt_table->resizeRowsToContents();
    ui.uwt_table->resizeColumnsToContents();
    ui.uwt_table->horizontalHeader()->resizeSections(
        QHeaderView::ResizeToContents);
    ui.uwt_table->verticalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
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

  ui.upg_but->setDisabled(true);
  ui.disband_but->setDisabled(true);
  ui.find_but->setDisabled(true);

  if (sl.isEmpty()) {
    return;
  }

  curr_row = sl.indexes().at(0).row();
  max_row = ui.units_table->rowCount() - 1;
  if (curr_row >= 0 && curr_row <= max_row) {
    itm = ui.units_table->item(curr_row, 0);
    qvar = itm->data(Qt::UserRole);
    uid = qvar.toInt();
    selected = cid_decode(uid);
    counter = ui.units_table->item(curr_row, 4)->text().toInt();
    if (counter > 0) {
      ui.disband_but->setDisabled(false);
      ui.find_but->setDisabled(false);
    }
    upg = ui.units_table->item(curr_row, 2)->text();
    if (upg != "-") {
      ui.upg_but->setDisabled(false);
    }
    if (curr_row == max_row) {
      ui.upg_but->setDisabled(true);
    }
  }
}

/**
 * Disband selected units
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
 * Find nearest selected unit, closest units view when button is clicked.
 */
void units_view::find_nearest()
{
  struct universal selected;
  struct tile *ptile;
  struct unit *punit;
  const struct unit_type *utype;

  selected = cid_decode(uid);
  utype = selected.value.utype;

  ptile = get_center_tile_mapcanvas();
  if ((punit = find_nearest_unit(utype, ptile))) {
    queen()->mapview_wdg->center_on_tile(punit->tile);

    if (ACTIVITY_IDLE == punit->activity
        || ACTIVITY_SENTRY == punit->activity) {
      if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        unit_focus_set_and_select(punit);
      }
    }
  }
  popdown_units_view();
}

/**
 * Upgrade selected units.
 */
void units_view::upgrade_units()
{
  struct universal selected = cid_decode(uid);
  const struct unit_type *utype = selected.value.utype;
  Unit_type_id type = utype_number(utype);
  const struct unit_type *upgrade =
      can_upgrade_unittype(client_player(), utype);
  int price = unit_upgrade_price(client_player(), utype, upgrade);

  QString b = QString::asprintf(PL_("Treasury contains %d gold.",
                                    "Treasury contains %d gold.",
                                    client_player()->economic.gold),
                                client_player()->economic.gold);
  QString c = QString::asprintf(PL_("Upgrade as many %s to %s as possible "
                                    "for %d gold each?\n%s",
                                    "Upgrade as many %s to %s as possible "
                                    "for %d gold each?\n%s",
                                    price),
                                utype_name_translation(utype),
                                utype_name_translation(upgrade), price,
                                b.toUtf8().data());

  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  ask->set_text_title(c, _("Upgrade Obsolete Units?"));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Upgrade"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted,
          [=]() { dsend_packet_unit_type_upgrade(&client.conn, type); });
  ask->show();
}

/****************************************
 * Helper functions related to units view
 ****************************************/

/**
 * Function to help us find the nearest unit
 */
struct unit *find_nearest_unit(const struct unit_type *utype,
                               struct tile *ptile)
{
  struct unit *best_candidate = NULL;
  int best_dist = FC_INFINITY, dist;

  players_iterate(pplayer)
  {
    if (client_has_player() && pplayer != client_player()) {
      continue;
    }

    unit_list_iterate(pplayer->units, punit)
    {
      if (utype == unit_type_get(punit)
          && FOCUS_AVAIL == punit->client.focus_status
          && 0 < punit->moves_left && !punit->done_moving
          && punit->ssa_controller == SSA_NONE) {
        dist = sq_map_distance(unit_tile(punit), ptile);
        if (dist < best_dist) {
          best_candidate = punit;
          best_dist = dist;
        }
      }
    }
    unit_list_iterate_end;
  }
  players_iterate_end;

  return best_candidate;
}

/**
 * Returns an array of units data.
 */
void get_units_view_data(struct unit_view_entry *entries,
                         int *num_entries_used)
{
  *num_entries_used = 0;

  players_iterate(pplayer)
  {
    if (client_has_player() && pplayer != client_player()) {
      continue;
    }

    unit_type_iterate(unittype)
    {
      int count = 0;           // Count of active unit type
      int in_progress = 0;     // Count of being produced
      int gold_cost = 0;       // Gold upkeep
      int food_cost = 0;       // Food upkeep
      int shield_cost = 0;     // Shield upkeep
      bool upgradable = false; // Unit type is upgradable

      unit_list_iterate(pplayer->units, punit)
      {
        if (unit_type_get(punit) == unittype) {
          count++;
          upgradable =
              client_has_player()
              && nullptr != can_upgrade_unittype(client_player(), unittype);

          // Only units with a home city have upkeep
          if (punit->homecity != 0) {
            gold_cost += punit->upkeep[O_GOLD];
            food_cost += punit->upkeep[O_FOOD];
            shield_cost += punit->upkeep[O_SHIELD];
          }
        }
      }
      unit_list_iterate_end;

      city_list_iterate(pplayer->cities, pcity)
      {
        if (pcity->production.value.utype == unittype
            && pcity->production.kind == VUT_UTYPE) {
          in_progress++;
        }
      }
      city_list_iterate_end;

      // Skip unused unit types
      if (count == 0 && in_progress == 0) {
        continue;
      }

      entries[*num_entries_used].type = unittype;
      entries[*num_entries_used].count = count;
      entries[*num_entries_used].in_prod = in_progress;
      entries[*num_entries_used].upg = upgradable;
      entries[*num_entries_used].gold_cost = gold_cost;
      entries[*num_entries_used].food_cost = food_cost;
      entries[*num_entries_used].shield_cost = shield_cost;
      (*num_entries_used)++;
    }
    unit_type_iterate_end;
  }
  players_iterate_end;

  std::sort(entries, entries + *num_entries_used,
            [](const auto &lhs, const auto &rhs) {
              return QString::localeAwareCompare(
                         utype_name_translation(lhs.type),
                         utype_name_translation(rhs.type))
                     < 0;
            });
}

/**
 * Returns an array of units subject to unitwaittime.
 */
void get_units_waiting_data(struct unit_waiting_entry *entries,
                            int *num_entries_used)
{
  *num_entries_used = 0;

  if (nullptr == client.conn.playing) {
    return;
  }

  int pcity_near_dist = 0; // Init distance

  unit_list_iterate(client.conn.playing->units, punit)
  {
    struct city *pcity_near = get_nearest_city(punit, &pcity_near_dist);

    if (!can_unit_move_now(punit) && punit->ssa_controller == SSA_NONE) {

      entries[*num_entries_used].type = punit->utype;
      entries[*num_entries_used].city_name =
          get_nearest_city_text(pcity_near, pcity_near_dist);
      entries[*num_entries_used].timer =
          time(nullptr) - punit->action_timestamp;
      entries[*num_entries_used].id = punit->id;

      (*num_entries_used)++;
    }

    // Skip unused unit types
    if (*num_entries_used == 0) {
      continue;
    }
  }
  unit_list_iterate_end;

  std::sort(entries, entries + *num_entries_used,
            [](const auto &lhs, const auto &rhs) {
              return QString::localeAwareCompare(
                         utype_name_translation(lhs.type),
                         utype_name_translation(rhs.type))
                     < 0;
            });
}

/************************************
 * Functions for connecting to the UI
 ************************************/

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
    uv->init();
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
