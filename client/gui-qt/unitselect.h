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

#include <QMenu>

class QPixmap;
class QSize;
class QFont;

struct unit;

/***************************************************************************
 Transparent widget for selecting units
***************************************************************************/
class units_select : public QMenu {
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

public:
  units_select(struct tile *ptile, QWidget *parent = 0);
  ~units_select() override;
  void update_units();
  void create_pixmap();
  tile *utile;

protected:
  void paint(QPainter *painter, QPaintEvent *event);
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  bool more;
  int show_line;
  int highligh_num;
  int unit_count;
};

void toggle_unit_sel_widget(struct tile *ptile);
void update_unit_sel();
void popdown_unit_sel();
