/*
  /\ ___ /\                 Copyright (c) 1996-2020 Freeciv21 and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                  If not, see https://www.gnu.org/licenses/.
 */

#include "page_scenario.h"
// Qt
#include <QFileDialog>
// utility
#include "fcintl.h"
// common
#include "chatline_common.h"
#include "connectdlg_common.h"
#include "version.h"
// client
#include "client_main.h"
// gui-qt
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"

page_scenario::page_scenario(QWidget *parent, fc_client *gui)
    : QWidget(parent)
{
  QHeaderView *header;
  QStringList sav;

  ui.setupUi(this);
  king = gui;
  ui.scenarios_view->setObjectName(QStringLiteral("scenarios_view"));
  ui.scenarios_text->setTextFormat(Qt::RichText);
  ui.scenarios_text->setWordWrap(true);
  sav << _("Choose a Scenario");
  ui.scenarios_load->setRowCount(0);
  ui.scenarios_load->setColumnCount(sav.count());
  ui.scenarios_load->setHorizontalHeaderLabels(sav);
  ui.scenarios_load->setProperty("showGrid", "false");
  ui.scenarios_load->setProperty("selectionBehavior", "SelectRows");
  ui.scenarios_load->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui.scenarios_load->setSelectionMode(QAbstractItemView::SingleSelection);
  ui.scenarios_load->verticalHeader()->setVisible(false);
  ui.scenarios_view->setReadOnly(true);
  ui.scenarios_view->setWordWrapMode(QTextOption::WordWrap);
  ui.scenarios_text->setAlignment(Qt::AlignCenter);

  header = ui.scenarios_load->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  connect(ui.scenarios_load->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &page_scenario::slot_selection_changed);

  ui.bbrowse->setText(_("Browse..."));
  ui.bbrowse->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  connect(ui.bbrowse, &QAbstractButton::clicked, this,
          &page_scenario::browse_scenarios);

  ui.bcancel->setText(_("Cancel"));
  ui.bcancel->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
  connect(ui.bcancel, &QAbstractButton::clicked, gui,
          &fc_client::slot_disconnect);
  ui.bload->setText(_("Load Scenario"));
  ui.bload->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
  connect(ui.bload, &QAbstractButton::clicked, this,
          &page_scenario::start_scenario);
  setLayout(ui.gridLayout);
}

page_scenario::~page_scenario() = default;

/**
   Browse scenarios directory
 */
void page_scenario::browse_scenarios()
{
  QString str;

  str = QString(_("Scenarios Files"))
        + QStringLiteral(" (*.sav *.sav.bz2 *.sav.gz *.sav.xz *.sav.zst)");
  current_file = QFileDialog::getOpenFileName(this, _("Open Scenario File"),
                                              QDir::homePath(), str);
  if (!current_file.isEmpty()) {
    start_scenario();
  }
}

/**
   Starts game from chosen scenario - chosen_file (save or scenario)
 */
void page_scenario::start_scenario()
{
  if (!is_server_running()) {
    client_start_server(client_url().userName());
    send_chat("/detach");
  }
  if (is_server_running() && !current_file.isEmpty()) {
    QByteArray c_bytes;

    c_bytes = current_file.toLocal8Bit();
    send_chat_printf("/load %s", c_bytes.data());
    king->switch_page(PAGE_LOADING);
  }
}

/**
   Gets scenarios list and updates it in TableWidget = scenarios_load
 */
void page_scenario::update_scenarios_page()
{
  int row = 0;

  ui.scenarios_load->clearContents();
  ui.scenarios_load->setRowCount(0);
  ui.scenarios_text->setText(QLatin1String(""));
  ui.scenarios_view->setText(QLatin1String(""));

  const auto files = find_files_in_path(get_scenario_dirs(),
                                        QStringLiteral("*.sav*"), false);
  for (const auto &info : files) {
    struct section_file *sf = secfile_load_section(
        info.absoluteFilePath(), QStringLiteral("scenario"), true);

    if (sf
        && secfile_lookup_bool_default(sf, true, "scenario.is_scenario")) {
      const char *sname, *sdescription, *sauthors;
      QTableWidgetItem *item;
      QString format;
      QString st;
      QStringList sl;
      int fcver;
      int current_ver = MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000;

      fcver = secfile_lookup_int_default(sf, 0, "scenario.game_version");
      if (fcver < 30000) {
        /* Pre-3.0 versions stored version number without emergency version
         * part in the end. To get comparable version number stored,
         * multiply by 100. */
        fcver *= 100;
      }
      fcver -= (fcver % 10000); // Patch level does not affect compatibility
      sname = secfile_lookup_str_default(sf, nullptr, "scenario.name");
      sdescription =
          secfile_lookup_str_default(sf, nullptr, "scenario.description");
      sauthors = secfile_lookup_str_default(sf, nullptr, "scenario.authors");
      if (fcver <= current_ver) {
        QString version;
        bool add_item = true;
        bool found = false;
        QStringList sl;
        int rows;
        int found_ver;
        int i;

        if (fcver > 0) {
          int maj;
          int min;

          maj = fcver / 1000000;
          fcver %= 1000000;
          min = fcver / 10000;
          version = QStringLiteral("%1.%2").arg(maj).arg(min);
        } else {
          // TRANS: Unknown scenario format
          version = QString(_("pre-2.6"));
        }

        rows = ui.scenarios_load->rowCount();
        for (i = 0; i < rows; ++i) {
          if (ui.scenarios_load->item(i, 0)
              && ui.scenarios_load->item(i, 0)->text() == info.baseName()) {
            found = true;
            item = ui.scenarios_load->takeItem(i, 0);
            break;
          }
        }

        if (found) {
          sl = item->data(Qt::UserRole).toStringList();
          found_ver = sl.at(3).toInt();
          if (found_ver < fcver) {
            secfile_destroy(sf);
            continue;
          }
          add_item = false;
        }
        if (add_item) {
          item = new QTableWidgetItem();
          ui.scenarios_load->insertRow(row);
        }
        item->setText(info.baseName());
        format = QStringLiteral("<br>") + QString(_("Format:")) + " "
                 + version.toHtmlEscaped();
        if (sauthors) {
          st = QStringLiteral("\n") + QStringLiteral("<b>") + _("Authors: ")
               + QStringLiteral("</b>") + QString(sauthors).toHtmlEscaped();
        } else {
          st = QLatin1String("");
        }
        sl << "<b>"
                  + QString(sname && qstrlen(sname) ? Q_(sname)
                                                    : info.baseName())
                        .toHtmlEscaped()
                  + "</b>"
           << info.absoluteFilePath()
           << QString(nullptr != sdescription && '\0' != sdescription[0]
                          ? Q_(sdescription)
                          : "")
                      .toHtmlEscaped()
                  + st + format
           << QString::number(fcver).toHtmlEscaped();
        sl.replaceInStrings(QStringLiteral("\n"), QStringLiteral("<br>"));
        item->setData(Qt::UserRole, sl);
        if (add_item) {
          ui.scenarios_load->setItem(row, 0, item);
          row++;
        } else {
          ui.scenarios_load->setItem(i, 0, item);
        }
      }
    }
    secfile_destroy(sf);
  }
  ui.scenarios_load->sortItems(0);
  ui.scenarios_load->update();
}

void page_scenario::slot_selection_changed(const QItemSelection &selected,
                                           const QItemSelection &deselected)
{
  Q_UNUSED(deselected)

  if (selected.isEmpty()) {
    return;
  }

  auto sl = selected.indexes().front().data(Qt::UserRole).toStringList();
  ui.scenarios_text->setText(sl.at(0));
  if (sl.count() > 1) {
    ui.scenarios_view->setText(sl.at(2));
    current_file = sl.at(1);
  }
}
