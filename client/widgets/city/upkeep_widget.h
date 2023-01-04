/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <QListView>
#include <QStandardItemModel>

namespace freeciv {

class upkeep_widget : public QListView {
  Q_OBJECT;

public:
  explicit upkeep_widget(QWidget *parent = nullptr);

  void refresh();
  void set_city(int city_id);

  QSize viewportSizeHint() const override;
  QSize minimumSizeHint() const override;

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  bool event(QEvent *event) override;

private:
  int m_city = -1;
  QStandardItemModel *m_model;
};

} // namespace freeciv
