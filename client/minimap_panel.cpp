/*
 * This file is part of Freeciv21.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "minimap_panel.h"

// client
#include "icons.h"
#include "page_game.h"
#include "text.h"
#include "top_bar.h"

/**
 * Constructor.
 */
minimap_panel::minimap_panel(QWidget *parent) : fcwidget(parent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_NoMousePropagation);

  ui.turn_done->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("endturn")));
  connect(ui.turn_done, &QAbstractButton::clicked, top_bar_finish_turn);
}

/**
 * See QWidget::changeEvent
 */
void minimap_panel::changeEvent(QEvent *event)
{
  // Detect theme changes and reset the turn done button icon
  if (event->type() == QEvent::ThemeChange
      || event->type() == QEvent::PaletteChange
      || event->type() == QEvent::ApplicationPaletteChange) {
    // Should eventually move to QStyle::polishWidget?
    ui.turn_done->setIcon(
        fcIcons::instance()->getIcon(QStringLiteral("endturn")));
  }
  QWidget::changeEvent(event);
}

/**
 * Update the timeout display.  The timeout is the time until the turn ends.
 */
void update_timeout_label()
{
  queen()->minimap_panel->turn_done()->setDescription(
      get_timeout_label_text());
}
