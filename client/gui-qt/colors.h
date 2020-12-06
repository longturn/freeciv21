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
#include <QWidget>
#include "QColor"

// helper class to get color from qss
class research_color : public QWidget {
  Q_OBJECT
public:
  research_color();
  static research_color *i();
  QColor *get_color(int);
  void init_colors();
private:
  static research_color *m_instance;
  QColor colors[31];
  bool colors_init;
};
