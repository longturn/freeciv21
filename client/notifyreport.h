/**************************************************************************
 Copyright (c) 1996-2023 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// common
#include "events.h"

// client
#include "widgets/decorations.h"

class QLabel;

void restart_notify_reports();

/***************************************************************************
 Widget around map view to display informations like demographics report,
 top 5 cities, traveler's report.
***************************************************************************/
class notify_dialog : public fcwidget {
  Q_OBJECT

public:
  notify_dialog(const QString &caption, const QString &headline,
                const QString &lines, QWidget *parent = nullptr);
  ~notify_dialog() override = default;

  /// Returns back the caption passed to the constructor
  QString caption() const { return m_caption; }

  /// Returns back the headline passed to the constructor
  QString headline() const { return m_headline; }

  void update_menu() override;

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
