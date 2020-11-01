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
#include <QFileDialog>
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

page_scenario::page_scenario(QWidget *parent, fc_client *gui) : QWidget(parent)
{
  QHeaderView *header;
  QStringList sav;

  ui.setupUi(this);
  king = gui;
  ui.scenarios_view->setObjectName("scenarios_view");
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
  ui.bbrowse->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  connect(ui.bbrowse, &QAbstractButton::clicked, this,
          &page_scenario::browse_scenarios);

  ui.bcancel->setText(_("Cancel"));
  ui.bcancel->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
  connect(ui.bcancel, &QAbstractButton::clicked, gui, &fc_client::slot_disconnect);
  ui.bload->setText(_("Load Scenario"));
  ui.bload->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
  connect(ui.bload, &QAbstractButton::clicked, this, &page_scenario::start_scenario);
  setLayout(ui.gridLayout);
}

page_scenario::~page_scenario() {}

/**********************************************************************/ /**
   Browse scenarios directory
 **************************************************************************/
void page_scenario::browse_scenarios(void)
{
  QString str;

  str = QString(_("Scenarios Files"))
        + QString(" (*.sav *.sav.bz2 *.sav.gz *.sav.xz)");
  current_file = QFileDialog::getOpenFileName(
      this, _("Open Scenario File"), QDir::homePath(), str);
  if (!current_file.isEmpty()) {
    start_scenario();
  }
}

/**********************************************************************/ /**
   Starts game from chosen scenario - chosen_file (save or scenario)
 **************************************************************************/
void page_scenario::start_scenario()
{
  if (!is_server_running()) {
    client_start_server();
    send_chat("/detach");
  }
  if (is_server_running() && !current_file.isEmpty()) {
    QByteArray c_bytes;

    c_bytes = current_file.toLocal8Bit();
    send_chat_printf("/load %s", c_bytes.data());
    king->switch_page(PAGE_GAME + 1);
  }
}

/**********************************************************************/ /**
   Gets scenarios list and updates it in TableWidget = scenarios_load
 **************************************************************************/
void page_scenario::update_scenarios_page(void)
{
  struct fileinfo_list *files;
  int row = 0;

  ui.scenarios_load->clearContents();
  ui.scenarios_load->setRowCount(0);
  ui.scenarios_text->setText("");
  ui.scenarios_view->setText("");

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

        rows = ui.scenarios_load->rowCount();
        for (i = 0; i < rows; ++i) {
          if (ui.scenarios_load->item(i, 0)
              && ui.scenarios_load->item(i, 0)->text() == pfile->name) {
            found = true;
            item = ui.scenarios_load->takeItem(i, 0);
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
          ui.scenarios_load->insertRow(row);
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
          ui.scenarios_load->setItem(row, 0, item);
          row++;
        } else {
          ui.scenarios_load->setItem(i, 0, item);
        }
      }
      secfile_destroy(sf);
    }
  }
  fileinfo_list_iterate_end;
  fileinfo_list_destroy(files);
  ui.scenarios_load->sortItems(0);
  ui.scenarios_load->update();
}

void page_scenario::slot_selection_changed(const QItemSelection &selected,
                                          const QItemSelection &deselected)
{
  QModelIndexList indexes = selected.indexes();
  QStringList sl;
  QModelIndex index;
  QVariant qvar;
    index = indexes.at(0);
    qvar = index.data(Qt::UserRole);
    sl = qvar.toStringList();
    ui.scenarios_text->setText(sl.at(0));
    if (sl.count() > 1) {
      ui.scenarios_view->setText(sl.at(2));
      current_file = sl.at(1);
    }
}
