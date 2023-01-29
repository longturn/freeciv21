/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "messagewin.h"

// Qt
#include <QApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

// client
#include "client_main.h"
#include "messagewin_common.h"
#include "update_queue.h"

// gui-qt
#include "fc_client.h"
#include "page_game.h"
#include "tileset/sprite.h"
#include "views/view_map.h"

/**
   message_widget constructor
 */
message_widget::message_widget(QWidget *parent)
{
  setParent(parent);
  setMinimumSize(200, 100);
  layout = new QGridLayout;
  layout->setContentsMargins(2, 2, 2, 2);
  setLayout(layout);
  setResizable(Qt::LeftEdge | Qt::BottomEdge);

  auto title = new QLabel(_("Messages"));
  title->setAlignment(Qt::AlignCenter);
  title->setMouseTracking(true);
  layout->addWidget(title, 0, 1);
  layout->setColumnStretch(1, 100);

  mesg_table = new QListWidget;
  mesg_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mesg_table->setSelectionMode(QAbstractItemView::SingleSelection);
  mesg_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  mesg_table->setTextElideMode(Qt::ElideNone);
  mesg_table->setWordWrap(true);
  layout->addWidget(mesg_table, 1, 0, 1, 3);

  connect(mesg_table->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &message_widget::item_selected);
  setMouseTracking(true);
}

/**
   Slot executed when selection on meg_table has changed
 */
void message_widget::item_selected(const QItemSelection &sl,
                                   const QItemSelection &ds)
{
  const struct message *pmsg;
  int i, j;
  QFont f;
  QModelIndex index;
  QModelIndexList indexes = sl.indexes();
  QListWidgetItem *item;

  if (indexes.isEmpty()) {
    return;
  }
  index = indexes.at(0);
  i = index.row();
  pmsg = meswin_get_message(i);
  if (i > -1 && pmsg != nullptr) {
    if (QApplication::mouseButtons() == Qt::LeftButton
        || QApplication::mouseButtons() == Qt::RightButton) {
      meswin_set_visited_state(i, true);
      item = mesg_table->item(i);
      f = item->font();
      f.setItalic(true);
      item->setFont(f);
    }
    if (QApplication::mouseButtons() == Qt::LeftButton
        && pmsg->location_ok) {
      meswin_goto(i);
    }
    if (QApplication::mouseButtons() == Qt::RightButton && pmsg->city_ok) {
      meswin_popup_city(i);
    }
    if (QApplication::mouseButtons() == Qt::RightButton
        && pmsg->event == E_DIPLOMACY) {
      j = queen()->gimmeIndexOf(QStringLiteral("DDI"));
      queen()->game_tab_widget->setCurrentIndex(j);
    }
  }
  mesg_table->clearSelection();
}

/**
   Mouse entered message_widget
 */
void message_widget::enterEvent(QEvent *event)
{
  setCursor(Qt::ArrowCursor);
}

/**
   Mouse left message_widget
 */
void message_widget::leaveEvent(QEvent *event)
{
  setCursor(Qt::ArrowCursor);
}

/**
   Paint event for message_widget
 */
void message_widget::paintEvent(QPaintEvent *event)
{
  QStyleOption opt;
  opt.init(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

/**
   Clears and removes mesg_table all items
 */
void message_widget::clr() { mesg_table->clear(); }

/**
   Adds news message to mesg_table
 */
void message_widget::msg(const struct message *pmsg)
{
  auto item = new QListWidgetItem;
  item->setText(pmsg->descr);
  mesg_table->addItem(item);
  if (pmsg->visited) {
    auto f = item->font();
    f.setItalic(true);
    item->setFont(f);
  }
  auto icon = get_event_sprite(tileset, pmsg->event);
  if (icon != nullptr) {
    item->setIcon(QIcon(*icon));
  }
  emit add_msg();
}

/**
   Updates mesg_table painting
 */
void message_widget::msg_update()
{
  const auto num = meswin_get_num_messages();
  if (num < mesg_table->count()) {
    // Fewer messages than before... no way to know which were removed, clear
    // all and start from scratch.
    mesg_table->clear();
  }
  for (int i = mesg_table->count(); i < num; i++) {
    msg(meswin_get_message(i));
  }

  // Scroll down to make sure the latest message is visible.
  if (client.conn.client.request_id_of_currently_handled_packet == 0) {
    mesg_table->scrollToBottom();
  } else {
    // Scroll only once to avoid laying out text repeately.
    update_queue::uq()->connect_processing_finished_unique(
        client.conn.client.request_id_of_currently_handled_packet,
        scroll_to_bottom, (void *) this);
  }
}

/*
 * Callback used to makes sure that the lastest message is visible.
 */
void message_widget::scroll_to_bottom(void *self)
{
  static_cast<message_widget *>(self)->mesg_table->scrollToBottom();
}

/**
   Do the work of updating (populating) the message dialog.
 */
void real_meswin_dialog_update(void *unused)
{
  if (queen()->message == nullptr) {
    return;
  }
  queen()->message->msg_update();
}
