/*
 * SPDX-FileCopyrightText: 2023 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#include "upkeep_widget.h"

// common
#include "game.h"

// client
#include "client_main.h"
#include "climisc.h"
#include "mapview_common.h"
#include "text.h"
#include "tilespec.h"
#include "tooltips.h"

#include <QPainter>

namespace freeciv {

/**
 * \class upkeep_widget
 *
 * Displays the list of items supported by a city (improvements and units).
 */

/**
 * Constructor
 */
upkeep_widget::upkeep_widget(QWidget *parent)
    : QListView(parent), m_model(new QStandardItemModel(this))
{
  setModel(m_model);

  setEditTriggers(NoEditTriggers);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSelectionMode(NoSelection);
  setSizeAdjustPolicy(AdjustToContents);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setVerticalScrollMode(ScrollPerPixel);
}

/**
 * Updates the widget from the city.
 */
void upkeep_widget::refresh()
{
  m_model->clear();
  const auto city = game_city_by_number(m_city);
  if (!city) {
    return;
  }

  // We assume that improvement icons are at most as wide as the city sprite
  // Otherwise they will be cropped.
  const auto icon_width = tileset_unit_width(get_tileset());

  // Improvements and wonders
  {
    universal targets[MAX_NUM_PRODUCTION_TARGETS];
    const auto targets_used = collect_already_built_targets(targets, city);

    item items[MAX_NUM_PRODUCTION_TARGETS];
    name_and_sort_items(targets, targets_used, items, false, city);

    for (int i = 0; i < targets_used; i++) {
      const auto *building = items[i].item.value.building;
      const auto &sprite = *get_building_sprite(tileset, building);

      // Center the sprite
      auto rect = QRect(QPoint(), sprite.size());
      rect.moveCenter(QPoint(icon_width / 2, sprite.height() / 2));

      QPixmap pixmap(icon_width, sprite.height());
      pixmap.fill(Qt::transparent);
      QPainter p(&pixmap);
      p.drawPixmap(rect, sprite);
      p.end();

      auto item = new QStandardItem;
      item->setData(pixmap, Qt::DecorationRole);
      item->setEditable(false);
      item->setToolTip(
          get_tooltip_improvement(building, city, true).trimmed());
      m_model->appendRow(item);
    }
  }

  // Units
  {
    const bool playing = (client.conn.playing == nullptr
                          || city_owner(city) == client.conn.playing);
    const auto units =
        playing ? city->units_supported : city->client.info_units_supported;

    unit_list_iterate(units, unit)
    {
      auto pixmap = QPixmap(icon_width,
                            tileset_unit_with_upkeep_height(get_tileset()));
      pixmap.fill(Qt::transparent);
      put_unit(unit, &pixmap, 0, 0);

      auto free_unhappy = get_city_bonus(city, EFT_MAKE_CONTENT_MIL);
      const auto happy_cost = city_unit_unhappiness(unit, &free_unhappy);
      put_unit_city_overlays(unit, &pixmap, 0,
                             tileset_unit_layout_offset_y(get_tileset()),
                             unit->upkeep, happy_cost);

      auto *item = new QStandardItem();
      item->setData(pixmap, Qt::DecorationRole);
      item->setEditable(false);
      item->setToolTip(unit_description(unit));
      m_model->appendRow(item);
    }
    unit_list_iterate_end;
  }
}

/**
 * Changes the city displayed by this widget
 */
void upkeep_widget::set_city(int city_id)
{
  if (city_id != m_city) {
    m_city = city_id;
    refresh();
  }
}

/**
 * Reimplemented to provide a meaningful size hint.
 */
QSize upkeep_widget::viewportSizeHint() const
{
  int height = 0;
  for (int i = 0; i < m_model->rowCount(); ++i) {
    height += sizeHintForRow(i);
  }
  // The 6 extra pixels seem to come from the style
  return QSize(6 + sizeHintForColumn(0), height);
}

/**
 * Reimplemented to allow for tiny tilesets.
 */
QSize upkeep_widget::minimumSizeHint() const { return QSize(0, 0); }

/**
 * Reimplemented protected function.
 */
bool upkeep_widget::event(QEvent *event)
{
  if (event->type() == TilesetChanged) {
    refresh();
    return true;
  }
  return QListView::event(event);
}

} // namespace freeciv
