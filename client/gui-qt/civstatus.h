/**************************************************************************
                  Copyright (c) 2021 Freeciv21 contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "widgetdecorations.h"

class QHBoxLayout;

class civstatus : public fcwidget {
  Q_OBJECT

public:
  civstatus(QWidget *parent = nullptr);
  ~civstatus() override;
  void update_menu() override;
private slots:
  void updateInfo();
private:
  QHBoxLayout *layout;
  QLabel economyLabel;
  QLabel scienceLabel;
  QLabel unitLabel;
};