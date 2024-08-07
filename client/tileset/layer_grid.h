// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer.h"

#include <array>
#include <memory>

class QPixmap;

namespace freeciv {

class layer_grid : public layer {
public:
  explicit layer_grid(struct tileset *ts, mapview_layer layer);
  virtual ~layer_grid() = default;

  void load_sprites() override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void initialize_player(const player *player) override;
  void free_player(int player_id) override;

private:
  std::array<QPixmap *, EDGE_COUNT> m_city = {nullptr}, m_main = {nullptr},
                                    m_selected = {nullptr},
                                    m_worked = {nullptr};
  QPixmap *m_unavailable, *m_nonnative;

  // Without player colors
  using edge_data = std::array<QPixmap *, 2>;
  std::array<edge_data, EDGE_COUNT> m_basic_borders = {
      edge_data{nullptr, nullptr}};

  // With player colors
  using unique_edge_data = std::array<std::unique_ptr<QPixmap>, 2>;
  /// Indices: [player][edge][in/out]
  std::array<std::array<unique_edge_data, EDGE_COUNT>, MAX_NUM_PLAYER_SLOTS>
      m_borders;
};

} // namespace freeciv
