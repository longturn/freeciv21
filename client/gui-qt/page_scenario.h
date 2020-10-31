/**************************************************************************
  /\___/\       Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors.
 (  o o  )      This file is part of Freeciv21. Freeciv21 is free software:
===  v  ===        you can redistribute it and/or modify it under the terms
   )   (        of the GNU General Public License  as published by the Free
   ooooo       Software Foundation, either version 3 of the License, or (at
            your option) any later version. You should have received a copy
                    of the GNU General Public License along with Freeciv21.
                                 If not, see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "ui_page_scenario.h"
#include <QWidget>
class fc_client;

class page_scenario : public QWidget
{
    Q_OBJECT
    public:
    page_scenario(QWidget *, fc_client *);
    ~page_scenario();
    void update_scenarios_page(void);
private slots:
  void slot_selection_changed(const QItemSelection &,
                              const QItemSelection &);
  void browse_scenarios();
  void start_scenario();
private:
  fc_client* king;
  Ui::FormPageScenario ui;
  QString current_file;
};
