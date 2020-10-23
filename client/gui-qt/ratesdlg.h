/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#ifndef FC__RATESDLG_H
#define FC__RATESDLG_H

// Qt
#include <QDialog>
#include <QList>
// gui-qt
#include "dialogs.h"

class QMouseEvent;
class QObject;
class QPaintEvent;
class QPushButton;
class QSize;
class QSlider;

/**************************************************************************
 * Custom slider with two settable values
 *************************************************************************/
class fc_double_edge : public QWidget {
  Q_OBJECT

private:
  double cursor_size;
  double mouse_x;
  int moved;
  bool on_min;
  bool on_max;
  int max_rates;
  QPixmap cursor_pix;

public:
  fc_double_edge(QWidget *parent = NULL);
  ~fc_double_edge();
  int current_min;
  int current_max;
  QSize sizeHint() const;

protected:
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
};

/**************************************************************************
 * Dialog used to change tax rates
 *************************************************************************/
class tax_rates_dialog : public qfc_dialog {
  Q_OBJECT

public:
  tax_rates_dialog(QWidget *parent = 0);

private:
  fc_double_edge *fcde;
private slots:
  void slot_ok_button_pressed();
  void slot_cancel_button_pressed();
  void slot_apply_button_pressed();
};

/**************************************************************************
 * Dialog used to change policies
 *************************************************************************/
class multipler_rates_dialog : public QDialog {
  Q_OBJECT

public:
  explicit multipler_rates_dialog(QWidget *parent = 0);

private:
  QList<QSlider *> slider_list;
  QPushButton *cancel_button;
  QPushButton *ok_button;
private slots:
  void slot_set_value(int i);
  void slot_ok_button_pressed();
  void slot_cancel_button_pressed();
};

void popup_multiplier_dialog(void);

#endif /* FC__RATESDLG_H */
