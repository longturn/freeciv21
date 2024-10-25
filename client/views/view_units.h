/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
 * SPDX-FileCopyrightText: Tobias Rehbein <tobias.rehbein@web.de>
 */

#pragma once

// Qt
#include <QItemSelection>
#include <QTableWidget>
#include <QWidget>

#include "ui_view_units.h"

// client
#include "climisc.h"

struct unit_type;

/**
 * Structure of data for the Units View.
 * See get_units_view_data()
 */
struct unit_view_entry {
  const struct unit_type *type;
  int count, in_prod, total_cost, food_cost, gold_cost, shield_cost;
  bool upg;
};
std::vector<unit_view_entry> get_units_view_data();

/**
 * Structure of unit waiting data for the Units View.
 * See get_units_waiting_data()
 */
struct unit_waiting_entry {
  const struct unit_type *type;
  time_t timer;
  QString city_name;
  int id;
};
std::vector<unit_waiting_entry> get_units_waiting_data();

void units_view_dialog_update(void *unused);

void popdown_units_view();

struct unit *find_nearest_unit(const struct unit_type *utype,
                               struct tile *ptile);

/**
 * Table widget to display units view (F2)
 */
class units_view : public QWidget {
  Q_OBJECT

public:
  units_view();
  ~units_view();
  void update_view();
  void update_units();
  void init();

private:
  int index;
  int curr_row{-1};
  int max_row{0};
  cid uid{0};
  int counter{0};
  Ui::FormUnitsView ui;
  void update_waiting();
  void update_buttons(const QItemSelection &sl);

private slots:
  void disband_units();
  void find_nearest();
  void upgrade_units();
  void selection_changed(const QItemSelection &sl, const QItemSelection &ds);
};
