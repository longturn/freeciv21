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

#ifndef FC__EDIT_UTYPE_H
#define FC__EDIT_UTYPE_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>

class QToolButton;

class ruledit_gui;

class edit_utype : public QDialog {
  Q_OBJECT

public:
  explicit edit_utype(ruledit_gui *ui_in, struct unit_type *utype_in);
  void refresh();

private:
  ruledit_gui *ui;
  struct unit_type *utype;
  QToolButton *req_button;

private slots:
  void req_menu(QAction *action);
};

#endif // FC__EDIT_UTYPE_H
