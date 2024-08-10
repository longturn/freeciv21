// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "fc_types.h"
#include "layer.h"

#include <array>
#include <map>
#include <utility>
#include <vector>

namespace freeciv {

class layer_roads : public layer {
  /// Stores the data common to all road types.
  struct corner_sprites {
    const extra_type *extra = nullptr;
    QPixmap *isolated = nullptr;
    std::array<QPixmap *, DIR8_MAGIC_MAX> corners = {nullptr};
  };

  /// Helper.
  template <class Sprites> struct data : corner_sprites {
    Sprites sprites;
  };

  /// Data for RoadAllCombined.
  using all_combined_data = data<std::array<QPixmap *, MAX_INDEX_VALID>>;
  /// Data for RoadAllSeparate.
  using all_separate_data = data<std::array<QPixmap *, DIR8_MAGIC_MAX>>;
  /// Data for RoadParityCombined. .first = even, .second = odd
  using parity_combined_data =
      data<std::pair<std::array<QPixmap *, MAX_INDEX_HALF>,
                     std::array<QPixmap *, MAX_INDEX_HALF>>>;

public:
  explicit layer_roads(struct tileset *ts);
  virtual ~layer_roads() = default;

  void initialize_extra(const extra_type *extra, const QString &tag,
                        extrastyle_id style) override;

  std::vector<drawn_sprite>
  fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                    const tile_corner *pcorner,
                    const unit *punit) const override;

  void reset_ruleset() override;

private:
  void initialize_corners(corner_sprites &data, const extra_type *extra,
                          const QString &tag, const terrain *terrain);
  void fill_corners(std::vector<drawn_sprite> &sprs,
                    const corner_sprites &data, const tile *ptile) const;

  void initialize_all_combined(all_combined_data &data, const QString &tag,
                               const terrain *terrain);
  void fill_all_combined(std::vector<drawn_sprite> &sprs,
                         const all_combined_data &data,
                         const tile *ptile) const;

  void initialize_all_separate(all_separate_data &data, const QString &tag,
                               const terrain *terrain);
  void fill_all_separate(std::vector<drawn_sprite> &sprs,
                         const all_separate_data &data,
                         const tile *ptile) const;

  void initialize_parity_combined(parity_combined_data &data,
                                  const QString &tag,
                                  const terrain *terrain);
  void fill_parity_combined(std::vector<drawn_sprite> &sprs,
                            const parity_combined_data &data,
                            const tile *ptile) const;

  // All sprites depend on the terrain (first index) and extra (stored in the
  // data structures, one structure per extra).
  std::vector<std::vector<all_combined_data>> m_all_combined;
  std::vector<std::vector<all_separate_data>> m_all_separate;
  std::vector<std::vector<parity_combined_data>> m_parity_combined;
};

} // namespace freeciv
