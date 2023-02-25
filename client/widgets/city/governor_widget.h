// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "governor.h"
#include "widgets/city/ui_governor_widget.h"

namespace freeciv {

class governor_widget : public QWidget {
  Q_OBJECT
  Ui::governor_widget ui;

public:
  explicit governor_widget(QWidget *parent = nullptr);
  virtual ~governor_widget() = default; ///< Destructor.

  cm_parameter parameters() const;
  void set_parameters(const cm_parameter &params);

signals:
  void parameters_changed(const cm_parameter &params);

private:
  void emit_params_changed();
};

} // namespace freeciv
