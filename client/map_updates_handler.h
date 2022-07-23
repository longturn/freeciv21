/*
 * SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>
 *
 * SPDX-License-Identifier: GPLv3-or-later
 */

#pragma once

#include <map>

#include <QObject>

#include "listener.h"

struct city;
struct tile;
struct unit;

namespace freeciv {

class map_updates_handler : public QObject,
                            public listener<map_updates_handler> {
  Q_OBJECT

public:
  /**
   * What kind of update should be performed
   */
  enum class update_type {
    tile_single = 0x01,
    tile_full = 0x02,
    unit = 0x04,
    city_description = 0x08,
    city_map = 0x10,
    tile_label = 0x20,
  };
  Q_DECLARE_FLAGS(updates, update_type)

  explicit map_updates_handler(QObject *parent = nullptr);
  virtual ~map_updates_handler() = default;

  /// Allows iteration on the list of pending updates.
  auto begin() const { return m_updates.begin(); }

  /// Allows iteration on the list of pending updates.
  auto end() const { return m_updates.begin(); }

  void clear();

  void update(const city *city, bool full);
  void update(const tile *tile, bool full);
  void update(const unit *unit, bool full);
  void update_all();
  void update_city_description(const city *city);
  void update_tile_label(const tile *tile);

private:
  bool m_full_update = false;
  std::map<const tile *, updates> m_updates;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(map_updates_handler::updates)

} // namespace freeciv
