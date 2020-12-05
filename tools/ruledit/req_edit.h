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

#ifndef FC__REQ_EDIT_H
#define FC__REQ_EDIT_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QListWidget>
#include <QToolButton>

// common
#include "requirements.h"

class ruledit_gui;

class req_edit : public QDialog {
  Q_OBJECT

public:
  explicit req_edit(ruledit_gui *ui_in, QString target,
                    struct requirement_vector *preqs);
  void refresh();
  void add(const char *msg);

  struct requirement_vector *req_vector;

signals:
  /********************************************************************/ /**
     A requirement vector may have been changed.
     @param vec the requirement vector that was changed.
   ************************************************************************/
  void rec_vec_may_have_changed(const requirement_vector *vec);

private:
  ruledit_gui *ui;

  QListWidget *req_list;

  struct requirement *selected;
  struct requirement selected_values;
  void clear_selected();
  void update_selected();

  QToolButton *edit_type_button;
  QToolButton *edit_value_enum_button;
  QMenu *edit_value_enum_menu;
  QLineEdit *edit_value_nbr_field;
  QToolButton *edit_range_button;
  QToolButton *edit_present_button;

private slots:
  void select_req();
  void fill_active();
  void add_now();
  void delete_now();
  void close_now();

  void req_type_menu(QAction *action);
  void req_range_menu(QAction *action);
  void req_present_menu(QAction *action);
  void univ_value_enum_menu(QAction *action);
  void univ_value_edit();

  void incoming_rec_vec_change(const requirement_vector *vec);

protected:
  void closeEvent(QCloseEvent *event);
};

#endif // FC__REQ_EDIT_H
