// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include <QFrame>

#include "widgets/ui_conn_loss_widget.h"

namespace freeciv {

class conn_loss_widget : public QFrame { // QFrame base for styling
  Q_OBJECT

public:
  explicit conn_loss_widget(QWidget *parent = nullptr);
  virtual ~conn_loss_widget() = default; ///< Destructor.

private:
  Ui::conn_loss_widget ui;
};

} // namespace freeciv
