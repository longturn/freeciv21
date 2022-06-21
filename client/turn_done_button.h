/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QPushButton>

/**
 * The "Turn Done" button in the main view.
 */
class turn_done_button : public QPushButton {
  Q_OBJECT

public:
  turn_done_button(QWidget *parent = nullptr);

  /// Destructor.
  ~turn_done_button() override = default;

  /// Mimics QCommandLinkButton
  QString description() const { return m_description; }

  /// Mimics QCommandLinkButton
  void setDescription(const QString &description)
  {
    m_description = description;
    update();
  }

  void update_timeout_label();

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QString m_description;
};
