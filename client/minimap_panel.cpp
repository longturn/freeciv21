/*
 * This file is part of Freeciv21.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "minimap_panel.h"

// client
#include "icons.h"
#include "mapview.h"
#include "page_game.h"
#include "text.h"
#include "top_bar.h"

/**
 * Constructor.
 */
minimap_panel::minimap_panel(map_view *map, QWidget *parent)
    : fcwidget(parent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_NoMousePropagation);

  ui.zoom_in->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("zoom-in")));
  connect(ui.zoom_in, &QAbstractButton::clicked, map, &map_view::zoom_in);

  ui.zoom_reset->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("zoom-original")));
  connect(ui.zoom_reset, &QAbstractButton::clicked, map,
          &map_view::zoom_reset);

  ui.zoom_out->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("zoom-out")));
  connect(ui.zoom_out, &QAbstractButton::clicked, map, &map_view::zoom_out);

  connect(ui.turn_done, &QAbstractButton::clicked, top_bar_finish_turn);
}

/**
 * Update the timeout display.  The timeout is the time until the turn ends.
 */
void update_timeout_label()
{
  queen()->minimap_panel->turn_done()->update_timeout_label();
}
