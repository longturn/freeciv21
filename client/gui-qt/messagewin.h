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
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;
class QPixmap;
class QResizeEvent;
class QTableWidget;
class chatwdg;

/***************************************************************************
  Class representing message output
***************************************************************************/
class messagewdg : public QWidget {
  Q_OBJECT
public:
  messagewdg(QWidget *parent);
  void msg_update();
  void clr();
  void msg(const struct message *pmsg);

private:
  QTableWidget *mesg_table;
  QGridLayout *layout;
  QPixmap *pix;

protected:
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);
public slots:
  void item_selected(const QItemSelection &sl, const QItemSelection &ds);
};

/***************************************************************************
  Class which manages chat and messages
***************************************************************************/
class info_tab : public fcwidget {
  Q_OBJECT
public:
  info_tab(QWidget *parent);
  void max_chat_size();
  QGridLayout *layout;
  messagewdg *msgwdg;
  chatwdg *chtwdg;
  void maximize_chat();
  void restore_chat();
  bool chat_maximized;

private:
  void update_menu();
  QPoint cursor;
  QSize last_size;
  move_widget *mw;
  bool resize_mode;
  bool resxy;
  bool resx;
  bool resy;

protected:
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  int &e_pos();
};
