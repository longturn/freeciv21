/**************************************************************************
 Copyright (c) 2021 Freeciv21 contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#include "tileset_debugger.h"

// client/include
#include "dialogs_g.h"

// client
#include "climap.h"
#include "editor.h"
#include "tilespec.h"

// common
#include "map.h"
#include "tile.h"

// utility
#include "fcintl.h"

#include <QLabel>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace freeciv {

/**
 * \class tileset_debugger
 * \brief A dialog to perform debugging of the tileset.
 */

/**
 * Constructor.
 */
tileset_debugger::tileset_debugger(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(_("Tileset debugger"));

  auto layout = new QVBoxLayout;
  setLayout(layout);

  auto toolbar = new QToolBar;
  toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  layout->addWidget(toolbar);

  m_pick_action =
      toolbar->addAction(QIcon::fromTheme("pointer"), _("Pick tile"));
  m_pick_action->setToolTip(_("Pick a tile to inspect on the map"));
  m_pick_action->setCheckable(true);
  connect(m_pick_action, &QAction::toggled, this,
          &tileset_debugger::pick_tile);

  m_label = new QLabel;
  layout->addWidget(m_label);

  m_content = new QTreeWidget;
  m_content->setHeaderHidden(true);
  m_content->setSelectionMode(QAbstractItemView::NoSelection);
  layout->addWidget(m_content, 100);

  set_tile(nullptr);
}

/**
 * Destructor.
 */
tileset_debugger::~tileset_debugger() {}

/**
 * Sets the tile being debugged.
 */
void tileset_debugger::set_tile(const ::tile *t)
{
  m_tile = t;
  m_pick_action->setChecked(false);

  // Update the GUI
  if (!t) {
    m_label->setText(_("Select a tile to start debugging."));
    return;
  }

  m_label->setText(QString(_("Tile at %1, %2"))
                       .arg(index_to_map_pos_x(tile_index(t)))
                       .arg(index_to_map_pos_y(tile_index(t))));

  // Fill tile data
  m_content->clear();

  auto maxSize = QSize(); // Max sprite size
  for (const auto &layer : tileset_get_layers(tileset)) {
    auto item = new QTreeWidgetItem(m_content);

    const auto name = mapview_layer_name(layer->type());
    item->setText(0, name);

    // Get the list of sprites for this layer
    ::unit *unit = nullptr;
    if (client_tile_get_known(t) != TILE_UNKNOWN
        || (editor_is_active() && editor_tile_is_selected(t))) {
      unit = get_drawable_unit(tileset, t);
    }
    const auto sprites = layer->fill_sprite_array(t, nullptr, nullptr, unit,
                                                  tile_city(t), nullptr);

    // Add the sprites as children
    for (const auto &ds : sprites) {
      auto child = new QTreeWidgetItem(item);
      child->setIcon(0, QIcon(*ds.sprite));
      maxSize = maxSize.expandedTo(ds.sprite->size());
    }
  }

  m_content->setIconSize(maxSize);
  m_content->expandAll();
}

/**
 * Enters or exits tile picking mode.
 */
void tileset_debugger::pick_tile(bool active)
{
  emit tile_picking_requested(active);
}

} // namespace freeciv
