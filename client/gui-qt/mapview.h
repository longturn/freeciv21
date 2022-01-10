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
#include <QPointer>
#include <QQueue>
#include <QThread>
#include <QTimer>

// gui-qt
#include "tileset_debugger.h"

// common
#include "tilespec.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;

class fcwidget;

bool is_point_in_area(int x, int y, int px, int py, int pxe, int pye);
void unscale_point(double scale_factor, int &x, int &y);
void draw_calculated_trade_routes(QPainter *painter);

/**************************************************************************
  QWidget used for displaying map
**************************************************************************/
class map_view : public QWidget {
  Q_OBJECT

  // Ought to be a private slot
  friend void debug_tile(tile *tile);

  void shortcut_pressed(int key);
  void shortcut_released(Qt::MouseButton mb);

public:
  map_view();
  void paint(QPainter *painter, QPaintEvent *event);
  void find_place(int pos_x, int pos_y, int &w, int &h, int wdth, int hght,
                  int recursive_nr, bool direction = false);
  void resume_searching(int pos_x, int pos_y, int &w, int &h, int wdtht,
                        int hght, int recursive_nr, bool direction);
  void update_cursor(enum cursor_type);

  void hide_all_fcwidgets();
  void show_all_fcwidgets();

  bool menu_click;

  freeciv::tileset_debugger *debugger() const { return m_debugger; }

public slots:
  void show_debugger();
  void hide_debugger();

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void leaveEvent(QEvent *event) override;
private slots:
  void timer_event();

private:
  void update_font(const QString &name, const QFont &font);

  bool stored_autocenter;
  int cursor_frame{0};
  int cursor;
  QPointer<freeciv::tileset_debugger> m_debugger = nullptr;
  std::vector<fcwidget *> m_hidden_fcwidgets;
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
  void paintEvent(QPaintEvent *event) override;
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
void mapview_freeze();
void mapview_thaw();
bool mapview_is_frozen();
void pixmap_put_overlay_tile(int canvas_x, int canvas_y, QPixmap *ssprite);

void show_city_desc(QPixmap *pcanvas, int canvas_x, int canvas_y,
                    struct city *pcity, int *width, int *height);
