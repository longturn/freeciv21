/*
 * This file is part of Freeciv21.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "minimap_panel.h"

// Qt
#include <QApplication>
#include <QGridLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

// client
#include "client_main.h"
#include "icons.h"
#include "mapview.h"
#include "page_game.h"
#include "top_bar.h"

/**
 * Constructor.
 */
minimap_panel::minimap_panel(map_view *map, QWidget *parent)
{
  setParent(parent);
  ui.setupUi(this);
  setAttribute(Qt::WA_NoMousePropagation);
  setMinimumSize(100, 100);
  setResizable(Qt::LeftEdge | Qt::TopEdge);

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
 * Shows or hides the minimap
 */
void minimap_panel::set_minimap_visible(bool visible)
{
  ui.minimap->setVisible(visible);
  ui.zoom_in->setVisible(visible);
  ui.zoom_reset->setVisible(visible);
  ui.zoom_out->setVisible(visible);
  ui.spacer->changeSize(0, 0, QSizePolicy::Minimum,
                        visible ? QSizePolicy::Expanding
                                : QSizePolicy::Minimum);
  // See documentation for QSpacerItem::changeSize
  ui.verticalLayout->invalidate();

  // This isn't properly propagated by the map view. One day we'll have a
  // proper QLayout...
  QApplication::postEvent(queen()->game_tab_widget,
                          new QEvent(QEvent::LayoutRequest));
}

/**
 * Update the timeout display.  The timeout is the time until the turn ends.
 */
void update_timeout_label()
{
  queen()->minimap_panel->turn_done()->update_timeout_label();
}
