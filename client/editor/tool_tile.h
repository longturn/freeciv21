// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

#pragma once

// common
#include "packets.h"

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
  QString get_tile_extra_text(const tile *ptile,
                              const std::vector<extra_cause> &causes) const;
  QPixmap get_tile_sprites(const struct tile *ptile) const;

public:
  editor_tool_tile(QWidget *parent = nullptr);
  ~editor_tool_tile() override;

  void close_tool();
  void set_default_values();
  void update_ett(struct tile *ptile);
};

bool check_tile_tool();
