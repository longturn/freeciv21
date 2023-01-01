/*
 * This file is part of Freeciv21.
 * SPDX-FileCopyrightText: 2022 Louis Moureaux
 * SPDX-FileCopyrightText: 2022 James Robertson
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "minimap_panel.h"

// Qt
#include <QMenu>

// client
#include "icons.h"
#include "mapview.h"
#include "options.h"
#include "overview_common.h"
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

  ui.settings->setIcon(
      fcIcons::instance()->getIcon(QStringLiteral("settings-minimap")));
  setup_minimap_menu();

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
 * Creates the menu with the minimap settings
 */
void minimap_panel::setup_minimap_menu()
{
  auto menu = new QMenu;

  m_show_relief = menu->addAction(_("Show Relief"));
  m_show_relief->setCheckable(true);
  ;
  m_show_relief->setChecked(gui_options->overview.layers[OLAYER_RELIEF]);
  QObject::connect(m_show_relief, &QAction::toggled, [](bool checked) {
    gui_options->overview.layers[OLAYER_RELIEF] = checked;
    refresh_overview_canvas();
  });

  m_show_borders = menu->addAction(_("Show Borders"));
  m_show_borders->setCheckable(true);
  ;
  m_show_borders->setChecked(gui_options->overview.layers[OLAYER_BORDERS]);
  QObject::connect(m_show_borders, &QAction::toggled, [=](bool checked) {
    gui_options->overview.layers[OLAYER_BORDERS] = checked;
    m_show_borders_ocean->setEnabled(checked);
    refresh_overview_canvas();
  });

  m_show_borders_ocean = menu->addAction(_("Show Borders on Oceans"));
  m_show_borders_ocean->setCheckable(true);
  ;
  m_show_borders_ocean->setChecked(
      gui_options->overview.layers[OLAYER_BORDERS_ON_OCEAN]);
  m_show_borders_ocean->setEnabled(
      gui_options->overview.layers[OLAYER_BORDERS]);
  QObject::connect(
      m_show_borders_ocean, &QAction::toggled, [](bool checked) {
        gui_options->overview.layers[OLAYER_BORDERS_ON_OCEAN] = checked;
        refresh_overview_canvas();
      });

  m_show_cities = menu->addAction(_("Show Cities"));
  m_show_cities->setCheckable(true);
  ;
  m_show_cities->setChecked(gui_options->overview.layers[OLAYER_CITIES]);
  QObject::connect(m_show_cities, &QAction::toggled, [](bool checked) {
    gui_options->overview.layers[OLAYER_CITIES] = checked;
    refresh_overview_canvas();
  });

  m_show_cities = menu->addAction(_("Show Units"));
  m_show_cities->setCheckable(true);
  ;
  m_show_cities->setChecked(gui_options->overview.layers[OLAYER_UNITS]);
  QObject::connect(m_show_cities, &QAction::toggled, [](bool checked) {
    gui_options->overview.layers[OLAYER_UNITS] = checked;
    refresh_overview_canvas();
  });

  m_show_fog = menu->addAction(_("Show Fog of War"));
  m_show_fog->setCheckable(true);
  ;
  m_show_fog->setChecked(gui_options->overview.fog);
  QObject::connect(m_show_fog, &QAction::toggled, [](bool checked) {
    gui_options->overview.fog = checked;
    refresh_overview_canvas();
  });

  ui.settings->setMenu(menu);
}

/**
 * Update the timeout display.  The timeout is the time until the turn ends.
 */
void update_timeout_label()
{
  queen()->minimap_panel->turn_done()->update_timeout_label();
}
