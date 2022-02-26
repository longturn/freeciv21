/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "messagewin_g.h"
// qt-client is one true king
#include "widgetdecorations.h"

class QEvent;
class QGridLayout;
class QItemSelection;
class QListWidget;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;
class QPushButton;
class QPixmap;
class QResizeEvent;
class chatwdg;

/***************************************************************************
  Class representing message output
***************************************************************************/
class message_widget : public resizable_widget {
  Q_OBJECT

public:
  message_widget(QWidget *parent);
  void msg_update();
  void clr();
  void msg(const struct message *pmsg);

private:
  void update_menu() override {}
  QListWidget *mesg_table;
  QGridLayout *layout;

protected:
  void enterEvent(QEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
public slots:
  void item_selected(const QItemSelection &sl, const QItemSelection &ds);
  void set_events_visible(bool visible);

private:
  QPushButton *min_max;
  move_widget *mw;
  static void scroll_to_bottom(void *);
};
