/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QWidget>

namespace freeciv {

class city_icon_widget : public QWidget {
  Q_OBJECT;

public:
  explicit city_icon_widget(QWidget *parent = nullptr);

  void set_city(int city_id);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  bool event(QEvent *event) override;

private:
  int m_city = -1;
};

} // namespace freeciv
