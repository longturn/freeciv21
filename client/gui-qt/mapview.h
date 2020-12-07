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
#include <QFrame>
#include <QLabel>
#include <QQueue>
#include <QThread>
#include <QTimer>
// common
#include "tilespec.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;

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

  void hide_all_fcwidgets();
  void show_all_fcwidgets();

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
  Q_DISABLE_COPY(info_tile);

public:
  static info_tile *i(struct tile *p = nullptr);
  static void drop();
  struct tile *itile;

protected:
  void paintEvent(QPaintEvent *event);
  void paint(QPainter *painter, QPaintEvent *event);

private:
  QFont info_font;
  info_tile(struct tile *ptile, QWidget *parent = 0);
  static info_tile *m_instance;
  QStringList str_list;
  void calc_size();
  void update_font(const QString &name, const QFont &font);
};

void popdown_tile_info();
void popup_tile_info(struct tile *ptile);
void mapview_freeze(void);
void mapview_thaw(void);
bool mapview_is_frozen(void);
void pixmap_put_overlay_tile(int canvas_x, int canvas_y,
                             struct sprite *ssprite);

void show_city_desc(struct canvas *pcanvas, int canvas_x,
                           int canvas_y, struct city *pcity, int *width,
                           int *height);

