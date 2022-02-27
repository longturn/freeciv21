/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

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
#include "icons.h"
#include "mapview.h"
#include "page_game.h"
#include "sprite.h"

/**
   message_widget constructor
 */
message_widget::message_widget(QWidget *parent)
{
  setParent(parent);
  layout = new QGridLayout;
  layout->setMargin(0);
  setLayout(layout);

  auto title = new QLabel(_("Messages"));
  title->setAlignment(Qt::AlignCenter);
  layout->addWidget(title, 0, 1);
  layout->setColumnStretch(1, 100);

  mesg_table = new QListWidget;
  mesg_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mesg_table->setSelectionMode(QAbstractItemView::SingleSelection);
  mesg_table->setTextElideMode(Qt::ElideNone);
  mesg_table->setWordWrap(true);
  layout->addWidget(mesg_table, 1, 0, 1, 3);

  mw = new move_widget(this);
  layout->addWidget(mw, 0, 0, Qt::AlignLeft | Qt::AlignTop);

  min_max = new QPushButton(this);
  min_max->setIcon(fcIcons::instance()->getIcon("expand-up"));
  min_max->setIconSize(QSize(24, 24));
  min_max->setFixedWidth(25);
  min_max->setFixedHeight(25);
  min_max->setCheckable(true);
  min_max->setChecked(true);
  layout->addWidget(min_max, 0, 2);
  connect(mesg_table->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &message_widget::item_selected);
  connect(min_max, &QAbstractButton::toggled, this,
          &message_widget::set_events_visible);
  setMouseTracking(true);
}

/**
   Manages toggling minimization.
 */
void message_widget::set_events_visible(bool visible)
{
  mesg_table->setVisible(visible);

  int h = visible ? qRound(parentWidget()->size().height()
                           * king()->qt_settings.chat_fheight)
                  : sizeHint().height();

  // Heuristic that more or less works
  bool expand_up =
      (y() > parentWidget()->height() - y() - (visible ? h : height()));

  QString icon_name = (expand_up ^ visible) ? QLatin1String("expand-up")
                                            : QLatin1String("expand-down");
  min_max->setIcon(fcIcons::instance()->getIcon(icon_name));

  auto geo = geometry();
  if (expand_up) {
    geo.setTop(
        std::clamp(geo.bottom() - h, 0, parentWidget()->height() - h));
    geo.setHeight(h);
  } else {
    geo.setBottom(std::clamp(geo.top() + h, h, parentWidget()->height()));
  }
  setGeometry(geo);
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
  if (i > -1 && pmsg != NULL) {
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
  if (queen()->message == NULL) {
    return;
  }
  queen()->message->msg_update();
}
