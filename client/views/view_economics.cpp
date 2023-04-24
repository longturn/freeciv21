/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

/*
 * This file contains functions to generate the table based GUI for
 * the economics view (formally known as the economy report).
 */

#include "views/view_economics.h"
// client
#include "tileset/sprite.h"
// gui-qt
#include "fc_client.h"
#include "hudwidget.h"
#include "page_game.h"
#include "top_bar.h"

/**
   Constructor for economy report
 */
eco_report::eco_report() : QWidget()
{
  ui.setupUi(this);

  QFont font = ui.eco_widget->horizontalHeader()->font();
  font.setWeight(QFont::Bold);
  ui.eco_widget->horizontalHeader()->setFont(font);

  QStringList slist;
  slist << _("Type") << Q_("?Building or Unit type:Name") << _("Redundant")
        << _("Count") << _("Cost") << _("Total Upkeep");

  ui.eco_widget->setColumnCount(slist.count());
  ui.eco_widget->setHorizontalHeaderLabels(slist);
  ui.eco_widget->setAlternatingRowColors(true);
  ui.bdisband->setText(_("Disband"));
  ui.bsell->setText(_("Sell All"));
  ui.bredun->setText(_("Sell Redundant"));

  connect(ui.bdisband, &QAbstractButton::pressed, this,
          &eco_report::disband_units);
  connect(ui.bsell, &QAbstractButton::pressed, this,
          &eco_report::sell_buildings);
  connect(ui.bredun, &QAbstractButton::pressed, this,
          &eco_report::sell_redundant);
  connect(ui.eco_widget->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &eco_report::selection_changed);
  setLayout(ui.eco_layout);
  queen()->gimmePlace(this, QStringLiteral("ECO"));
  index = queen()->addGameTab(this);
}

/**
   Destructor for economy report
 */
eco_report::~eco_report() { queen()->removeRepoDlg(QStringLiteral("ECO")); }

/**
   Initializes place in tab for economy report
 */
void eco_report::init() { queen()->game_tab_widget->setCurrentIndex(index); }

/**
   Refresh all widgets for economy report
 */
void eco_report::update_report()
{
  struct improvement_entry building_entries[B_LAST];
  struct unit_entry unit_entries[U_LAST];
  int entries_used, building_total, unit_total, tax, i, j;
  QString buf;
  QTableWidgetItem *item;
  QFont f = QApplication::font();
  QFontMetrics fm(f);
  int h = fm.height() + 24;

  ui.eco_widget->setRowCount(0);
  ui.eco_widget->clearContents();
  get_economy_report_data(building_entries, &entries_used, &building_total,
                          &tax);
  for (i = 0; i < entries_used; i++) {
    struct improvement_entry *pentry = building_entries + i;
    struct impr_type *pimprove = pentry->type;
    cid id = cid_encode_building(pimprove);

    ui.eco_widget->insertRow(i);
    for (j = 0; j < 6; j++) {
      item = new QTableWidgetItem;
      switch (j) {
      case 0: {
        auto sprite =
            get_building_sprite(tileset, pimprove)->scaledToHeight(h);
        QLabel *lbl = new QLabel;
        lbl->setPixmap(QPixmap(sprite));
        lbl->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        ui.eco_widget->setCellWidget(i, j, lbl);
        item->setData(Qt::UserRole, id);
      } break;
      case 1:
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item->setText(improvement_name_translation(pimprove));
        break;
      case 2:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->redundant);
        break;
      case 3:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->count);
        break;
      case 4:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->cost);
        break;
      case 5:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->total_cost);
        break;
      }
      ui.eco_widget->setItem(i, j, item);
    }
  }
  max_row = i;
  get_economy_report_units_data(unit_entries, &entries_used, &unit_total);
  for (i = 0; i < entries_used; i++) {
    struct unit_entry *pentry = unit_entries + i;
    struct unit_type *putype = pentry->type;
    cid id = cid_encode_unit(putype);

    ui.eco_widget->insertRow(i + max_row);
    for (j = 0; j < 6; j++) {
      item = new QTableWidgetItem;
      switch (j) {
      case 0: {
        auto sprite =
            get_unittype_sprite(tileset, putype, direction8_invalid())
                ->scaledToHeight(h);
        QLabel *lbl = new QLabel;
        lbl->setPixmap(QPixmap(sprite));
        lbl->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        ui.eco_widget->setCellWidget(max_row + i, j, lbl);
      }
        item->setData(Qt::UserRole, id);
        break;
      case 1:
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item->setText(utype_name_translation(putype));
        break;
      case 2:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, 0);
        break;
      case 3:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->count);
        break;
      case 4:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->cost);
        break;
      case 5:
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        item->setData(Qt::DisplayRole, pentry->total_cost);
        break;
      }
      ui.eco_widget->setItem(max_row + i, j, item);
    }
  }
  max_row = max_row + i;
  buf = QString(_("Income: %1    Total Costs: %2"))
            .arg(QString::number(tax),
                 QString::number(building_total + unit_total));
  ui.eco_label->setText(buf);
  ui.eco_widget->resizeRowsToContents();
  ui.eco_widget->resizeColumnsToContents();
}

/**
   Action for selection changed in economy report
 */
void eco_report::selection_changed(const QItemSelection &sl,
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

/**
   Disband pointed units (in economy report)
 */
void eco_report::disband_units()
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
   Sell all pointed builings
 */
void eco_report::sell_buildings()
{
  struct universal selected;
  QString buf;
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  const struct impr_type *pimprove;
  Impr_type_id impr_id;

  selected = cid_decode(uid);
  pimprove = selected.value.building;
  impr_id = improvement_number(pimprove);

  buf = QString(_("Do you really wish to sell "
                  "every %1 (%2 total)?"))
            .arg(improvement_name_translation(pimprove),
                 QString::number(counter));

  ask->set_text_title(buf, _("Sell Improvements?"));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Sell"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, [=]() {
    char buf[1024];
    hud_message_box *result;
    struct impr_type *pimprove = improvement_by_number(impr_id);

    if (!pimprove) {
      return;
    }

    sell_all_improvements(pimprove, false, buf, sizeof(buf));

    result = new hud_message_box(king()->central_wdg);
    result->set_text_title(buf, _("Sell Results"));
    result->setStandardButtons(QMessageBox::Ok);
    result->setAttribute(Qt::WA_DeleteOnClose);
    result->show();
  });
  ask->show();
}

/**
   Sells redundant buildings
 */
void eco_report::sell_redundant()
{
  struct universal selected;
  QString s, buf;
  hud_message_box *ask = new hud_message_box(king()->central_wdg);
  const struct impr_type *pimprove;
  Impr_type_id impr_id;

  selected = cid_decode(uid);
  pimprove = selected.value.building;
  impr_id = improvement_number(pimprove);

  buf = QString::asprintf(_("Do you really wish to sell "
                            "every redundant %s (%d total)?"),
                          improvement_name_translation(pimprove), counter);

  ask->set_text_title(s, _("Sell Redundant Improvements?"));
  ask->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
  ask->setDefaultButton(QMessageBox::Cancel);
  ask->button(QMessageBox::Yes)->setText(_("Yes Sell"));
  ask->setAttribute(Qt::WA_DeleteOnClose);
  connect(ask, &hud_message_box::accepted, [=]() {
    char buf[1024];
    hud_message_box *result;
    struct impr_type *pimprove = improvement_by_number(impr_id);

    if (!pimprove) {
      return;
    }

    sell_all_improvements(pimprove, true, buf, sizeof(buf));

    result = new hud_message_box(king()->central_wdg);
    result->set_text_title(buf, _("Sell Redundant Results"));
    result->setStandardButtons(QMessageBox::Ok);
    result->setAttribute(Qt::WA_DeleteOnClose);
    result->show();
  });
  ask->show();
}

/**
   Update the economy report.
 */
void real_economy_report_dialog_update(void *unused)
{
  int i;
  eco_report *eco_rep;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("ECO"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("ECO"));
    if (queen()->game_tab_widget->currentIndex() == i) {
      w = queen()->game_tab_widget->widget(i);
      eco_rep = reinterpret_cast<eco_report *>(w);
      eco_rep->update_report();
    }
  }
  queen()->updateSidebarTooltips();
}

/**
 * Display the economy report. Typically triggered by F5.
 */
void economy_report_dialog_popup()
{
  int i;
  eco_report *eco_rep;
  QWidget *w;
  if (!queen()->isRepoDlgOpen(QStringLiteral("ECO"))) {
    eco_rep = new eco_report;
    eco_rep->init();
    eco_rep->update_report();
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("ECO"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    if (w->isVisible()) {
      top_bar_show_map();
      return;
    }
    eco_rep = reinterpret_cast<eco_report *>(w);
    eco_rep->update_report();
    queen()->game_tab_widget->setCurrentWidget(eco_rep);
  }
}

/**
   Closes economy report
 */
void popdown_economy_report()
{
  int i;
  eco_report *eco_rep;
  QWidget *w;

  if (queen()->isRepoDlgOpen(QStringLiteral("ECO"))) {
    i = queen()->gimmeIndexOf(QStringLiteral("ECO"));
    fc_assert(i != -1);
    w = queen()->game_tab_widget->widget(i);
    eco_rep = reinterpret_cast<eco_report *>(w);
    eco_rep->deleteLater();
  }
}
