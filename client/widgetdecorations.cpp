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
   Set resizable flags
 */
void resizable_widget::setResizable(QFlags<resizable_flag> flags)
{
  resizeFlags = flags;
}

/**
   Get resizable flags of wdiget
 */
QFlags<resizable_flag> resizable_widget::getResizable() const
{
  return resizeFlags;
}

/**
   Remove all resizable flags
 */
void resizable_widget::removeResizable() { resizeFlags = {}; }

/**
   Check if resizable flag is active
 */
bool resizable_widget::hasResizable(resizable_flag flag) const
{
  return resizeFlags.testFlag(flag);
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
    // Get flag from mouse position
    auto flag = get_in_event_mouse(event);

    // Check the flag and widget for the presence of a flag
    if (flag != resizable_flag::none && resizeFlags.testFlag(flag)) {
      // Save flag and mouse position for mouse move event
      eventFlag = flag;
      last_position = event->globalPos();
    }
  }
  event->setAccepted(true);
}

/**
   Get resizable_flag from mouse position
 */
resizable_flag
resizable_widget::get_in_event_mouse(const QMouseEvent *event) const
{
  if (event->x() >= width() / 2 - event_width
      && event->x() <= width() / 2 + event_width && event->y() >= 0
      && event->y() <= event_width) {
    return resizable_flag::top;
  }

  if (event->x() >= 0 && event->x() <= event_width && event->y() >= 0
      && event->y() <= event_width) {
    return resizable_flag::topLeft;
  }

  if (event->x() >= width() - event_width && event->x() <= width()
      && event->y() >= 0 && event->y() <= event_width) {
    return resizable_flag::topRight;
  }

  if (event->x() >= width() / 2 - event_width
      && event->x() <= width() / 2 + event_width
      && event->y() >= height() - event_width && event->y() <= height()) {
    return resizable_flag::bottom;
  }

  if (event->x() >= 0 && event->x() <= event_width
      && event->y() >= height() - event_width && event->y() <= height()) {
    return resizable_flag::bottomLeft;
  }

  if (event->x() >= width() - event_width && event->x() <= width()
      && event->y() >= height() - event_width && event->y() <= height()) {
    return resizable_flag::bottomRight;
  }

  if (event->x() >= 0 && event->x() <= event_width
      && event->y() >= height() / 2 - event_width
      && event->y() <= height() / 2 + event_width) {
    return resizable_flag::left;
  }

  if (event->x() >= width() - event_width && event->x() <= width()
      && event->y() >= height() / 2 - event_width
      && event->y() <= height() / 2 + event_width) {
    return resizable_flag::right;
  }

  return resizable_flag::none;
}

/**
   Restores cursor when resizing is done.
 */
void resizable_widget::mouseReleaseEvent(QMouseEvent *event)
{
  if (king()->interface_locked) {
    return;
  }

  // If the event flag is active, then reset all
  if (eventFlag != resizable_flag::none) {
    eventFlag = resizable_flag::none;
    last_position = QPoint{};
    setCursor(Qt::ArrowCursor);
  }
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

  // Check left button state
  if (event->buttons() & Qt::LeftButton) {
    // If the event flag is active
    if (eventFlag != resizable_flag::none) {
      QSize size{width(), height()};
      QPoint pos{x(), y()};

      // Calculate diff betwen position and update last position
      auto diff = event->globalPos() - last_position;
      last_position = event->globalPos();

      // Resizing and moving depending on the type of event
      switch (eventFlag) {
      case resizable_flag::top: {
        if (minimumHeight() < height() - diff.y()) {
          size.setHeight(height() - diff.y());
          pos.setY(y() + diff.y());
        }

        resize(size.width(), size.height());
        move(pos.x(), pos.y());
      } break;

      case resizable_flag::topLeft: {
        if (minimumWidth() < width() - diff.x()) {
          size.setWidth(width() - diff.x());
          pos.setX(x() + diff.x());
        }

        if (minimumHeight() < height() - diff.y()) {
          size.setHeight(height() - diff.y());
          pos.setY(y() + diff.y());
        }

        resize(size.width(), size.height());
        move(pos.x(), pos.y());
      } break;

      case resizable_flag::topRight: {
        if (minimumWidth() < width() + diff.x()) {
          size.setWidth(width() + diff.x());
        }

        if (minimumHeight() < height() - diff.y()) {
          size.setHeight(height() - diff.y());
          pos.setY(y() + diff.y());
        }

        resize(size.width(), size.height());
        move(pos.x(), pos.y());
      } break;

      case resizable_flag::bottom: {
        if (minimumHeight() < height() + diff.y()) {
          size.setHeight(height() + diff.y());
        }

        resize(size.width(), size.height());
      } break;

      case resizable_flag::bottomLeft: {
        if (minimumWidth() < width() - diff.x()) {
          size.setWidth(width() - diff.x());
          pos.setX(x() + diff.x());
        }

        if (minimumHeight() < height() + diff.y()) {
          size.setHeight(height() + diff.y());
        }

        resize(size.width(), size.height());
        move(pos.x(), pos.y());
      } break;

      case resizable_flag::bottomRight: {
        if (minimumWidth() < width() + diff.x()) {
          size.setWidth(width() + diff.x());
        }

        if (minimumHeight() < height() + diff.y()) {
          size.setHeight(height() + diff.y());
        }

        resize(size.width(), size.height());
      } break;

      case resizable_flag::left: {
        if (minimumWidth() < width() - diff.x()) {
          size.setWidth(width() - diff.x());
          pos.setX(x() + diff.x());
        }

        resize(size.width(), size.height());
        move(pos.x(), pos.y());
      } break;

      case resizable_flag::right: {
        resize((std::max)(minimumWidth(), width() + diff.x()), height());
      } break;

      default:
        break;
      }
    }
  } else {
    // Get flag from mouse position
    auto flag = get_in_event_mouse(event);

    // Change the cursor if the flag is active and the widget has this flag
    if (flag != resizable_flag::none && resizeFlags.testFlag(flag)) {
      if (flag == resizable_flag::top || flag == resizable_flag::bottom) {
        setCursor(Qt::SizeVerCursor);
      } else if (flag == resizable_flag::topLeft
                 || flag == resizable_flag::bottomRight) {
        setCursor(Qt::SizeFDiagCursor);
      } else if (flag == resizable_flag::topRight
                 || flag == resizable_flag::bottomLeft) {
        setCursor(Qt::SizeBDiagCursor);
      } else if (flag == resizable_flag::left
                 || flag == resizable_flag::right) {
        setCursor(Qt::SizeHorCursor);
      }
    } else {
      // Otherwise change cursor to default
      setCursor(Qt::ArrowCursor);
    }
  }
  event->setAccepted(true);
}
