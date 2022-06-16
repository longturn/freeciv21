/***    _     ************************************************************
      /` '\                 Copyright (c) 1996-2020 Freeciv21 and Freeciv
    /| @   l                 contributors. This file is part of Freeciv21.
    \|      \         Freeciv21 is free software: you can redistribute it
      `\     `\_              and/or modify it under the terms of the GNU
        \    __ `\       General Public License  as published by the Free
        l  \   `\ `\__       Software Foundation, either version 3 of the
         \  `\./`     ``\  License, or (at your option) any later version.
           \ ____ / \   l          You should have received a copy of the
             ||  ||  )  / GNU General Public License along with Freeciv21.
-----------(((-(((---l /-----------
                    l /        If not, see https://www.gnu.org/licenses/.
                   / /      ********************************************/

#include "widgetdecorations.h"
// Qt
#include <QMouseEvent>
#include <QPainter>
// gui-qt
#include "fc_client.h"
#include "icons.h"
#include "page_game.h"

/****************************************************************************
  Scale widget allowing scaling other widgets, shown in right top corner
****************************************************************************/
scale_widget::scale_widget(QRubberBand::Shape s, QWidget *p)
    : QRubberBand(s, p)
{
  QPixmap *pix;

  size = 12;
  pix = fcIcons::instance()->getPixmap(QStringLiteral("plus"));
  plus = pix->scaledToWidth(size);
  delete pix;
  pix = fcIcons::instance()->getPixmap(QStringLiteral("minus"));
  minus = plus = pix->scaledToWidth(size);
  delete pix;
  setFixedSize(2 * size, size);
  scale = 1.0f;
  setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

/****************************************************************************
  Draws 2 icons for resizing
****************************************************************************/
void scale_widget::paintEvent(QPaintEvent *event)
{
  QRubberBand::paintEvent(event);
  QPainter p;
  p.begin(this);
  p.drawPixmap(0, 0, minus);
  p.drawPixmap(size, 0, plus);
  p.end();
}

/****************************************************************************
  Mouse press event for scale widget
****************************************************************************/
void scale_widget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
    if (event->localPos().x() <= size) {
      scale = scale / 1.2;
    } else {
      scale = scale * 1.2;
    }
    parentWidget()->update();
  }
}

/**
   Constructor for move widget
 */
move_widget::move_widget(QWidget *parent) : QLabel()
{
  QPixmap pix;

  setParent(parent);
  setCursor(Qt::SizeAllCursor);
  pix = fcIcons::instance()->getIcon(QStringLiteral("move")).pixmap(32);
  pix.setDevicePixelRatio(2);
  setPixmap(pix);
  setFixedSize(16, 16);
}

/**
   Puts move widget to left top corner
 */
void move_widget::put_to_corner() { move(0, 0); }

/**
   Mouse handler for move widget (moves parent widget)
 */
void move_widget::mouseMoveEvent(QMouseEvent *event)
{
  if (!king()->interface_locked) {
    auto new_location = event->globalPos() - point;
    if (new_location.x() < 0) {
      new_location.setX(0);
    } else if (new_location.x() + width()
               > parentWidget()->parentWidget()->width()) {
      new_location.setX(parentWidget()->parentWidget()->width() - width());
    }
    if (new_location.y() < 0) {
      new_location.setY(0);
    } else if (new_location.y() + height()
               > parentWidget()->parentWidget()->height()) {
      new_location.setY(parentWidget()->parentWidget()->height() - height());
    }
    parentWidget()->move(new_location);
  }
}

/**
   Sets moving point for move widget;
 */
void move_widget::mousePressEvent(QMouseEvent *event)
{
  if (!king()->interface_locked) {
    point = event->globalPos() - parentWidget()->geometry().topLeft();
  }
  update();
}

/**
   Constructor for close widget
 */
close_widget::close_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::ArrowCursor);
  pix = fcIcons::instance()->getPixmap(QStringLiteral("close"));
  *pix = pix->scaledToHeight(12);
  setPixmap(*pix);
  delete pix;
}

/**
   Puts close widget to right top corner
 */
void close_widget::put_to_corner()
{
  move(parentWidget()->width() - width(), 0);
}

/**
   Mouse handler for close widget, hides parent widget
 */
void close_widget::mousePressEvent(QMouseEvent *event)
{
  if (king()->interface_locked) {
    return;
  }
  if (event->button() == Qt::LeftButton) {
    parentWidget()->hide();
    notify_parent();
  }
}

/**
   Notifies parent to do custom action, parent is already hidden.
 */
void close_widget::notify_parent()
{
  fcwidget *fcw;

  fcw = reinterpret_cast<fcwidget *>(parentWidget());
  fcw->update_menu();
}

/**
   Checks if info_tab can be moved
 */
void resizable_widget::mousePressEvent(QMouseEvent *event)
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
   Restores cursor when resizing is done.
 */
void resizable_widget::mouseReleaseEvent(QMouseEvent *event)
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
  emit resized(rect());
}
/**
   Called when mouse moved (mouse track is enabled).
   Used for resizing resizable_widget.
 */
void resizable_widget::mouseMoveEvent(QMouseEvent *event)
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
  } else {
    setCursor(Qt::ArrowCursor);
  }
  event->setAccepted(true);
}
