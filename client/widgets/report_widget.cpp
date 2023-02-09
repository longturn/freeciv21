/*
 * SPDX-FileCopyrightText: Freeciv21 and Freeciv contributors
 * SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "widgets/report_widget.h"

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
 *\class report_widget
 * Widget used to display the demographics, top 5 cities, and travelers'
 * reports.
 */

/**
 * Creates a report widget displaying the provided text.
 */
report_widget::report_widget(const QString &caption, const QString &headline,
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

  adjustSize();

  auto cw = new close_widget(this);
  cw->setFixedSize(12, 12);
  cw->put_to_corner();

  auto x = width();
  auto y = height();
  queen()->mapview_wdg->find_place(queen()->mapview_wdg->width() - x - 4, 4,
                                   x, y, x, y, 0);
  move(x, y);
}

/**
 * Reimplemented to let the user drag the widget.
 */
void report_widget::mousePressEvent(QMouseEvent *event)
{
  m_cursor = event->globalPos() - geometry().topLeft();
}

/**
 * Reimplemented to let the user drag the widget.
 */
void report_widget::mouseMoveEvent(QMouseEvent *event)
{
  move(event->globalPos() - m_cursor);
  setCursor(Qt::SizeAllCursor);
}

/**
 * Reimplemented to let the user drag the widget.
 */
void report_widget::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  setCursor(Qt::ArrowCursor);
}

/**
 * Overridden to handle font changes
 */
bool report_widget::event(QEvent *event)
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
 * Called when the close button is pressed
 */
void report_widget::update_menu() { deleteLater(); }

/**
   Restarts all notify dialogs
 */
void restart_notify_reports()
{
  auto list = queen()->mapview_wdg->findChildren<report_widget *>();
  for (auto nd : list) {
    QApplication::postEvent(nd, new QEvent(QEvent::FontChange));
  }
}
