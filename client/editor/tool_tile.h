// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
class QWidget;

#include "ui_tool_tile.h"
/*
 * Tile tool editor class
 */
class editor_tool_tile : public QWidget {
  Q_OBJECT

private:
  Ui::FormToolTile ui;

  void select_tile();
  void set_default_values();

public:
  editor_tool_tile(QWidget *parent = nullptr);
  ~editor_tool_tile() override;

  void close_tool();
  void update_ett(struct tile *ptile);
};

bool check_tile_tool();
