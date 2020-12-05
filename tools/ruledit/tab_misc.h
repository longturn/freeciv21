/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#ifndef FC__TAB_MISC_H
#define FC__TAB_MISC_H

// Qt
#include <QWidget>

class QLineEdit;
class QRadioButton;
class QTableWidget;

class ruledit_gui;

class tab_misc : public QWidget {
  Q_OBJECT

public:
  explicit tab_misc(ruledit_gui *ui_in);
  void refresh();
  void flush_widgets();

private slots:
  void save_now();
  void refresh_stats();
  void edit_aae_effects();
  void edit_all_effects();

private:
  ruledit_gui *ui;
  QLineEdit *name;
  QLineEdit *version;
  QLineEdit *savedir;
  QRadioButton *savedir_version;
  QTableWidget *stats;
};

#endif // FC__TAB_MISC_H
