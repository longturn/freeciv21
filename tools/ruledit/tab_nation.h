/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__TAB_NATION_H
#define FC__TAB_NATION_H

// Qt
#include <QLineEdit>
#include <QRadioButton>
#include <QWidget>

class ruledit_gui;

class tab_nation : public QWidget {
  Q_OBJECT

public:
  explicit tab_nation(ruledit_gui *ui_in);
  void refresh();
  void flush_widgets();

private slots:
  void nationlist_toggle(bool checked);

private:
  ruledit_gui *ui;

  QRadioButton *via_include;
  QLineEdit *nationlist;
};

#endif // FC__TAB_MISC_H
