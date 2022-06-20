/*
 * This file is part of Freeciv21.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui_minimap_panel.h"
#include "widgetdecorations.h"

/**
 * The panel at the bottom right of the game screen, holding the minimap and
 * the Turn Done button.
 */
class minimap_panel : public fcwidget {
  Q_OBJECT
public:
  explicit minimap_panel(QWidget *parent = nullptr);

  /// Destructor.
  virtual ~minimap_panel() override = default;

  void update_menu() override {}

  /// Retrieves the minimap widget.
  auto minimap() { return ui.minimap; }

  /// Retrieves the Turn Done button.
  auto turn_done() { return ui.turn_done; }

private:
  Ui::minimap_panel ui;
};

void update_timeout_label();
