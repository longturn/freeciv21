// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_abstract_activities.h"

#include "tilespec.h"

// common
#include "city.h"
#include "fc_types.h"
#include "tile.h"

namespace freeciv {

/**
 * \class layer_abstract_activities
 * \brief An abstract class for layers that need sprites for unit activities.
 */

/**
 * Loads the sprites in memory.
 */
void layer_abstract_activities::load_sprites()
{
  m_activities[ACTIVITY_MINE] = load_sprite({"unit.plant"}, true);
  m_activities[ACTIVITY_PLANT] = m_activities[ACTIVITY_MINE];
  m_activities[ACTIVITY_IRRIGATE] = load_sprite({"unit.irrigate"}, true);
  m_activities[ACTIVITY_CULTIVATE] = m_activities[ACTIVITY_IRRIGATE];
  m_activities[ACTIVITY_PILLAGE] = load_sprite({"unit.pillage"}, true);
  m_activities[ACTIVITY_FORTIFIED] = load_sprite({"unit.fortified"}, true);
  m_activities[ACTIVITY_FORTIFYING] = load_sprite({"unit.fortifying"}, true);
  m_activities[ACTIVITY_SENTRY] = load_sprite({"unit.sentry"}, true);
  m_activities[ACTIVITY_GOTO] = load_sprite({"unit.goto"}, true);
  m_activities[ACTIVITY_TRANSFORM] = load_sprite({"unit.transform"}, true);
  m_activities[ACTIVITY_CONVERT] = load_sprite({"unit.convert"}, true);
}

/**
 * Loads the extra activity and remove activity sprites.
 */
void layer_abstract_activities::initialize_extra(const extra_type *extra,
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
    m_extra_activities.push_back(load_sprite(tags, true));
  }

  if (!fc_strcasecmp(extra->rmact_gfx, "none")) {
    m_extra_rm_activities.push_back(nullptr);
  } else {
    QStringList tags = {
        extra->rmact_gfx,
        extra->rmact_gfx_alt,
    };
    m_extra_rm_activities.push_back(load_sprite(tags, true));
  }
}

/**
 * Resets data about extras
 */
void layer_abstract_activities::reset_ruleset()
{
  m_extra_activities.clear();
  m_extra_rm_activities.clear();
}

/**
 * Returns the sprite used to represent a given activity on the map.
 */
QPixmap *
layer_abstract_activities::activity_sprite(unit_activity activity,
                                           const extra_type *target) const
{
  // Nicely inconsistent
  switch (activity) {
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT:
    return m_extra_rm_activities[extra_index(target)];
    break;
  case ACTIVITY_BASE:
  case ACTIVITY_GEN_ROAD:
    return m_extra_activities[extra_index(target)];
    break;
  default:
    break;
  }

  return target ? m_extra_activities[extra_index(target)]
                : m_activities[activity];
}

} // namespace freeciv
