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
#include <QObject>
#include <QString>
#include <QWidget>
// client
#include "repodlgs_g.h"
class QObject;
class QTableWidget;

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
