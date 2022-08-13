/*  .'       '.  Copyright (c) 1996-2020 Freeciv21 and Freeciv   ********
    \\  .  . \ \            contributors. This file is part of Freeciv21.
    \ \ o  o  \ \     Freeciv21 is free software: you can redistribute it
     \ . \/ \/ \ \            and/or modify it under the terms of the GNU
       \/    , . \\      General Public License  as published by the Free
        ' ,. '(  .\\ Software Foundation, either version 3 of the License,
        //``\\ . | \                or (at your option) any later version.
~~~~~'''''~''''''~~~\~             You should have received a copy of the
              ((          GNU General Public License along with Freeciv21.
               \\               If not, see https://www.gnu.org/licenses/.
                ))
*************   V   ******************************************************/
#pragma once

#include <QFlags>
#include <QFrame>
#include <QLabel>
#include <QRubberBand>

/**************************************************************************
  Widget allowing moving other widgets
**************************************************************************/
class move_widget : public QLabel {
  Q_OBJECT
public:
  move_widget(QWidget *parent);
  void put_to_corner();

protected:
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  QPoint point;
};

/****************************************************************************
  Widget for resizing other widgets
****************************************************************************/
class scale_widget : public QRubberBand {
  Q_OBJECT

public:
  scale_widget(Shape s, QWidget *p = 0);
  float scale;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  int size;
  QPixmap plus;
  QPixmap minus;
};

/**************************************************************************
  Abstract class for widgets wanting to do custom action
  when closing widgets is called (eg. update menu)
**************************************************************************/
class fcwidget : public QFrame {
  Q_OBJECT

public:
  /// Constructor
  explicit fcwidget(QWidget *parent = nullptr) : QFrame(parent) {}

  virtual void update_menu() = 0;
};

/**************************************************************************
  Abstract class for widgets that can be resized by dragging the edges.
**************************************************************************/
class resizable_widget : public fcwidget {
  Q_OBJECT

  static constexpr int event_width = 25;
  static constexpr int margin_width = 3;

signals:
  void resized(QRect rect);

public:
  // make widget resizable (multiple flags supported)
  void setResizable(Qt::Edges edges);

  // get resizable flags
  Qt::Edges getResizable() const;

  // remove resizable for widget
  void removeResizable();

private:
  Qt::Edges get_in_event_mouse(const QMouseEvent *event) const;

  QPoint last_position{};
  Qt::Edges eventFlags{};
  Qt::Edges resizeFlags{};

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
};

/**************************************************************************
  Widget allowing closing other widgets
**************************************************************************/
class close_widget : public QLabel {
  Q_OBJECT

public:
  close_widget(QWidget *parent);
  void put_to_corner();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void notify_parent();
};
