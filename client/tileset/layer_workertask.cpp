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
layer_workertask::layer_workertask(struct tileset *ts, mapview_layer layer,
                                   const QPoint &activity_offset)
    : freeciv::layer(ts, layer), m_activity_offset(activity_offset)
{
}

/**
 * Loads all static sprites needed by this layer (activities etc).
 */
void layer_workertask::load_sprites()
{
  m_activities[ACTIVITY_MINE] = load_sprite(tileset(), {"unit.plant"}, true);
  m_activities[ACTIVITY_PLANT] = m_activities[ACTIVITY_MINE];
  m_activities[ACTIVITY_IRRIGATE] =
      load_sprite(tileset(), {"unit.irrigate"}, true);
  m_activities[ACTIVITY_CULTIVATE] = m_activities[ACTIVITY_IRRIGATE];
  m_activities[ACTIVITY_PILLAGE] =
      load_sprite(tileset(), {"unit.pillage"}, true);
  m_activities[ACTIVITY_FORTIFIED] =
      load_sprite(tileset(), {"unit.fortified"}, true);
  m_activities[ACTIVITY_FORTIFYING] =
      load_sprite(tileset(), {"unit.fortifying"}, true);
  m_activities[ACTIVITY_SENTRY] =
      load_sprite(tileset(), {"unit.sentry"}, true);
  m_activities[ACTIVITY_GOTO] = load_sprite(tileset(), {"unit.goto"}, true);
  m_activities[ACTIVITY_TRANSFORM] =
      load_sprite(tileset(), {"unit.transform"}, true);
  m_activities[ACTIVITY_CONVERT] =
      load_sprite(tileset(), {"unit.convert"}, true);
}

/**
 * Loads the extra activity and remove activity sprites.
 */
void layer_workertask::initialize_extra(const extra_type *extra,
                                        const QString &tag,
                                        extrastyle_id style)
{
  fc_assert(extra->id == m_extra_activities.size());

  if (!fc_strcasecmp(extra->activity_gfx, "none")) {
    m_extra_activities.push_back(nullptr);
  } else {
    QStringList tags = {
        extra->activity_gfx,
        extra->act_gfx_alt,
        extra->act_gfx_alt2,
    };
    m_extra_activities.push_back(load_sprite(tileset(), tags, true));
  }

  if (!fc_strcasecmp(extra->rmact_gfx, "none")) {
    m_extra_rm_activities.push_back(nullptr);
  } else {
    QStringList tags = {
        extra->rmact_gfx,
        extra->rmact_gfx_alt,
    };
    m_extra_rm_activities.push_back(load_sprite(tileset(), tags, true));
  }
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

  // All sprites drawn in this function are for activities
  const auto offset =
      tileset_full_tile_offset(tileset()) + m_activity_offset;

  worker_task_list_iterate(city->task_reqs, ptask)
  {
    if (ptask->ptile == ptile) {
      switch (ptask->act) {
      case ACTIVITY_MINE:
        if (ptask->tgt == nullptr) {
          sprs.emplace_back(tileset(), m_activities[ACTIVITY_PLANT], true,
                            offset);
        } else {
          sprs.emplace_back(tileset(),
                            m_extra_activities[extra_index(ptask->tgt)],
                            true, offset);
        }
        break;
      case ACTIVITY_PLANT:
        sprs.emplace_back(tileset(), m_activities[ACTIVITY_PLANT], true,
                          offset);
        break;
      case ACTIVITY_IRRIGATE:
        if (ptask->tgt == nullptr) {
          sprs.emplace_back(tileset(), m_activities[ACTIVITY_IRRIGATE], true,
                            offset);
        } else {
          sprs.emplace_back(tileset(),
                            m_extra_activities[extra_index(ptask->tgt)],
                            true, offset);
        }
        break;
      case ACTIVITY_CULTIVATE:
        sprs.emplace_back(tileset(), m_activities[ACTIVITY_IRRIGATE], true,
                          offset);
        break;
      case ACTIVITY_GEN_ROAD:
        sprs.emplace_back(tileset(),
                          m_extra_activities[extra_index(ptask->tgt)], true,
                          offset);
        break;
      case ACTIVITY_TRANSFORM:
        sprs.emplace_back(tileset(), m_activities[ACTIVITY_TRANSFORM], true,
                          offset);
        break;
      case ACTIVITY_POLLUTION:
      case ACTIVITY_FALLOUT:
        sprs.emplace_back(tileset(),
                          m_extra_rm_activities[extra_index(ptask->tgt)],
                          true, offset);
        break;
      default:
        break;
      }
    }
  }
  worker_task_list_iterate_end;

  return sprs;
}

/**
 * Resets data about extras
 */
void layer_workertask::reset_ruleset()
{
  m_extra_activities.clear();
  m_extra_rm_activities.clear();
}

} // namespace freeciv
