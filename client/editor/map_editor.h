// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
#include <QWidget>

// client
#include "editor/tool_tile.h"

#include "ui_map_editor.h"
/**
 * Map Editor Dialog Class
 */
class map_editor : public QWidget {
  Q_OBJECT

private:
  Ui::FormMapEditor ui;

  void close();

  // Players
  void update_players();
  bool players_done = false;

  // Tile Tool
  void select_tool_tile();
  editor_tool_tile *ett_wdg;
  bool ett_wdg_active = false;

private slots:
  void player_changed(int index);

public:
  map_editor(QWidget *parent);
  ~map_editor() override;
  void check_open();

protected:
  void showEvent(QShowEvent *event) override;
};
