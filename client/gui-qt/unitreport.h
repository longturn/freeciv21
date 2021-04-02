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
#include <QLabel>
#include <QPushButton>
#include <QWidget>

// client
#include "repodlgs_g.h"
// gui-qt
#include "mapview.h"
#include "widgetdecorations.h"

class QEvent;
class QHBoxLayout; // lines 25-25
class QObject;
class QPaintEvent;
class QScrollArea;
class QTableWidget;
class QWheelEvent;
struct unit_type;

class units_waiting : public QWidget {
  Q_OBJECT
public:
  units_waiting(QWidget *parent = nullptr);
  ~units_waiting();
protected:
  void showEvent(QShowEvent *event) override;
private:
  void clicked(int x, int y);
  void update_units();
  QTableWidget *waiting_units;
};

class unittype_item : public QFrame {
  Q_OBJECT

  bool entered;
  int unit_scroll;
  QLabel label_pix;

public:
  unittype_item(QWidget *parent, unit_type *ut);
  ~unittype_item() override;
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
  void enterEvent(QEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
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
  units_waiting *uw;
public:
  ~units_reports() override;
  static units_reports *instance();
  static void drop();
  void clear_layout();
  void init_layout();
  void update_units(bool show = false);
  void add_item(unittype_item *item);
  void update_menu() override;
  static bool exists();
  QHBoxLayout *layout;
  QList<unittype_item *> unittype_list;

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void hideEvent(QHideEvent *event) override;
};

void popdown_units_report();
void toggle_units_report(bool);
