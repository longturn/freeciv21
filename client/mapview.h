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
#include <QPropertyAnimation>
#include <QQueue>
#include <QThread>
#include <QTimer>

// client
#include "shortcuts.h"
#include "tileset_debugger.h"
#include "tilespec.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPainter;

class fcwidget;
namespace freeciv {
class renderer;
}

bool is_point_in_area(int x, int y, int px, int py, int pxe, int pye);
void draw_calculated_trade_routes(QPainter *painter);

/**************************************************************************
  QWidget used for displaying map
**************************************************************************/
class map_view : public QWidget {
  Q_OBJECT
  Q_PROPERTY(
      double scale READ scale WRITE set_scale_now NOTIFY scale_changed);

  // Ought to be a private slot
  friend void debug_tile(tile *tile);

  void shortcut_released(Qt::MouseButton mb);

public:
  map_view();
  void find_place(int pos_x, int pos_y, int &w, int &h, int wdth, int hght,
                  int recursive_nr, bool direction = false);
  void resume_searching(int pos_x, int pos_y, int &w, int &h, int wdtht,
                        int hght, int recursive_nr, bool direction);
  void update_cursor(enum cursor_type);

  void hide_all_fcwidgets();
  void show_all_fcwidgets();

  bool menu_click;

  double scale() const;

  freeciv::tileset_debugger *debugger() const { return m_debugger; }

signals:
  void scale_changed(double scale) const;

public slots:
  void center_on_tile(tile *tile, bool animate = true);

  void zoom_in();
  void zoom_reset();
  void zoom_out();
  void set_scale(double scale);

  void show_debugger();
  void hide_debugger();

  void shortcut_pressed(shortcut_id key);

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void focusOutEvent(QFocusEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private slots:
  void set_scale_now(double scale);
  void timer_event();

private:
  int cursor_frame{0};
  int cursor;
  freeciv::renderer *m_renderer;
  double m_scale = 1;
  std::unique_ptr<QPropertyAnimation> m_origin_animation;
  std::unique_ptr<QPropertyAnimation> m_scale_animation;
  QPointer<freeciv::tileset_debugger> m_debugger = nullptr;
  std::vector<QPointer<fcwidget>> m_hidden_fcwidgets;
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
  static bool shown();

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
};

void popdown_tile_info();
void popup_tile_info(struct tile *ptile);
bool mapview_is_frozen();

void show_city_desc(QPixmap *pcanvas, int canvas_x, int canvas_y,
                    struct city *pcity, int *width, int *height);
