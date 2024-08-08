// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_workertask.h"

#include "citydlg_g.h"
#include "tilespec.h"
#include "workertask.h"

/**
 * \class freeciv::layer_workertask
 * \brief Draws tasks assigned by cities to autoworkers on the map.
 */

namespace freeciv {

/**
 * Constructor
 */
layer_workertask::layer_workertask(struct tileset *ts,
                                   const QPoint &activity_offset)
    : freeciv::layer_abstract_activities(ts, LAYER_WORKERTASK),
      m_activity_offset(activity_offset)
{
}

std::vector<drawn_sprite> layer_workertask::fill_sprite_array(
    const tile *ptile, const tile_edge *pedge, const tile_corner *pcorner,
    const unit *punit) const
{
  Q_UNUSED(pedge);
  Q_UNUSED(pcorner);

  // Should we draw anything in the first place?
  const auto city = is_any_city_dialog_open();
  if (!city || !ptile) {
    return {};
  }

  // Now we draw
  std::vector<drawn_sprite> sprs;

  worker_task_list_iterate(city->task_reqs, ptask)
  {
    if (ptask->ptile == ptile) {
      if (auto sprite = activity_sprite(ptask->act, ptask->tgt)) {
        sprs.emplace_back(tileset(), sprite, true,
                          tileset_full_tile_offset(tileset())
                              + m_activity_offset);
      }
    }
  }
  worker_task_list_iterate_end;

  return sprs;
}

} // namespace freeciv
