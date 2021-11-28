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

// client
#include "client_main.h"
#include "messagewin_common.h"
#include "update_queue.h"

// gui-qt
#include "fc_client.h"
#include "mapview.h"
#include "page_game.h"
#include "sprite.h"

/**
   info_tab constructor
 */
info_tab::info_tab(QWidget *parent)
{
  setParent(parent);

  layout = new QGridLayout;
  msgwdg = new messagewdg(this);
  layout->addWidget(msgwdg, 0, 0);
  chtwdg = new chatwdg(this);
  chtwdg->setProperty("messagewindow", true);
  msgwdg->setProperty("messagewindow", true);
  layout->addWidget(chtwdg, 1, 0);
  layout->setHorizontalSpacing(0);
  layout->setVerticalSpacing(0);
  layout->setContentsMargins(0, 3, 3, 0);
  layout->setSpacing(0);
  layout->setVerticalSpacing(0);
  setLayout(layout);
  resize_mode = false;
  resx = false;
  resy = false;
  resxy = false;
  mw = new move_widget(this);
  mw->put_to_corner();
  mw->setFixedSize(13, 13);
  setMouseTracking(true);
  chat_maximized = false;
}

/**
   Sets chat to default size of 3 lines
 */
void info_tab::restore_chat()
{
  msgwdg->setFixedHeight(qMax(0, (height() - chtwdg->default_size(3))));
  chtwdg->setFixedHeight(chtwdg->default_size(3));
  chat_maximized = false;
  chtwdg->scroll_to_bottom();
}

/**
   Maximizes size of chat
 */
void info_tab::maximize_chat()
{
  msgwdg->setFixedHeight(0);
  chtwdg->setFixedHeight(height());
  chat_maximized = true;
  chtwdg->scroll_to_bottom();
}

/**
   Checks if info_tab can be moved
 */
void info_tab::mousePressEvent(QMouseEvent *event)
{
  if (king()->interface_locked) {
    return;
  }
  if (event->button() == Qt::LeftButton) {
    cursor = event->globalPos() - geometry().topLeft();
    if (event->y() > 0 && event->y() < 25 && event->x() > width() - 25
        && event->x() < width()) {
      resize_mode = true;
      resxy = true;
      return;
    }
    if (event->y() > 0 && event->y() < 5) {
      resize_mode = true;
      resy = true;
    } else if (event->x() > width() - 5 && event->x() < width()) {
      resize_mode = true;
      resx = true;
    }
  }
  event->setAccepted(true);
}

/**
   Restores cursor when resizing is done
 */
void info_tab::mouseReleaseEvent(QMouseEvent *event)
{
  QPoint p;
  if (king()->interface_locked) {
    return;
  }
  if (resize_mode) {
    resize_mode = false;
    resx = false;
    resy = false;
    resxy = false;
    setCursor(Qt::ArrowCursor);
  }
  p = pos();
  king()->qt_settings.chat_fwidth =
      static_cast<float>(width()) / queen()->mapview_wdg->width();
  king()->qt_settings.chat_fheight =
      static_cast<float>(height()) / queen()->mapview_wdg->height();
  king()->qt_settings.chat_fx_pos =
      static_cast<float>(p.x()) / queen()->mapview_wdg->width();
  king()->qt_settings.chat_fy_pos =
      static_cast<float>(p.y()) / queen()->mapview_wdg->height();
}

/**
   Called when mouse moved (mouse track is enabled).
   Used for resizing info_tab.
 */
void info_tab::mouseMoveEvent(QMouseEvent *event)
{
  if (king()->interface_locked) {
    return;
  }
  if ((event->buttons() & Qt::LeftButton) && resize_mode && resy) {
    QPoint to_move;
    int newheight = event->globalY() - cursor.y() - geometry().y();
    resize(width(), this->geometry().height() - newheight);
    to_move = event->globalPos() - cursor;
    move(this->x(), to_move.y());
    setCursor(Qt::SizeVerCursor);
    restore_chat();
  } else if (event->x() > width() - 9 && event->y() > 0 && event->y() < 9) {
    setCursor(Qt::SizeBDiagCursor);
  } else if ((event->buttons() & Qt::LeftButton) && resize_mode && resx) {
    resize(event->x(), height());
    setCursor(Qt::SizeHorCursor);
  } else if (event->x() > width() - 5 && event->x() < width()) {
    setCursor(Qt::SizeHorCursor);
  } else if (event->y() > 0 && event->y() < 5) {
    setCursor(Qt::SizeVerCursor);
  } else if (resxy && (event->buttons() & Qt::LeftButton)) {
    QPoint to_move;
    int newheight = event->globalY() - cursor.y() - geometry().y();
    resize(event->x(), this->geometry().height() - newheight);
    to_move = event->globalPos() - cursor;
    move(this->x(), to_move.y());
    setCursor(Qt::SizeBDiagCursor);
    restore_chat();
  } else {
    setCursor(Qt::ArrowCursor);
  }
  event->setAccepted(true);
}

/**
   Inherited from abstract parent, does nothing here
 */
void info_tab::update_menu() {}

/**
   Messagewdg constructor
 */
messagewdg::messagewdg(QWidget *parent) : QWidget(parent)
{
  QPalette palette;
  layout = new QGridLayout;

  mesg_table = new QListWidget;
  mesg_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  mesg_table->setSelectionMode(QAbstractItemView::SingleSelection);
  mesg_table->setTextElideMode(Qt::ElideNone);
  mesg_table->setWordWrap(true);
  layout->addWidget(mesg_table, 0, 2, 1, 1);
  layout->setContentsMargins(0, 0, 3, 3);
  setLayout(layout);

  /* dont highlight show current cell - set the same colors*/
  palette.setColor(QPalette::Highlight, QColor(0, 0, 0, 0));
  palette.setColor(QPalette::HighlightedText, QColor(205, 206, 173));
  palette.setColor(QPalette::Text, QColor(205, 206, 173));
  mesg_table->setPalette(palette);
  connect(mesg_table->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &messagewdg::item_selected);
  setMouseTracking(true);
}

/**
   Slot executed when selection on meg_table has changed
 */
void messagewdg::item_selected(const QItemSelection &sl,
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
   Mouse entered messagewdg
 */
void messagewdg::enterEvent(QEvent *event) { setCursor(Qt::ArrowCursor); }

/**
   Mouse left messagewdg
 */
void messagewdg::leaveEvent(QEvent *event) { setCursor(Qt::ArrowCursor); }

/**
   Paints semi-transparent background
 */
void messagewdg::paint(QPainter *painter, QPaintEvent *event)
{
  painter->setBrush(QColor(0, 0, 0, 35));
  painter->drawRect(0, 0, width(), height());
}

/**
   Paint event for messagewdg
 */
void messagewdg::paintEvent(QPaintEvent *event)
{
  QPainter painter;

  painter.begin(this);
  paint(&painter, event);
  painter.end();
}

/**
   Clears and removes mesg_table all items
 */
void messagewdg::clr() { mesg_table->clear(); }

/**
   Adds news message to mesg_table
 */
void messagewdg::msg(const struct message *pmsg)
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
void messagewdg::msg_update()
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
void messagewdg::scroll_to_bottom(void *self)
{
  static_cast<messagewdg *>(self)->mesg_table->scrollToBottom();
}

/**
   Resize event for messagewdg
 */
void messagewdg::resizeEvent(QResizeEvent *event)
{
  mesg_table->scrollToBottom();
}

/**
   Do the work of updating (populating) the message dialog.
 */
void real_meswin_dialog_update(void *unused)
{
  if (queen()->infotab == NULL) {
    return;
  }
  queen()->infotab->msgwdg->msg_update();
}
