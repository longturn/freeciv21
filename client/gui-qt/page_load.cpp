/**************************************************************************
  /\ ___ /\        Copyright (c) 1996-2020 ＦＲＥＥＣＩＶ ２１ and Freeciv
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

#include "page_load.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>

// utility
#include "fcintl.h"

// common
#include "connectdlg_common.h"
#include "version.h"

// gui-qt
#include "colors.h"
#include "dialogs.h"
#include "fc_client.h"


/**********************************************************************/ /**
   Creates buttons and layouts for load page.
 **************************************************************************/
void fc_client::create_load_page()
{
  pages_layout[PAGE_LOAD] = new QGridLayout;
  QPushButton *but;
  QHeaderView *header;
  QLabel *lbl_show_preview;
  QWidget *wdg;
  QHBoxLayout *hbox;

  saves_load = new QTableWidget;
  wdg = new QWidget;
  hbox = new QHBoxLayout;
  QStringList sav;
  lbl_show_preview = new QLabel(_("Show preview"));
  sav << _("Choose Saved Game to Load") << _("Date");
  load_pix = new QLabel;
  load_pix->setProperty("themed_border", true);
  load_pix->setFixedSize(0, 0);
  load_save_text = new QLabel;
  load_save_text->setTextFormat(Qt::RichText);
  load_save_text->setWordWrap(true);
  show_preview = new QCheckBox;
  show_preview->setChecked(gui_options.gui_qt_show_preview);
  saves_load->setAlternatingRowColors(true);
  saves_load->setRowCount(0);
  saves_load->setColumnCount(sav.count());
  saves_load->setHorizontalHeaderLabels(sav);
  hbox->addWidget(show_preview);
  hbox->addWidget(lbl_show_preview, Qt::AlignLeft);
  wdg->setLayout(hbox);

  saves_load->setProperty("showGrid", "false");
  saves_load->setProperty("selectionBehavior", "SelectRows");
  saves_load->setEditTriggers(QAbstractItemView::NoEditTriggers);
  saves_load->setSelectionMode(QAbstractItemView::SingleSelection);
  saves_load->verticalHeader()->setVisible(false);

  header = saves_load->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setStretchLastSection(true);

  pages_layout[PAGE_LOAD]->addWidget(saves_load, 0, 0, 1, 4);
  connect(saves_load->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &fc_client::slot_selection_changed);
  connect(show_preview, &QCheckBox::stateChanged, this,
          &fc_client::state_preview);
  pages_layout[PAGE_LOAD]->addWidget(wdg, 1, 0);
  pages_layout[PAGE_LOAD]->addWidget(load_save_text, 2, 0, 1, 2);
  pages_layout[PAGE_LOAD]->addWidget(load_pix, 2, 2, 1, 2);

  but = new QPushButton;
  but->setText(_("Browse..."));
  but->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  connect(but, &QAbstractButton::clicked, this, &fc_client::browse_saves);
  pages_layout[PAGE_LOAD]->addWidget(but, 3, 0);

  but = new QPushButton;
  but->setText(_("Cancel"));
  but->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
  connect(but, &QAbstractButton::clicked, this, &fc_client::slot_disconnect);
  pages_layout[PAGE_LOAD]->addWidget(but, 3, 2);

  but = new QPushButton;
  but->setText(_("Load"));
  but->setIcon(
      QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
  connect(but, &QAbstractButton::clicked, this, &fc_client::start_from_save);
  pages_layout[PAGE_LOAD]->addWidget(but, 3, 3);
  pages_layout[PAGE_LOAD]->setColumnStretch(3, 10);
  pages_layout[PAGE_LOAD]->setColumnStretch(2, 10);
  pages_layout[PAGE_LOAD]->setColumnStretch(0, 10);
}

/**********************************************************************/ /**
   Updates saves to load and updates in tableview = saves_load
 **************************************************************************/
void fc_client::update_load_page(void)
{
  struct fileinfo_list *files;
  int row;

  row = 0;
  files = fileinfolist_infix(get_save_dirs(), ".sav", FALSE);
  saves_load->clearContents();
  saves_load->setRowCount(0);
  show_preview->setChecked(gui_options.gui_qt_show_preview);
  fileinfo_list_iterate(files, pfile)
  {
    QTableWidgetItem *item;
    QDateTime dt;

    item = new QTableWidgetItem();
    item->setData(Qt::UserRole, pfile->fullname);
    saves_load->insertRow(row);
    item->setText(pfile->name);
    saves_load->setItem(row, 0, item);
    item = new QTableWidgetItem();
    dt = QDateTime::fromSecsSinceEpoch(pfile->mtime);
    item->setText(dt.toString(Qt::TextDate));
    saves_load->setItem(row, 1, item);
    row++;
  }
  fileinfo_list_iterate_end;
  fileinfo_list_destroy(files);
}

/**********************************************************************/ /**
   Starts game from chosen save - chosen_file (save or scenario)
 **************************************************************************/
void fc_client::start_from_save()
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
