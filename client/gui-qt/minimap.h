/**************************************************************************
    Copyright (c) 1996-2020 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "mapview_g.h"
// Qt
#include <QFrame>
#include <QLabel>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
// gui-qt is The One
#include "widgetdecorations.h"
class QImage;
class QMouseEvent;
class QMoveEvent;
class QObject;
class QPaintEvent;
class QPainter;
class QPixmap; // lines 26-26
class QResizeEvent;
class QShowEvent;
class QWheelEvent;
class QWidget;

/**************************************************************************
  Thread helper for drawing minimap
**************************************************************************/
class minimap_thread : public QThread {
  Q_OBJECT
public:
  minimap_thread(QObject *parent = 0);
  ~minimap_thread() override;
  void render(double scale_factor, int width, int height);

signals:
  void rendered_image(const QImage &image);

protected:
  void run() Q_DECL_OVERRIDE;

private:
  int mini_width, mini_height;
  double scale;
  QMutex mutex;
  bool threadrestart;
  bool threadabort;
  QWaitCondition condition;
};

/**************************************************************************
  Widget used for displaying overview (minimap)
**************************************************************************/
class minimap_view : public fcwidget {
  Q_OBJECT

public:
  minimap_view(QWidget *parent);
  ~minimap_view() override;
  void paint(QPainter *painter, QPaintEvent *event);
  void update_menu() override;
  void update_image();
  void reset();

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void moveEvent(QMoveEvent *event) override;
  void showEvent(QShowEvent *event) override;

private slots:
  void update_pixmap(const QImage &image);
  void zoom_in();
  void zoom_out();

private:
  void draw_viewport(QPainter *painter);
  void scale(double factor);
  void scale_point(int &x, int &y);
  double scale_factor;
  float w_ratio, h_ratio;
  minimap_thread thread;
  QBrush background;
  QPixmap *pix;
  QPoint cursor;
  QPoint position;
  resize_widget *rw;
};
