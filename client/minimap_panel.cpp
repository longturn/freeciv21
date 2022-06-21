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

  connect(ui.turn_done, &QAbstractButton::clicked, top_bar_finish_turn);
}

/**
 * Update the timeout display.  The timeout is the time until the turn ends.
 */
void update_timeout_label()
{
  queen()->minimap_panel->turn_done()->update_timeout_label();
}
