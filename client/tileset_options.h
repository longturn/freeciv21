// SPDX-FileCopyrightText: 2024 Louis Moureaux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QString>

#include <map>

class QCheckBox;
struct tileset;

namespace freeciv {

/**
 * Lets the user toggle tileset options.
 */
class tileset_options_dialog : public QDialog {
  std::map<QString, QCheckBox *> m_checks;

public:
  explicit tileset_options_dialog(struct tileset *t, QWidget *parent = 0);

private slots:
  void reset();
};

} // namespace freeciv
