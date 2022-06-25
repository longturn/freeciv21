/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include <QPainter>
// gui-qt
#include "colors.h"
#include "colors_g.h"

research_color *research_color::m_instance = 0;

research_color::research_color() {}

research_color *research_color::i()
{
  if (!m_instance) {
    m_instance = new research_color;
  }
  return m_instance;
}

void research_color::init_colors()
{
  colors[0] = QColor(palette().color(QPalette::Highlight));
  // 4 darker
  colors[1] = QColor(palette().color(QPalette::Highlight)).lighter(20);
  colors[2] = QColor(palette().color(QPalette::Highlight)).lighter(40);
  colors[3] = QColor(palette().color(QPalette::Highlight)).lighter(60);
  colors[4] = QColor(palette().color(QPalette::Highlight)).lighter(80);
  // 4 lighter
  colors[5] = QColor(palette().color(QPalette::Highlight)).lighter(120);
  colors[6] = QColor(palette().color(QPalette::Highlight)).lighter(140);
  colors[7] = QColor(palette().color(QPalette::Highlight)).lighter(120);
  // tech goal - green boost
  colors[7].setRgb(colors[0].red() * 0.9,
                   qMax(static_cast<int>(colors[0].green() * 1.1), 254),
                   colors[0].blue() * 0.9);
  colors[8] = QColor(palette().color(QPalette::Highlight)).lighter(180);
  colors[9] = QColor(colors[0]);
  // red boost - current tech
  colors[9].setRgb(qMax(static_cast<int>(colors[9].red() * 1.1), 254),
                   colors[9].green() * 0.9, colors[9].blue() * 0.9);
  colors[10] = QColor(palette().color(QPalette::AlternateBase));
  colors[20] = QColor(palette().color(QPalette::Text));
  colors[21] = QColor(palette().color(QPalette::Text)).lighter(20);
  colors[22] = QColor(palette().color(QPalette::Text)).lighter(40);
  colors[23] = QColor(palette().color(QPalette::Text)).lighter(60);
  colors[24] = QColor(palette().color(QPalette::Text)).lighter(80);
  // 4 lighter
  colors[25] = QColor(palette().color(QPalette::Text)).lighter(120);
  colors[26] = QColor(palette().color(QPalette::Text)).lighter(140);
  colors[27] = QColor(palette().color(QPalette::Text)).lighter(160);
  colors[28] = QColor(palette().color(QPalette::Text)).lighter(180);
  colors[30] = QColor(0, 0, 0);
  colors_init = true;
}
// get color from fake object
QColor research_color::get_color(int c)
{
  if (!colors_init) {
    init_colors();
  }
  return colors[c];
}

// returns color from qss fake object
QColor get_diag_color(int c) { return research_color::i()->get_color(c); }
