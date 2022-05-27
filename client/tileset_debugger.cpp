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

#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
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
  resize(600, 500);

  auto tabs = new QTabWidget;

  auto layout = new QVBoxLayout;
  layout->addWidget(tabs);
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);

  // Log tab
  {
    m_messages = new QListWidget;
    tabs->addTab(m_messages, _("Messages"));

    m_messages->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_messages->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_messages->setTextElideMode(Qt::ElideNone);
    m_messages->setWordWrap(true);
    m_messages->setSizeAdjustPolicy(
        QAbstractItemView::AdjustToContentsOnFirstShow);
    refresh_messages(tileset);
  }

  // Tile inspector tab
  {
    auto tab = new QWidget;
    tabs->addTab(tab, _("Inspector"));
    auto layout = new QVBoxLayout;
    tab->setLayout(layout);

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

    if (sprites.empty()) {
      continue;
    }

    // Generate a sprite with this layer only
    // Geometry
    auto rectangle = QRect();
    for (const auto &ds : sprites) {
      rectangle |= QRect(ds.offset_x, ds.offset_y, ds.sprite->width(),
                         ds.sprite->height());
    }

    // Draw the composite picture
    auto this_layer = QPixmap(rectangle.size() + QSize(2, 2));
    this_layer.fill(Qt::transparent);
    auto p = QPainter();
    p.begin(&this_layer);
    // Outline
    p.setPen(palette().color(QPalette::WindowText));
    p.drawRect(0, 0, this_layer.width() - 1, this_layer.height() - 1);
    // If there are negative offsets, the pixmap was extended in the negative
    // direction. Compensate by offsetting the painter back...
    p.translate(-rectangle.topLeft() + QPoint(1, 1));
    for (const auto &ds : sprites) {
      p.drawPixmap(ds.offset_x, ds.offset_y, *ds.sprite);
    }
    p.end();
    item->setIcon(0, QIcon(this_layer));

    // Add the sprites as children
    for (const auto &ds : sprites) {
      auto child = new QTreeWidgetItem(item);
      auto this_sprite = QPixmap(rectangle.size() + QSize(2, 2));
      this_sprite.fill(Qt::transparent);
      p.begin(&this_sprite);
      // Outline
      p.resetTransform();
      p.setPen(palette().color(QPalette::WindowText));
      p.drawRect(0, 0, this_layer.width() - 1, this_layer.height() - 1);
      // We inherit the translation set above
      p.translate(-rectangle.topLeft() + QPoint(1, 1));
      p.drawPixmap(ds.offset_x, ds.offset_y, *ds.sprite);
      p.end();
      child->setIcon(0, QIcon(this_sprite));
      child->setText(
          0, QString(_("Offset: %1, %2")).arg(ds.offset_x).arg(ds.offset_y));
      maxSize = maxSize.expandedTo(rectangle.size());
    }
  }

  m_content->setIconSize(maxSize);
  m_content->expandAll();
}

/**
 * Enters or exits tile picking mode.
 */
void tileset_debugger::refresh(const struct tileset *t)
{
  // Refresh the tile info, if any
  set_tile(m_tile);

  // Reload messages
  refresh_messages(t);
}

/**
 * Enters or exits tile picking mode.
 */
void tileset_debugger::pick_tile(bool active)
{
  emit tile_picking_requested(active);
}

/**
 * Refresh the messages list
 */
void tileset_debugger::refresh_messages(const struct tileset *t)
{
  fc_assert_ret(t != nullptr);

  m_messages->clear();

  const auto log = tileset_log(t);
  for (std::size_t i = 0; i < log.size(); ++i) {
    auto item = new QListWidgetItem;
    switch (log[i].level) {
    case QtFatalMsg:
    case QtCriticalMsg:
      item->setIcon(QIcon::fromTheme(QStringLiteral("data-error")));
      break;
    case QtWarningMsg:
      item->setIcon(QIcon::fromTheme(QStringLiteral("data-warning")));
      break;
    case QtInfoMsg:
    case QtDebugMsg:
      item->setIcon(QIcon::fromTheme(QStringLiteral("data-information")));
      break;
    }
    item->setText(log[i].message);
    m_messages->addItem(item);
  }
}

} // namespace freeciv
