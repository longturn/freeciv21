/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__MAPVIEW_H
#define FC__MAPVIEW_H

// In this case we have to include fc_config.h from header file.
// Some other headers we include demand that fc_config.h must be
// included also. Usually all source files include fc_config.h, but
// there's moc generated meta_mapview.cpp file which does not.
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "mapview_g.h"

// gui-qt
#include "fonts.h"

// Qt
#include <QFrame>
#include <QLabel>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QTimer>

// Forward declarations
class QMutex;
class QPixmap;

bool is_point_in_area(int x, int y, int px, int py, int pxe, int pye);
void unscale_point(double scale_factor, int &x, int &y);
void draw_calculated_trade_routes(QPainter *painter);

/**************************************************************************
  QWidget used for displaying map
**************************************************************************/
class map_view : public QWidget {
  Q_OBJECT
  void shortcut_pressed(int key);
  void shortcut_released(Qt::MouseButton mb);

public:
  map_view();
  void paint(QPainter *painter, QPaintEvent *event);
  void find_place(int pos_x, int pos_y, int &w, int &h, int wdth, int hght,
                  int recursive_nr);
  void resume_searching(int pos_x, int pos_y, int &w, int &h, int wdtht,
                        int hght, int recursive_nr);
  void update_cursor(enum cursor_type);
  bool menu_click;

protected:
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void focusOutEvent(QFocusEvent *event);
  void leaveEvent(QEvent *event);
private slots:
  void timer_event();

private:
  void update_font(const QString &name, const QFont &font);

  bool stored_autocenter;
  int cursor_frame;
  int cursor;
};

/**************************************************************************
  Information label about clicked tile
**************************************************************************/
class info_tile : public QLabel {
  Q_OBJECT
  QFont info_font;

public:
  info_tile(struct tile *ptile, QWidget *parent = 0);
  struct tile *itile;

protected:
  void paintEvent(QPaintEvent *event);
  void paint(QPainter *painter, QPaintEvent *event);

private:
  QStringList str_list;
  void calc_size();
  void update_font(const QString &name, const QFont &font);
};

void mapview_freeze(void);
void mapview_thaw(void);
bool mapview_is_frozen(void);
void pixmap_put_overlay_tile(int canvas_x, int canvas_y,
                             struct sprite *ssprite);

#endif /* FC__MAPVIEW_H */
