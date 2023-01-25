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
// Qt
#include <QDialog>
#include <QMessageBox>
#include <QVariant>
// client
#include "dialogs_g.h"
// gui-qt
#include "widgets/decorations.h"

class QLabel;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QVBoxLayout;

void restart_notify_reports();

/***************************************************************************
 Widget around map view to display informations like demographics report,
 top 5 cities, traveler's report.
***************************************************************************/
class notify_dialog : public fcwidget {
  Q_OBJECT

public:
  notify_dialog(const char *caption, const char *headline, const char *lines,
                QWidget *parent = 0);
  void update_menu() override;
  ~notify_dialog() override;
  void restart();
  QString qcaption;
  QString qheadline;

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  void paintEvent(QPaintEvent *paint_event) override;
  void calc_size(int &x, int &y);
  close_widget *cw;
  QLabel *label;
  QVBoxLayout *layout;

  QStringList qlist;
  QFont small_font;
  QPoint cursor;
};
