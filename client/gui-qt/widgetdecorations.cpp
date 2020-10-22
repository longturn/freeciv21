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

/****************************************************************************
  Scale widget allowing scaling other widgets, shown in right top corner
****************************************************************************/
scale_widget::scale_widget(QRubberBand::Shape s, QWidget *p)
    : QRubberBand(s, p)
{
  QPixmap *pix;

  size = 12;
  pix = fc_icons::instance()->get_pixmap("plus");
  plus = pix->scaledToWidth(size);
  delete pix;
  pix = fc_icons::instance()->get_pixmap("minus");
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

/**********************************************************************/ /**
   Constructor for move widget
 **************************************************************************/
move_widget::move_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::SizeAllCursor);
  pix = fc_icons::instance()->get_pixmap("move");
  setPixmap(*pix);
  delete pix;
  setFixedSize(16, 16);
}

/**********************************************************************/ /**
   Puts move widget to left top corner
 **************************************************************************/
void move_widget::put_to_corner() { move(0, 0); }

/**********************************************************************/ /**
   Mouse handler for move widget (moves parent widget)
 **************************************************************************/
void move_widget::mouseMoveEvent(QMouseEvent *event)
{
  if (!gui()->interface_locked) {
    parentWidget()->move(event->globalPos() - point);
  }
}

/**********************************************************************/ /**
   Sets moving point for move widget;
 **************************************************************************/
void move_widget::mousePressEvent(QMouseEvent *event)
{
  if (!gui()->interface_locked) {
    point = event->globalPos() - parentWidget()->geometry().topLeft();
  }
  update();
}

/**********************************************************************/ /**
   Constructor for resize widget
 **************************************************************************/
resize_widget::resize_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::SizeFDiagCursor);
  pix = fc_icons::instance()->get_pixmap("resize");
  setPixmap(*pix);
  delete pix;
}

/**********************************************************************/ /**
   Puts resize widget to left top corner
 **************************************************************************/
void resize_widget::put_to_corner()
{
  move(parentWidget()->width() - width(),
       parentWidget()->height() - height());
}

/**********************************************************************/ /**
   Mouse handler for resize widget (resizes parent widget)
 **************************************************************************/
void resize_widget::mouseMoveEvent(QMouseEvent *event)
{
  QPoint qp, np;

  if (gui()->interface_locked) {
    return;
  }
  qp = event->globalPos();
  np.setX(qp.x() - point.x());
  np.setY(qp.y() - point.y());
  np.setX(qMax(np.x(), 32));
  np.setY(qMax(np.y(), 32));
  parentWidget()->resize(np.x(), np.y());
}

/**********************************************************************/ /**
   Sets moving point for resize widget;
 **************************************************************************/
void resize_widget::mousePressEvent(QMouseEvent *event)
{
  QPoint qp;

  if (gui()->interface_locked) {
    return;
  }
  qp = event->globalPos();
  point.setX(qp.x() - parentWidget()->width());
  point.setY(qp.y() - parentWidget()->height());
  update();
}

/**********************************************************************/ /**
   Constructor for close widget
 **************************************************************************/
close_widget::close_widget(QWidget *parent) : QLabel()
{
  QPixmap *pix;

  setParent(parent);
  setCursor(Qt::ArrowCursor);
  pix = fc_icons::instance()->get_pixmap("close");
  setPixmap(*pix);
  delete pix;
}

/**********************************************************************/ /**
   Puts close widget to right top corner
 **************************************************************************/
void close_widget::put_to_corner()
{
  move(parentWidget()->width() - width(), 0);
}

/**********************************************************************/ /**
   Mouse handler for close widget, hides parent widget
 **************************************************************************/
void close_widget::mousePressEvent(QMouseEvent *event)
{
  if (gui()->interface_locked) {
    return;
  }
  if (event->button() == Qt::LeftButton) {
    parentWidget()->hide();
    notify_parent();
  }
}

/**********************************************************************/ /**
   Notifies parent to do custom action, parent is already hidden.
 **************************************************************************/
void close_widget::notify_parent()
{
  fcwidget *fcw;

  fcw = reinterpret_cast<fcwidget *>(parentWidget());
  fcw->update_menu();
}
