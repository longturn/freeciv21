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

void popdown_endgame_report();
void popup_endgame_report();

#endif /* FC__REPODLGS_H */
