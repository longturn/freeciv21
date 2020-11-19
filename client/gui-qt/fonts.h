/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// Qt
#include <QMap>

class QFont;
class QStringList;

namespace fonts {
const char *const default_font = "gui_qt_font_default";
const char *const notify_label = "gui_qt_font_notify_label";
const char *const help_label = "gui_qt_font_help_label";
const char *const help_text = "gui_qt_font_help_text";
const char *const chatline = "gui_qt_font_chatline";
const char *const city_names = "gui_qt_font_city_names";
const char *const city_productions = "gui_qt_font_city_productions";
const char *const reqtree_text = "gui_qt_font_reqtree_text";
} // namespace fonts

class fc_font {
  Q_DISABLE_COPY(fc_font);

private:
  QMap<QString, QFont *> font_map;
  static fc_font *m_instance;
  explicit fc_font();

public:
  static fc_font *instance();
  static void drop();
  void set_font(const QString &name, QFont *qf);
  void set_size_all(int);
  QFont *get_font(const QString &name);
  void init_fonts();
  void release_fonts();
  void get_mapfont_size();
  int city_fontsize;
  int prod_fontsize;
};

void configure_fonts();
QString configure_font(const QString &font_name, const QStringList &sl, int size,
                       bool bold = false);
