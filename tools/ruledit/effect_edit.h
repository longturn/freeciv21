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

#ifndef FC__EFFECT_EDIT_H
#define FC__EFFECT_EDIT_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QListWidget>
#include <QToolButton>

// common
#include "requirements.h"

class QSpinBox;

class ruledit_gui;

enum effect_filter_main_class {
  EFMC_NORMAL,
  EFMC_NONE, /* No requirements */
  EFMC_ALL   /* Any requirements */
};

struct effect_list_fill_data {
  struct universal *filter;
  enum effect_filter_main_class efmc;
  class effect_edit *edit;
  int num;
};

class effect_edit : public QDialog {
  Q_OBJECT

public:
  explicit effect_edit(ruledit_gui *ui_in, QString target,
                       struct universal *filter_in,
                       enum effect_filter_main_class efmc_in);
  ~effect_edit();
  void refresh();
  void add(const char *msg);
  void add_effect_to_list(struct effect *peffect,
                          struct effect_list_fill_data *data);

  struct universal *filter_get();

  enum effect_filter_main_class efmc;

private:
  ruledit_gui *ui;

  QString name;
  QListWidget *list_widget;
  struct universal filter;
  struct effect_list *effects;

  struct effect *selected;
  int selected_nbr;

  QToolButton *edit_type_button;
  QSpinBox *value_box;

private slots:
  void select_effect();
  void fill_active();
  void edit_reqs();
  void close_now();

  void effect_type_menu(QAction *action);
  void set_value(int value);

protected:
  void closeEvent(QCloseEvent *event);
};

#endif // FC__EFFECT_EDIT_H
