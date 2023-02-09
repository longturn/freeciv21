/*
 * SPDX-FileCopyrightText: Freeciv21 and Freeciv contributors
 * SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

// common
#include "events.h"

// client
#include "widgets/decorations.h"

class QLabel;

class report_widget : public fcwidget {
  Q_OBJECT

public:
  report_widget(const QString &caption, const QString &headline,
                const QString &lines, QWidget *parent = nullptr);
  ~report_widget() override = default;

  /// Returns back the caption passed to the constructor
  QString caption() const { return m_caption; }

  /// Returns back the headline passed to the constructor
  QString headline() const { return m_headline; }

  void update_menu() override;

  static void update_fonts();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  bool event(QEvent *event) override;

private:
  QString m_caption, m_headline;
  QPoint m_cursor;
  QLabel *m_contents;
};
