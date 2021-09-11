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

// common
#include "map.h"
#include "tile.h"

// utility
#include "fcintl.h"

#include <QLabel>
#include <QToolBar>
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
  layout->addWidget(m_label, 100, Qt::AlignCenter);

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

  m_label->setText(QStringLiteral("%1 %2")
                       .arg(index_to_map_pos_x(tile_index(t)))
                       .arg(index_to_map_pos_y(tile_index(t))));
}

/**
 * Enters or exits tile picking mode.
 */
void tileset_debugger::pick_tile(bool active)
{
  emit tile_picking_requested(active);
}

} // namespace freeciv
