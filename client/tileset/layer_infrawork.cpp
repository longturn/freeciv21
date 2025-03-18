// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

// client/tileset
#include "tilespec.h"

#include "layer_infrawork.h"

/**
 * \class freeciv::layer_infrawork
 * \brief Draws infrastructure (extras) being placed.
 */

namespace freeciv {

/**
 * Constructor
 */
layer_infrawork::layer_infrawork(struct tileset *ts,
                                 const QPoint &activity_offset)
    : freeciv::layer_abstract_activities(ts, LAYER_INFRAWORK),
      m_activity_offset(activity_offset)
{
}

std::vector<drawn_sprite>
layer_infrawork::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                                   const tile_corner *pcorner,
                                   const unit *punit) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);

  // Should we draw anything in the first place?
  if (!ptile || !ptile->placing) {
    return {};
  }

  // Now we draw
  std::vector<drawn_sprite> sprs;

  if (auto sprite = activity_sprite(ACTIVITY_IDLE, ptile->placing)) {
    sprs.emplace_back(tileset(), sprite, true,
                      tileset_full_tile_offset(tileset())
                          + m_activity_offset);
  }

  return sprs;
}

} // namespace freeciv
