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
#include <QMap>
#include <QTabWidget>
// common
#include "diptreaty.h"

class QCloseEvent;
class QGridLayout;
class QLabel;
class QObject;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;

/****************************************************************************
  Diplomacy tab for one nation
****************************************************************************/
class diplo_wdg : public QWidget {
  Q_OBJECT
  QGridLayout *layout;
  QLabel *plr1_accept;
  QLabel *plr2_accept;
  QPushButton *accept_treaty;
  QPushButton *cancel_treaty;
  QSpinBox *gold_edit1;
  QSpinBox *gold_edit2;
  QTableWidget *text_edit;

public:
  diplo_wdg(int id, int id2);
  ~diplo_wdg() override;
  void update_wdg();
  void set_index(int ind);
  int get_index();
  struct Treaty treaty;

private slots:
  void all_advances();
  void dbl_click(QTableWidgetItem *item);
  void give_advance(int tech);
  void give_city(int city_num);
  void give_embassy();
  void give_shared_vision();
  void gold_changed1(int val);
  void gold_changed2(int val);
  void pact_allianze();
  void pact_ceasfire();
  void pact_peace();
  void response_accept();
  void response_cancel();
  void sea_map_clause();
  void show_menu(int player);
  void show_menu_p1();
  void show_menu_p2();
  void world_map_clause();
  void restore_pixmap();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  int player1;
  int player2;
  int active_menu;
  int curr_player;
  bool p1_accept;
  bool p2_accept;
  int index;
};

/****************************************************************************
  Diplomacy dialog containing many diplo_wdg
****************************************************************************/
class diplo_dlg : public QTabWidget {
  Q_OBJECT
  QMap<int, diplo_wdg *> treaty_list;

public:
  diplo_dlg(int counterpart, int initiated_from);
  ~diplo_dlg() override;
  void update_dlg();
  bool init(bool raise);
  diplo_wdg *find_widget(int counterpart);
  void close_widget(int counterpart);
  void add_widget(int counterpart, int initiated_from);
  void make_active(int party);

private:
  int index;
};
