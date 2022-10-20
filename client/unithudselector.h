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
#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRubberBand>
#include <QTableWidget>
// utility
#include "fc_types.h"
// gui-qt
#include "dialogs.h"
#include "shortcuts.h"

class QComboBox;
class QFontMetrics;
class QHBoxLayout;
class QIcon;
class QItemSelection;
class QKeyEvent;
class QMouseEvent;
class QMoveEvent;
class QObject;
class QPaintEvent;
class QPushButton;
class QRadioButton;
class QTimerEvent;
class QVBoxLayout;
class move_widget;
class scale_widget;
struct tile;
struct unit;
struct unit_list;

void show_new_turn_info();
bool has_player_unit_type(Unit_type_id utype);

/****************************************************************************
  Widget allowing quick select given type of units
****************************************************************************/
class unit_hud_selector : public qfc_dialog {
  Q_OBJECT
  QVBoxLayout *main_layout;
  QComboBox *unit_sel_type;
  QPushButton *select;
  QPushButton *cancel;

public:
  unit_hud_selector(QWidget *parent);
  ~unit_hud_selector() override;
  void show_me();
private slots:
  void select_units(int x = 0);
  void select_units(bool x);
  void uhs_select();
  void uhs_cancel();

protected:
  void keyPressEvent(QKeyEvent *event) override;

private:
  bool activity_filter(struct unit *punit);
  bool hp_filter(struct unit *punit);
  bool island_filter(struct unit *punit);
  bool type_filter(struct unit *punit);

  QRadioButton *any_activity;
  QRadioButton *fortified;
  QRadioButton *idle;
  QRadioButton *sentried;

  QRadioButton *any;
  QRadioButton *full_mp;
  QRadioButton *full_hp;
  QRadioButton *full_hp_mp;

  QRadioButton *this_tile;
  QRadioButton *this_continent;
  QRadioButton *main_continent;
  QRadioButton *anywhere;

  QRadioButton *this_type;
  QRadioButton *any_type;
  QLabel result_label;
};

/***************************************************************************
  Nonmodal message box for disbanding units
***************************************************************************/
class disband_box : public hud_message_box {
  Q_OBJECT
  const std::vector<unit *> cpunits;

public:
  explicit disband_box(const std::vector<unit *> &punits,
                       QWidget *parent = 0);
  ~disband_box() override;
private slots:
  void disband_clicked();
};
