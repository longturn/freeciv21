/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "messageoptions.h"
// Qt
#include <QApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
// utility
#include "log.h"
// client
#include "options.h"
// gui-qt
#include "fc_client.h"
#include "page_game.h"

extern QApplication *qapp;
/**********************************************************************/ /**
   Message widget constructor
 **************************************************************************/
message_dlg::message_dlg()
{
  int index;
  QStringList slist;
  QLabel *empty1, *empty2;
  QPushButton *but1;
  QPushButton *but2;
  QMargins margins;
  int len;

  setAttribute(Qt::WA_DeleteOnClose);
  empty1 = new QLabel;
  empty2 = new QLabel;
  layout = new QGridLayout;
  msgtab = new QTableWidget;
  slist << _("Event") << _("Out") << _("Mes") << _("Pop");
  msgtab->setColumnCount(slist.count());
  msgtab->setHorizontalHeaderLabels(slist);
  msgtab->setProperty("showGrid", "false");
  msgtab->setEditTriggers(QAbstractItemView::NoEditTriggers);
  msgtab->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
  msgtab->verticalHeader()->setVisible(false);
  msgtab->setSelectionMode(QAbstractItemView::SingleSelection);
  msgtab->setSelectionBehavior(QAbstractItemView::SelectRows);
  msgtab->setAlternatingRowColors(true);

  but1 = new QPushButton(
      style()->standardIcon(QStyle::SP_DialogCancelButton), _("Cancel"));
  connect(but1, &QAbstractButton::clicked, this,
          &message_dlg::cancel_changes);
  layout->addWidget(but1, 1, 1, 1, 1);
  but2 = new QPushButton(style()->standardIcon(QStyle::SP_DialogOkButton),
                         _("Ok"));
  connect(but2, &QAbstractButton::clicked, this,
          &message_dlg::apply_changes);
  layout->addWidget(but2, 1, 2, 1, 1, Qt::AlignRight);
  layout->addWidget(empty1, 0, 0, 1, 1);
  layout->addWidget(msgtab, 0, 1, 1, 2);
  layout->addWidget(empty2, 0, 3, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 10);
  layout->setColumnStretch(3, 1);
  setLayout(layout);
  queen()->gimmePlace(this, QStringLiteral("MSD"));
  index = queen()->addGameTab(this);
  queen()->game_tab_widget->setCurrentIndex(index);

  fill_data();
  margins = msgtab->contentsMargins();
  len = msgtab->horizontalHeader()->length() + margins.left()
        + margins.right()
        + qapp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  msgtab->setFixedWidth(len);
  msgtab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  but1->setFixedWidth(len / 3);
  but2->setFixedWidth(len / 3);
}

/**********************************************************************/ /**
   Message widget destructor
 **************************************************************************/
message_dlg::~message_dlg()
{
  queen()->removeRepoDlg(QStringLiteral("MSD"));
}

/**********************************************************************/ /**
   Fills column in table
 **************************************************************************/
void message_dlg::fill_data()
{
  int i, j;
  QTableWidgetItem *item;
  i = 0;
  msgtab->setRowCount(0);

  sorted_event_iterate(ev)
  {
    item = new QTableWidgetItem;
    item->setText(get_event_message_text(ev));
    msgtab->insertRow(i);
    msgtab->setItem(i, 0, item);
    for (j = 0; j < NUM_MW; j++) {
      bool checked;
      item = new QTableWidgetItem;
      checked = messages_where[ev] & (1 << j);
      if (checked) {
        item->setCheckState(Qt::Checked);
      } else {
        item->setCheckState(Qt::Unchecked);
      }
      msgtab->setItem(i, j + 1, item);
    }
    i++;
  }
  sorted_event_iterate_end;
  msgtab->resizeColumnsToContents();
}

/**********************************************************************/ /**
   Apply changes and closes widget
 **************************************************************************/
void message_dlg::apply_changes()
{
  int i, j;
  QTableWidgetItem *item;
  Qt::CheckState state;
  for (i = 0; i <= event_type_max(); i++) {
    /* Include possible undefined messages. */
    messages_where[i] = 0;
  }
  i = 0;
  sorted_event_iterate(ev)
  {
    for (j = 0; j < NUM_MW; j++) {
      bool checked;
      item = msgtab->item(i, j + 1);
      checked = messages_where[ev] & (1 << j);
      state = item->checkState();
      if ((state == Qt::Checked && !checked)
          || (state == Qt::Unchecked && checked)) {
        messages_where[ev] |= (1 << j);
      }
    }
    i++;
  }
  sorted_event_iterate_end;
  close();
}

/**********************************************************************/ /**
   Closes widget
 **************************************************************************/
void message_dlg::cancel_changes() { close(); }

/**********************************************************************/ /**
   Popup a window to let the user edit their message options.
 **************************************************************************/
void popup_messageopt_dialog(void)
{
  message_dlg *mdlg;
  int i;
  QWidget *w;

  if (!queen()->isRepoDlgOpen(QStringLiteral("MSD"))) {
    mdlg = new message_dlg;
  } else {
    i = queen()->gimmeIndexOf(QStringLiteral("MSD"));
    fc_assert(i != -1);
    if (queen()->game_tab_widget->currentIndex() == i) {
      return;
    }
    w = queen()->game_tab_widget->widget(i);
    mdlg = reinterpret_cast<message_dlg *>(w);
    queen()->game_tab_widget->setCurrentWidget(mdlg);
  }
}
