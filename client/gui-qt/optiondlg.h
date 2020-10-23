/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__OPTIONDLG_H
#define FC__OPTIONDLG_H

// Qt
#include <QDialog>
#include <QMap>
// client
#include "optiondlg_g.h"
// qt-client
#include "dialogs.h"

struct option_set;

QString split_text(const QString &text, bool cut);
QString cut_helptext(const QString &text);
/****************************************************************************
  Dialog for client/server options
****************************************************************************/
class option_dialog : public qfc_dialog {
  Q_OBJECT
  QVBoxLayout *main_layout;
  QTabWidget *tab_widget;
  QDialogButtonBox *button_box;
  QList<QString> categories;
  QMap<QString, QWidget *> widget_map;

public:
  option_dialog(const QString &name, const option_set *options,
                QWidget *parent = 0);
  ~option_dialog();
  void fill(const struct option_set *poptset);
  void add_option(struct option *poption);
  void option_dialog_refresh(struct option *poption);
  void option_dialog_reset(struct option *poption);
  void full_refresh();
  void apply_options();

private:
  const option_set *curr_options;
  void set_bool(struct option *poption, bool value);
  void set_int(struct option *poption, int value);
  void set_string(struct option *poption, const char *string);
  void set_enum(struct option *poption, int index);
  void set_bitwise(struct option *poption, unsigned value);
  void set_color(struct option *poption, struct ft_color color);
  void set_font(struct option *poption, const QString &s);
  void get_color(struct option *poption, QByteArray &a1, QByteArray &a2);
  bool get_bool(struct option *poption);
  int get_int(struct option *poption);
  QFont get_font(struct option *poption);
  QByteArray get_button_font(struct option *poption);
  QByteArray get_string(struct option *poption);
  int get_enum(struct option *poption);
  struct option *get_color_option();
  unsigned get_bitwise(struct option *poption);
  void full_reset();
private slots:
  void apply_option(int response);
  void set_color();
  void set_font();
};

#endif /* FC__OPTIONDLG_H */
