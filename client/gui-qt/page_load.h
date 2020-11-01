/**************************************************************************
  /\ ___ /\           Copyright (c) 1996-2020 ＦＲＥＥＣＩＶ ２１ and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                If not, see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "ui_page_load.h"
#include <QWidget>
class fc_client;

class page_load : public QWidget
{
    Q_OBJECT
    public:
    page_load(QWidget *, fc_client *);
    ~page_load();
    void update_load_page(void);
private slots:
  void slot_selection_changed(const QItemSelection &,
                              const QItemSelection &);
  void state_preview(int);
  void browse_saves();

private:
  void start_from_save();
  fc_client* gui;
  Ui::FormPageLoad ui;
  QString current_file;
};
