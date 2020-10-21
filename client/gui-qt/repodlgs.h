/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__REPODLGS_H
#define FC__REPODLGS_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "repodlgs_g.h"

// gui-qt
#include "mapview.h"

// Qt
#include <QLabel>
#include <QPushButton>
#include <QWidget>

class QGridLayout;
class QHBoxLayout;
class QItemSelection;
class QTableWidget;
class QTableWidgetItem;

class unittype_item : public QFrame {

  Q_OBJECT

  bool entered;
  int unit_scroll;
  QLabel label_pix;

public:
  unittype_item(QWidget *parent, unit_type *ut);
  ~unittype_item();
  void init_img();
  QLabel food_upkeep;
  QLabel gold_upkeep;
  QLabel label_info_active;
  QLabel label_info_inbuild;
  QLabel label_info_unit;
  QLabel shield_upkeep;
  QPushButton upgrade_button;

private:
  unit_type *utype;

private slots:
  void upgrade_units();

protected:
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void paintEvent(QPaintEvent *event);
  void wheelEvent(QWheelEvent *event);
};

class units_reports : public fcwidget {

  Q_DISABLE_COPY(units_reports);
  Q_OBJECT

  close_widget *cw;
  explicit units_reports();
  QHBoxLayout *scroll_layout;
  QScrollArea *scroll;
  QWidget scroll_widget;
  static units_reports *m_instance;

public:
  ~units_reports();
  static units_reports *instance();
  static void drop();
  void clear_layout();
  void init_layout();
  void update_units(bool show = false);
  void add_item(unittype_item *item);
  virtual void update_menu();
  QHBoxLayout *layout;
  QList<unittype_item *> unittype_list;

protected:
  void paintEvent(QPaintEvent *event);
};

/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class eco_report : public QWidget {
  Q_OBJECT
  QPushButton *disband_button;
  QPushButton *sell_button;
  QPushButton *sell_redun_button;
  QTableWidget *eco_widget;
  QLabel *eco_label;

public:
  eco_report();
  ~eco_report();
  void update_report();
  void init();

private:
  int index;
  int curr_row;
  int max_row;
  cid uid;
  int counter;

private slots:
  void disband_units();
  void sell_buildings();
  void sell_redundant();
  void selection_changed(const QItemSelection &sl, const QItemSelection &ds);
};

/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class endgame_report : public QWidget {
  Q_OBJECT
  QTableWidget *end_widget;

public:
  endgame_report(const struct packet_endgame_report *packet);
  ~endgame_report();
  void update_report(const struct packet_endgame_player *packet);
  void init();

private:
  int index;
  int players;
};

void popdown_economy_report();
void popdown_units_report();
void popdown_endgame_report();
void popup_endgame_report();
void toggle_units_report(bool);

#endif /* FC__REPODLGS_H */
