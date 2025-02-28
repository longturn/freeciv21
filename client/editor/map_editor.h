// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
#include <QLabel>
#include <QWidget>

// client
#include "dialogs.h"

class QEvent;

#include "ui_map_editor.h"
/**
 * Map Editor Dialog Class
 */
class map_editor : public QWidget {
  Q_OBJECT

private:
  Ui::FormMapEditor ui;

private slots:
  void close();

public:
  map_editor(QWidget *parent = 0);
  ~map_editor() override;

protected:
  void showEvent(QShowEvent *event) override;
};
