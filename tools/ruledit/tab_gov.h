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

#ifndef FC__TAB_GOV_H
#define FC__TAB_GOV_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QWidget>

class QLineEdit;
class QListWidget;
class QRadioButton;

class ruledit_gui;

class tab_gov : public QWidget {
  Q_OBJECT

public:
  explicit tab_gov(ruledit_gui *ui_in);
  void refresh();

private:
  ruledit_gui *ui;
  void update_gov_info(struct government *pgov);
  bool initialize_new_gov(struct government *pgov);

  QLineEdit *name;
  QLineEdit *rname;
  QListWidget *gov_list;
  QRadioButton *same_name;

  struct government *selected;

private slots:
  void name_given();
  void select_gov();
  void add_now();
  void delete_now();
  void same_name_toggle(bool checked);
  void edit_reqs();
  void edit_effects();
};

#endif // FC__TAB_GOV_H
