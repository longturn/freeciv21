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
#include <qnamespace.h>
// gui-qt
#include "fc_client.h"
#include "icons.h"

/**
  Scale widget allowing scaling other widgets, shown in right top corner
 */
scale_widget::scale_widget(QRubberBand::Shape s, QWidget *p)
    : QRubberBand(s, p)
{
  QPixmap *pix;

  size = 12;
  pix = fcIcons::instance()->getPixmap(QStringLiteral("plus"));
  plus = pix->scaledToWidth(size);
  delete pix;
  pix = fcIcons::instance()->getPixmap(QStringLiteral("minus"));
  minus = pix->scaledToWidth(size);
  delete pix;
  setFixedSize(2 * size, size);
  scale = 1.0f;
  setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

/**
  Draws 2 icons for resizing
 */
void scale_widget::paintEvent(QPaintEvent *event)
{
  QRubberBand::paintEvent(event);
  QPainter p;
  p.begin(this);
  p.drawPixmap(0, 0, minus);
  p.drawPixmap(size, 0, plus);
  p.end();
}

/**
  Mouse press event for scale widget
 */
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
void close_widget::put_to_corner() { move(parentWidget()->width() - 12, 0); }

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
  }
}

/**
 * Hide event handler for close widget
 */
void close_widget::hideEvent(QHideEvent *event)
{
  Q_UNUSED(event);
  if (auto fcw = qobject_cast<fcwidget *>(parentWidget()); fcw) {
    fcw->update_menu();
  }
}

/**
   Set resizable flags
 */
void resizable_widget::setResizable(Qt::Edges edges)
{
  resizeFlags = edges;
  auto margins = QMargins();
  if (edges & Qt::LeftEdge) {
    margins.setLeft(margin_width);
  }
  if (edges & Qt::RightEdge) {
    margins.setRight(margin_width);
  }
  if (edges & Qt::TopEdge) {
    margins.setTop(margin_width);
  }
  if (edges & Qt::BottomEdge) {
    margins.setBottom(margin_width);
  }
  setContentsMargins(margins);
}

/**
   Get resizable flags of wdiget
 */
Qt::Edges resizable_widget::getResizable() const { return resizeFlags; }

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
    eventFlags = resizeFlags & get_in_event_mouse(event);
  }
  event->setAccepted(true);
}

/**
   Get resizable_flag from mouse position
 */
Qt::Edges
resizable_widget::get_in_event_mouse(const QMouseEvent *event) const
{
  auto flags = Qt::Edges();

  if (std::abs(event->x()) < event_width && event->x() < width() / 2) {
    flags |= Qt::LeftEdge;
  } else if (std::abs(event->x() - width()) < event_width
             && event->x() >= width() / 2) {
    flags |= Qt::RightEdge;
  }

  if (std::abs(event->y()) < event_width) {
    flags |= Qt::TopEdge;
  } else if (std::abs(event->y() - height()) < event_width
             && event->y() >= height() / 2) {
    flags |= Qt::BottomEdge;
  }

  return flags;
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
  if (eventFlags != Qt::Edges()) {
    eventFlags = Qt::Edges();
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
    if (eventFlags != Qt::Edges()) {
      auto new_rect = QRect(pos(), size());

      // The x and y that we should reach, relative to the parent widget
      const auto target = event->pos() + pos();

      // Resizing and moving depending on the type of event
      if (eventFlags & Qt::TopEdge) {
        new_rect.setTop(target.y());
        if (new_rect.height() < minimumHeight()) {
          new_rect.setTop(new_rect.bottom() - minimumHeight());
        }
      } else if (eventFlags & Qt::BottomEdge) {
        new_rect.setBottom(target.y());
        if (new_rect.height() < minimumHeight()) {
          new_rect.setBottom(new_rect.top() + minimumHeight());
        }
      }

      if (eventFlags & Qt::LeftEdge) {
        new_rect.setLeft(target.x());
        if (new_rect.width() < minimumWidth()) {
          new_rect.setLeft(new_rect.right() - minimumWidth());
        }
      } else if (eventFlags & Qt::RightEdge) {
        new_rect.setRight(target.x());
        if (new_rect.width() < minimumWidth()) {
          new_rect.setRight(new_rect.left() + minimumWidth());
        }
      }

      // Prevent resizing out of the parent
      new_rect &= parentWidget()->rect();

      resize(new_rect.size());
      move(new_rect.topLeft());
    }
  } else {
    // Get flag from mouse position
    auto flags = get_in_event_mouse(event) & resizeFlags;

    // Change the cursor if the flag is active and the widget has this flag
    if (flags == Qt::LeftEdge || flags == Qt::RightEdge) {
      setCursor(Qt::SizeHorCursor);
    } else if (flags == Qt::TopEdge || flags == Qt::BottomEdge) {
      setCursor(Qt::SizeVerCursor);
    } else if (flags == (Qt::TopEdge | Qt::LeftEdge)
               || flags == (Qt::BottomEdge | Qt::RightEdge)) {
      setCursor(Qt::SizeFDiagCursor);
    } else if (flags == (Qt::TopEdge | Qt::RightEdge)
               || flags == (Qt::BottomEdge | Qt::LeftEdge)) {
      setCursor(Qt::SizeBDiagCursor);
    } else {
      // Otherwise change cursor to default
      setCursor(Qt::ArrowCursor);
    }
  }
  event->setAccepted(true);
}
