// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "layer.h"
#include "utils/colorizer.h"

#include <QPoint>

#include <memory>

class QPixmap;

namespace freeciv {

class layer_city : public layer {
public:
  using styles = std::vector<std::unique_ptr<freeciv::colorizer>>;
  using city_sprite = std::vector<styles>;
  constexpr static int NUM_WALL_TYPES = 7;

  explicit layer_city(struct tileset *ts, const QPoint &city_offset,
                      const QPoint &occupied_offset);
  virtual ~layer_city() = default;

  void initialize_city_style(const citystyle &style, int index) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  std::vector<drawn_sprite>
  fill_sprite_array_no_flag(const city *pcity) const;

  /**
   * Returns an example of what a city might look like.
   */
  QPixmap sample_sprite(int style_index = 0) const
  {
    // Pick the largest size -- looks more interesting.
    return m_tile[style_index].back().get()->base();
  }

  void reset_ruleset() override;

private:
  QPoint m_city_offset, m_occupied_offset;
  QPixmap *m_disorder, *m_happy;

  city_sprite m_tile, m_single_wall, m_occupied;
  std::array<city_sprite, NUM_WALL_TYPES> m_walls;
};

} // namespace freeciv
