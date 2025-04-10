// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// Qt
class QWidget;
class QString;
class QPixmap;

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

  QString get_tile_road_name(const struct tile *ptile) const;
  QString get_tile_infra_name(const struct tile *ptile) const;
  QString get_tile_nuisance_name(const struct tile *ptile) const;
  QString get_tile_hut_name(const struct tile *ptile) const;
  QString get_tile_base_name(const struct tile *ptile) const;
  QPixmap get_tile_sprites(const struct tile *ptile) const;

public:
  editor_tool_tile(QWidget *parent = nullptr);
  ~editor_tool_tile() override;

  void close_tool();
  void update_ett(struct tile *ptile);
};

bool check_tile_tool();
