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

#ifndef FC__REQUIRERS_DLG_H
#define FC__REQUIRERS_DLG_H

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QDialog>
#include <QTextEdit>

class ruledit_gui;

class requirers_dlg : public QDialog {
  Q_OBJECT

public:
  explicit requirers_dlg(ruledit_gui *ui_in);
  void clear(const char *title);
  void add(const char *msg);

private:
  ruledit_gui *ui;

  QTextEdit *area;

private slots:
  void close_now();
};

#endif // FC__REQUIRERS_DIALOG_H
