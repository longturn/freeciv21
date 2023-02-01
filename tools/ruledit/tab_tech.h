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

#ifndef FC__TAB_TECH_H
#define FC__TAB_TECH_H

#include <fc_config.h>

// Qt
#include <QWidget>

// common
#include "tech.h"

class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QRadioButton;
class QToolButton;

class ruledit_gui;

class tab_tech : public QWidget {
  Q_OBJECT

public:
  explicit tab_tech(ruledit_gui *ui_in);
  void refresh();
  static void techs_to_menu(QMenu *fill_menu);
  static QString tech_name(struct advance *padv);

private:
  ruledit_gui *ui;
  void update_tech_info(struct advance *adv);
  QMenu *prepare_req_button(QToolButton *button, enum tech_req rn);
  bool initialize_new_tech(struct advance *padv);

  QLineEdit *name;
  QLineEdit *rname;
  QToolButton *req1_button;
  QToolButton *req2_button;
  QToolButton *root_req_button;
  QMenu *req1;
  QMenu *req2;
  QMenu *root_req;
  QListWidget *tech_list;
  QRadioButton *same_name;

  struct advance *selected;

private slots:
  void name_given();
  void select_tech();
  void req1_jump();
  void req2_jump();
  void root_req_jump();
  void req1_menu(QAction *action);
  void req2_menu(QAction *action);
  void root_req_menu(QAction *action);
  void add_now();
  void delete_now();
  void same_name_toggle(bool checked);
  void edit_effects();
};

#endif // FC__TAB_TECH_H
