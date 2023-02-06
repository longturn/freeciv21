/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
 */

#pragma once

// Qt
#include <QItemSelection>
#include <QTableWidget>
#include <QWidget>

#include "ui_view_units.h"

// client
#include "climisc.h"
#include "repodlgs_g.h"

struct unit_type;

/****************************************************************************
  Tab widget to display units view (F2)
****************************************************************************/
class units_view : public QWidget {
  Q_OBJECT

public:
  units_view();
  ~units_view();
  void update_view();
  void update_waiting();
  void init();

private:
  int index;
  int curr_row{-1};
  int max_row{0};
  cid uid{0};
  int counter{0};
  Ui::FormUnitsView ui;
private slots:
  void disband_units();
  void find_nearest();
  void upgrade_units();
  void selection_changed(const QItemSelection &sl, const QItemSelection &ds);
};

void popdown_units_view();
