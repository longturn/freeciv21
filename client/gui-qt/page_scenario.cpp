/**************************************************************************
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
**************************************************************************/

#include "page_scenario.h"
// Qt
#include <QApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
// utility
#include "fcintl.h"
// common
#include "chatline_common.h"
#include "connectdlg_common.h"
#include "version.h"
// gui-qt
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"

/**********************************************************************/ /**
   Creates buttons and layouts for scenario page.
 **************************************************************************/
void fc_client::create_scenario_page()
{
  QPushButton *but;
  QHeaderView *header;
  QStringList sav;

  pages_layout[PAGE_SCENARIO] = new QGridLayout;
  scenarios_load = new QTableWidget;
  scenarios_view = new QTextEdit;
  scenarios_text = new QLabel;

  scenarios_view->setObjectName("scenarios_view");
  scenarios_text->setTextFormat(Qt::RichText);
  scenarios_text->setWordWrap(true);
  sav << _("Choose a Scenario");
  scenarios_load->setRowCount(0);
  scenarios_load->setColumnCount(sav.count());
  scenarios_load->setHorizontalHeaderLabels(sav);
  scenarios_load->setProperty("showGrid", "false");
  scenarios_load->setProperty("selectionBehavior", "SelectRows");
  scenarios_load->setEditTriggers(QAbstractItemView::NoEditTriggers);
  scenarios_load->setSelectionMode(QAbstractItemView::SingleSelection);
  scenarios_load->verticalHeader()->setVisible(false);
  pages_layout[PAGE_SCENARIO]->addWidget(scenarios_load, 0, 0, 3, 3,
                                         Qt::AlignLeft);
  pages_layout[PAGE_SCENARIO]->addWidget(scenarios_view, 1, 3, 2, 3);
  pages_layout[PAGE_SCENARIO]->addWidget(scenarios_text, 0, 3, 1, 2,
                                         Qt::AlignTop);
  scenarios_view->setReadOnly(true);
  scenarios_view->setWordWrapMode(QTextOption::WordWrap);
  scenarios_text->setAlignment(Qt::AlignCenter);

  header = scenarios_load->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);
  connect(scenarios_load->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &fc_client::slot_selection_changed);

  but = new QPushButton;
  but->setText(_("Browse..."));
  but->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  connect(but, &QAbstractButton::clicked, this,
          &fc_client::browse_scenarios);
  pages_layout[PAGE_SCENARIO]->addWidget(but, 4, 0);

  but = new QPushButton;
  but->setText(_("Cancel"));
  but->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
  connect(but, &QAbstractButton::clicked, this, &fc_client::slot_disconnect);
  pages_layout[PAGE_SCENARIO]->addWidget(but, 4, 3);

  pages_layout[PAGE_SCENARIO]->setColumnStretch(2, 10);
  pages_layout[PAGE_SCENARIO]->setColumnStretch(4, 20);
  pages_layout[PAGE_SCENARIO]->setColumnStretch(3, 20);
  pages_layout[PAGE_SCENARIO]->setRowStretch(1, 5);
  but = new QPushButton;
  but->setText(_("Load Scenario"));
  but->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
  connect(but, &QAbstractButton::clicked, this, &fc_client::start_scenario);
  pages_layout[PAGE_SCENARIO]->addWidget(but, 4, 4);
}

/**********************************************************************/ /**
   Browse scenarios directory
 **************************************************************************/
void fc_client::browse_scenarios(void)
{
  QString str;

  str = QString(_("Scenarios Files"))
        + QString(" (*.sav *.sav.bz2 *.sav.gz *.sav.xz)");
  current_file = QFileDialog::getOpenFileName(
      gui()->central_wdg, _("Open Scenario File"), QDir::homePath(), str);
  if (!current_file.isEmpty()) {
    start_scenario();
  }
}

/**********************************************************************/ /**
   Starts game from chosen scenario - chosen_file (save or scenario)
 **************************************************************************/
void fc_client::start_scenario()
{
  if (!is_server_running()) {
    client_start_server();
    send_chat("/detach");
  }
  if (is_server_running() && !current_file.isEmpty()) {
    QByteArray c_bytes;

    c_bytes = current_file.toLocal8Bit();
    send_chat_printf("/load %s", c_bytes.data());
    switch_page(PAGE_GAME + 1);
  }
}

/**********************************************************************/ /**
   Gets scenarios list and updates it in TableWidget = scenarios_load
 **************************************************************************/
void fc_client::update_scenarios_page(void)
{
  struct fileinfo_list *files;
  int row = 0;

  scenarios_load->clearContents();
  scenarios_load->setRowCount(0);
  scenarios_text->setText("");
  scenarios_view->setText("");

  files = fileinfolist_infix(get_scenario_dirs(), ".sav", false);
  fileinfo_list_iterate(files, pfile)
  {
    struct section_file *sf;

    if ((sf = secfile_load_section(pfile->fullname, "scenario", TRUE))
        && secfile_lookup_bool_default(sf, TRUE, "scenario.is_scenario")) {

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
      fcver -=
          (fcver % 10000); /* Patch level does not affect compatibility */
      sname = secfile_lookup_str_default(sf, NULL, "scenario.name");
      sdescription =
          secfile_lookup_str_default(sf, NULL, "scenario.description");
      sauthors = secfile_lookup_str_default(sf, NULL, "scenario.authors");
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
          version = QString("%1.%2").arg(maj).arg(min);
        } else {
          /* TRANS: Unknown scenario format */
          version = QString(_("pre-2.6"));
        }

        rows = scenarios_load->rowCount();
        for (i = 0; i < rows; ++i) {
          if (scenarios_load->item(i, 0)
              && scenarios_load->item(i, 0)->text() == pfile->name) {
            found = true;
            item = scenarios_load->takeItem(i, 0);
            break;
          }
        }

        if (found) {
          sl = item->data(Qt::UserRole).toStringList();
          found_ver = sl.at(3).toInt();
          if (found_ver < fcver) {
            continue;
          }
          add_item = false;
        }
        if (add_item) {
          item = new QTableWidgetItem();
          scenarios_load->insertRow(row);
        }
        item->setText(QString(pfile->name));
        format = QString("<br>") + QString(_("Format:")) + " "
                 + version.toHtmlEscaped();
        if (sauthors) {
          st = QString("\n") + QString("<b>") + _("Authors: ")
               + QString("</b>") + QString(sauthors).toHtmlEscaped();
        } else {
          st = "";
        }
        sl << "<b>"
                  + QString(sname && strlen(sname) ? Q_(sname) : pfile->name)
                        .toHtmlEscaped()
                  + "</b>"
           << QString(pfile->fullname).toHtmlEscaped()
           << QString(NULL != sdescription && '\0' != sdescription[0]
                          ? Q_(sdescription)
                          : "")
                      .toHtmlEscaped()
                  + st + format
           << QString::number(fcver).toHtmlEscaped();
        sl.replaceInStrings("\n", "<br>");
        item->setData(Qt::UserRole, sl);
        if (add_item) {
          scenarios_load->setItem(row, 0, item);
          row++;
        } else {
          scenarios_load->setItem(i, 0, item);
        }
      }
      secfile_destroy(sf);
    }
  }
  fileinfo_list_iterate_end;
  fileinfo_list_destroy(files);
  scenarios_load->sortItems(0);
  scenarios_load->update();
}
