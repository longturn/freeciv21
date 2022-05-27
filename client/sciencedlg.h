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

// utility
#include "fc_types.h"
// client
#include "repodlgs_g.h"
#include "reqtree.h"

class QComboBox;
class QGridLayout;
class QLabel;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QScrollArea;
class progress_bar;

/****************************************************************************
  Custom widget representing research diagram in science_report
****************************************************************************/
class research_diagram : public QWidget {
  Q_OBJECT

public:
  research_diagram(QWidget *parent = 0);
  ~research_diagram() override;
  void update_reqtree();
  void reset();
  QSize size();
private slots:
  void show_tooltip();

private:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  QPixmap *pcanvas;
  struct reqtree *req;
  bool timer_active;
  int width;
  int height;
  QList<req_tooltip_help *> *tt_help{nullptr};
  QPoint tooltip_pos;
  QString tooltip_text;
  QRect tooltip_rect;
};

/****************************************************************************
  Helper item for comboboxes, holding string of tech and its id
****************************************************************************/
struct qlist_item {
  QString tech_str;
  Tech_type_id id;
};

/****************************************************************************
  Widget embedded as tab on game view (F6 default)
  Uses string "SCI" to mark it as opened
  You can check it using if (queen()->is_repo_dlg_open("SCI"))
****************************************************************************/
class science_report : public QWidget {
  Q_OBJECT

  QComboBox *goal_combo;
  QComboBox *researching_combo;
  QGridLayout *sci_layout;
  progress_bar *progress;
  QLabel *info_label;
  QLabel *progress_label;
  QList<qlist_item> *curr_list{nullptr};
  QList<qlist_item> *goal_list{nullptr};
  research_diagram *res_diag;
  QScrollArea *scroll;

public:
  science_report();
  ~science_report() override;
  void update_report();
  void init(bool raise);
  void redraw();
  void reset_tree();

private:
  void update_reqtree();
  int index{0};

private slots:
  void current_tech_changed(int index);
  void goal_tech_changed(int index);
};

void popdown_science_report();
bool comp_less_than(const qlist_item &q1, const qlist_item &q2);
