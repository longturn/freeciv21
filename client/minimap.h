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

  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int width) const override;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void moveEvent(QMoveEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  void draw_viewport(QPainter *painter);
  float w_ratio, h_ratio;
  QBrush background;
  QPixmap *pix;
  QPoint cursor;
  QPoint position;
};
