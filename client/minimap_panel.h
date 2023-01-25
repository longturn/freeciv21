/*
 * This file is part of Freeciv21.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux
 * SPDX-FileCopyrightText: 2022-2023 James Robertson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "ui_minimap_panel.h"

// qt-client is one true king
#include "widgets/decorations.h"

class QAction;

class map_view;

/**
 * The panel at the bottom right of the game screen, holding the minimap and
 * the Turn Done button.
 */
class minimap_panel : public resizable_widget {
  Q_OBJECT
public:
  explicit minimap_panel(map_view *map, QWidget *parent = nullptr);

  /// Destructor.
  virtual ~minimap_panel() = default;

  void update_menu() override {}

  void set_minimap_visible(bool visible);

  /// Retrieves the minimap widget.
  auto minimap() { return ui.minimap; }

  /// Retrieves the Turn Done button.
  auto turn_done() { return ui.turn_done; }

private:
  void setup_minimap_menu();

  Ui::minimap_panel ui;

  QAction *m_show_relief, *m_show_borders, *m_show_borders_ocean,
      *m_show_cities, *m_show_fog;
};

void update_timeout_label();
