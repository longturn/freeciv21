/**************************************************************************
  /\ ___ /\        Copyright (c) 1996-2020 ＦＲＥＥＣＩＶ ２１ and Freeciv
 (  o   o  )                 contributors. This file is part of Freeciv21.
  \  >#<  /           Freeciv21 is free software: you can redistribute it
  /       \                    and/or modify it under the terms of the GNU
 /         \       ^      General Public License  as published by the Free
|           |     //  Software Foundation, either version 3 of the License,
 \         /    //                  or (at your option) any later version.
  ///  ///   --                     You should have received a copy of the
                          GNU General Public License along with Freeciv21.
                                  If not, see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "widgetdecorations.h"
#include <QWidget>

class QPixmap;
class close_widget;
struct unit;
class QSize;
class QFont;

/***************************************************************************
 Transparent widget for selecting units
***************************************************************************/
class units_select : public fcwidget {
  Q_OBJECT
  Q_DISABLE_COPY(units_select);
  QPixmap *pix;
  QPixmap *h_pix;          /** pixmap for highlighting */
  QSize item_size;         /** size of each pixmap of unit */
  QList<unit *> unit_list; /** storing units only for drawing, for rest units
                            * iterate utile->units */
  QFont ufont;
  QFont info_font;
  int row_count;
  close_widget *cw;

public:
  units_select(struct tile *ptile, QWidget *parent = 0);
  ~units_select();
  void update_menu();
  void update_units();
  void create_pixmap();
  tile *utile;

protected:
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void closeEvent(QCloseEvent *event);
private slots:
  void update_img();

private:
  bool more;
  int show_line;
  int highligh_num;
  int unit_count;
};

void toggle_unit_sel_widget(struct tile *ptile);
void update_unit_sel();
void popdown_unit_sel();
