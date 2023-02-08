/*
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include "notifyreport.h"

// client
#include "dialogs_g.h"
#include "fc_client.h"
#include "fonts.h"
#include "page_game.h"
#include "views/view_map.h"

// Qt
#include <QApplication>
#include <QGridLayout>
#include <QMouseEvent>

/**
   Constructor for notify dialog
 */
notify_dialog::notify_dialog(const QString &caption, const QString &headline,
                             const QString &lines, QWidget *parent)
    : fcwidget(), m_caption(caption), m_headline(headline)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_NoMousePropagation);
  setCursor(Qt::ArrowCursor);
  setParent(parent);
  setFrameStyle(QFrame::Box);

  auto layout = new QGridLayout;
  setLayout(layout);

  m_contents =
      new QLabel(QStringLiteral("%1 %2\n%3").arg(caption, headline, lines));
  m_contents->setTextInteractionFlags(Qt::TextSelectableByMouse);
  m_contents->setFont(fcFont::instance()->getFont(
      QStringLiteral("gui_qt_font_notify_label")));
  layout->addWidget(m_contents, 0, 0);

  auto cw = new close_widget(this);
  layout->addWidget(cw, 0, 1, Qt::AlignTop | Qt::AlignRight);

  adjustSize();

  auto x = width();
  auto y = height();
  queen()->mapview_wdg->find_place(queen()->mapview_wdg->width() - x - 4, 4,
                                   x, y, x, y, 0);
  move(x, y);
}

/**
   Called when mouse button was pressed, just to close on right click
 */
void notify_dialog::mousePressEvent(QMouseEvent *event)
{
  m_cursor = event->globalPos() - geometry().topLeft();
}

/**
   Called when mouse button was pressed and moving around
 */
void notify_dialog::mouseMoveEvent(QMouseEvent *event)
{
  move(event->globalPos() - m_cursor);
  setCursor(Qt::SizeAllCursor);
}

/**
   Called when mouse button unpressed. Restores cursor.
 */
void notify_dialog::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  setCursor(Qt::ArrowCursor);
}

/**
 * Overridden to handle font changes
 */
bool notify_dialog::event(QEvent *event)
{
  if (event->type() == QEvent::FontChange) {
    m_contents->setFont(fcFont::instance()->getFont(
        QStringLiteral("gui_qt_font_notify_label")));
    adjustSize();
    event->accept();
    return true;
  }
  return fcwidget::event(event);
}

/**
   Called when close button was pressed
 */
void notify_dialog::update_menu() { deleteLater(); }

/**
   Restarts all notify dialogs
 */
void restart_notify_reports()
{
  auto list = queen()->mapview_wdg->findChildren<notify_dialog *>();
  for (auto nd : list) {
    QApplication::postEvent(nd, new QEvent(QEvent::FontChange));
  }
}
