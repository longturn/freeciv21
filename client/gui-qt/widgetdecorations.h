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

#include <QFrame>
#include <QLabel>
#include <QRubberBand>

/**************************************************************************
  Widget allowing resizing other widgets
**************************************************************************/
class resize_widget : public QLabel {
  Q_OBJECT

public:
  resize_widget(QWidget *parent);
  void put_to_corner();

protected:
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  QPoint point;
};

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
  virtual void update_menu() = 0;
  bool was_destroyed;
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
