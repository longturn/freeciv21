// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
#include <QWidget>

#include "ui_tool_tile.h"
/*
 * Tile tool editor class
 */
class editor_tool_tile : public QWidget {
  Q_OBJECT

private:
  Ui::FormToolTile ui;

public:
  editor_tool_tile(QWidget *parent = 0);
  ~editor_tool_tile() override;
  void select_tile();

protected:
  void showEvent(QShowEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
};
