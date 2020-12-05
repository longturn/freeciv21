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

#ifndef FC__TAB_ENABLERS_H
#define FC__TAB_ENABLERS_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QWidget>

/* ruledit */
#include "req_vec_fix.h"

class QPushButton;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QRadioButton;
class QToolButton;

class ruledit_gui;

class tab_enabler : public QWidget {
  Q_OBJECT

public:
  explicit tab_enabler(ruledit_gui *ui_in);
  void refresh();

private:
  ruledit_gui *ui;
  void update_enabler_info(struct action_enabler *enabler);
  bool initialize_new_enabler(struct action_enabler *enabler);

  QToolButton *type_button;
  QMenu *type_menu;
  QPushButton *act_reqs_button;
  QPushButton *tgt_reqs_button;
  QPushButton *delete_button;
  QPushButton *repair_button;
  QListWidget *enabler_list;

  struct action_enabler *selected;

private slots:
  void select_enabler();
  void add_now();
  void repair_now();
  void incoming_rec_vec_change(const requirement_vector *vec);
  void delete_now();
  void edit_type(QAction *action);
  void edit_target_reqs();
  void edit_actor_reqs();
};

#endif // FC__TAB_ENABLERS_H
