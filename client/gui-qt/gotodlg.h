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

// Qt
#include <QWidget>
// common
#include "unit.h"
// client
#include "gotodlg_g.h"

class QTableWidget;
class QPushButton;
class QCheckBox;
class QGridLayout;
class QItemSelection;
class QLabel;

/***************************************************************************
 Class for displaying goto/airlift dialog (widget)
***************************************************************************/
class goto_dialog : public QWidget {
  Q_OBJECT
  QTableWidget *goto_tab;
  QPushButton *goto_city;
  QPushButton *airlift_city;
  QPushButton *close_but;
  QCheckBox *show_all;
  QGridLayout *layout;
  QLabel *show_all_label;

public:
  goto_dialog(QWidget *parent = 0);
  void init();
  ~goto_dialog();
  void update_dlg();
  void show_me();
  void sort_def();

private slots:
  void go_to_city();
  void airlift_to();
  void close_dlg();
  void item_selected(const QItemSelection &sl, const QItemSelection &ds);
  void checkbox_changed(int state);

protected:
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);

private:
  void fill_tab(struct player *pplayer);
  struct tile *original_tile;
};
