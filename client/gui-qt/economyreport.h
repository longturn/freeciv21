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

#include "ui_economyreport.h"

// client
#include "climisc.h"
#include "repodlgs_g.h"

/****************************************************************************
  Tab widget to display economy report (F5)
****************************************************************************/
class eco_report : public QWidget {
  Q_OBJECT

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
  Ui::FormEconomyReport ui;
private slots:
  void disband_units();
  void sell_buildings();
  void sell_redundant();
  void selection_changed(const QItemSelection &sl, const QItemSelection &ds);
};

void popdown_economy_report();
