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

/* client */
#include "text.h"
// Qt
#include <QWidget>

class QLabel;
class QPixmap;
class QPushButton;

/****************************************************************************
  Tab widget to display spaceship report (F12)
****************************************************************************/
class ss_report : public QWidget {
  Q_OBJECT
  QPushButton *launch_button;
  QLabel *ss_pix_label;
  QLabel *ss_label;
  struct canvas *can;

public:
  ss_report(struct player *pplayer);
  ~ss_report();
  void update_report();
  void init();

private slots:
  void launch();

private:
  struct player *player;
};

void popup_spaceship_dialog(struct player *pplayer);
void popdown_all_spaceships_dialogs();
