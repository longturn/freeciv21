// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_roads.h"

#include "tilespec.h"

/**
 * \class freeciv::layer_roads
 * \brief Draws "road" extras on the map.
 *
 * Road extras are those with style RoadAllSeparate, RoadParityCombined, and
 * RoadAllCombined.
 */

namespace freeciv {

/**
 * Constructor
 */
layer_roads::layer_roads(struct tileset *ts)
    : freeciv::layer(ts, LAYER_ROADS)
{
}

/**
 * Collects all extras to be drawn.
 */
void layer_roads::initialize_extra(const extra_type *extra,
                                   const QString &tag, extrastyle_id style)
{
  if (style == ESTYLE_ROAD_ALL_COMBINED) {
    m_all_combined.push_back(extra);
  } else if (style == ESTYLE_ROAD_ALL_SEPARATE) {
    m_all_separate.push_back(extra);
  } else if (style == ESTYLE_ROAD_PARITY_COMBINED) {
    m_parity_combined.push_back(extra);
  }
}

std::vector<drawn_sprite>
layer_roads::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                               const tile_corner *pcorner,
                               const unit *punit) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);

  // Should we draw anything in the first place?
  /* Future code-dweller beware: We do not fully understand how
   * fill_crossing_sprite_array works for the ESTYLE_ROAD_PARITY_COMBINED
   * sprites. We do know that our recent changes for the terrain-specific
   * extras caused an issue when the client tried to assemble an edge tile
   * where one edge is null. We have found that adding this `if (!ptile)`
   * solved the error, and we will leave it there. Good luck.
   * HF and LM. */
  if (!ptile) {
    return {};
  }

  const auto terrain = tile_terrain(ptile);
  if (!terrain) {
    return {};
  }

  const auto pcity = tile_city(ptile);
  const auto extras = *tile_extras(ptile);

  struct terrain *terrain_near[8] = {nullptr};
  bv_extras extras_near[8];
  build_tile_data(ptile, terrain, terrain_near, extras_near);

  // Now we draw
  std::vector<drawn_sprite> sprs;

  for (const auto *extra : m_all_combined) {
    if (is_extra_drawing_enabled(extra)) {
      fill_crossing_sprite_array(tileset(), extra, sprs, extras, extras_near,
                                 terrain_near, terrain, pcity);
    }
  }
  for (const auto *extra : m_all_separate) {
    if (is_extra_drawing_enabled(extra)) {
      fill_crossing_sprite_array(tileset(), extra, sprs, extras, extras_near,
                                 terrain_near, terrain, pcity);
    }
  }
  for (const auto *extra : m_parity_combined) {
    if (is_extra_drawing_enabled(extra)) {
      fill_crossing_sprite_array(tileset(), extra, sprs, extras, extras_near,
                                 terrain_near, terrain, pcity);
    }
  }

  return sprs;
}

void layer_roads::reset_ruleset()
{
  m_all_combined.clear();
  m_all_separate.clear();
  m_parity_combined.clear();
}

} // namespace freeciv
