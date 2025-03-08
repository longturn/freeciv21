// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
#include <QStackedWidget>

// client
#include "dialogs.h"
#include "editor/tool_tile.h"

#include "ui_map_editor.h"
/**
 * Map Editor Dialog Class
 */
class map_editor : public QStackedWidget {
  Q_OBJECT

private:
  Ui::FormMapEditor ui;

private slots:
  void close();

public:
  map_editor(QWidget *parent = 0);
  ~map_editor() override;
  void check_open();
  void update_players();
  bool players_done = false;
  void select_tool_tile();

  editor_tool_tile *ett_wdg;
  bool ett_wdg_active = false;

protected:
  void showEvent(QShowEvent *event) override;
};
