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
#include <QDialog>
#include <QList>
// gui-qt
#include "dialogs.h"
#include "widgets/multi_slider.h"

class QMouseEvent;
class QObject;
class QPaintEvent;
class QPushButton;
class QSize;
class QSlider;

/**************************************************************************
 * Dialog used to change national budget
 */
class national_budget_dialog : public qfc_dialog {
  Q_OBJECT

public:
  national_budget_dialog(QWidget *parent = 0);

  void refresh();

private:
  freeciv::multi_slider *slider;
  bool slider_init = false;
  QLabel *m_info;

  void apply();
};

/**************************************************************************
 * Dialog used to change policies
 */
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

void popup_multiplier_dialog();
