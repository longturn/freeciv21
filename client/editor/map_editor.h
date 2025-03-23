// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
class QWidget;

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
  void update_players();
  void select_tool_tile();

  bool players_done = false;
  bool ett_wdg_active = false;

  editor_tool_tile *ett_wdg;

private slots:
  void player_changed(int index);

public:
  map_editor(QWidget *parent);
  ~map_editor() override;

  void check_open();
  void tile_selected(struct tile *ptile);

protected:
  void showEvent(QShowEvent *event) override;
};
