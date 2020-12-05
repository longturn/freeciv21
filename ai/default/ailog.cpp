/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>

/* common */
#include "map.h"
#include "player.h"
#include "research.h"

/* server */
#include "notify.h"

/* ai/default */
#include "aidata.h"
#include "aiplayer.h"
#include "aiunit.h"
#include "daicity.h"

#include "ailog.h"

Q_LOGGING_CATEGORY(ai_category, "freeciv.ai")

/**********************************************************************/ /**
   Produce logline fragment for srv_log.
 **************************************************************************/
void dai_city_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct city *pcity)
{
  struct ai_city *city_data = def_ai_city_data(pcity, ait);

  fc_snprintf(buffer, buflength, "d%d u%d g%d", city_data->danger,
              city_data->urgency, city_data->grave_danger);
}

/**********************************************************************/ /**
   Produce logline fragment for srv_log.
 **************************************************************************/
void dai_unit_log(struct ai_type *ait, char *buffer, int buflength,
                  const struct unit *punit)
{
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  fc_snprintf(buffer, buflength, "%d %d", unit_data->bodyguard,
              unit_data->ferryboat);
}

/**********************************************************************/ /**
   Log player tech messages.
 **************************************************************************/
QString tech_log_prefix(ai_type *ait, const player *pplayer,
                        advance *padvance)
{
  // FIXME const-correctness of the arguments
  if (!valid_advance(padvance) || advance_by_number(A_NONE) == padvance) {
    return QStringLiteral("(invalid tech)");
  }

  auto plr_data = def_ai_player_data(pplayer, ait);
  return QString::asprintf(
      "%s::%s (want " ADV_WANT_PRINTF ", dist %d) ", player_name(pplayer),
      advance_rule_name(padvance),
      plr_data->tech_want[advance_index(padvance)],
      research_goal_unknown_techs(research_get(pplayer),
                                  advance_number(padvance)));
}

/**********************************************************************/ /**
   Log player messages, they will appear like this

   where ti is timer, co countdown and lo love for target, who is e.
 **************************************************************************/
QString diplo_log_prefix(ai_type *ait, const player *pplayer,
                         const player *aplayer)
{
  // FIXME const-correctness of the arguments
  /* Don't use ai_data_get since it can have side effects. */
  auto adip = dai_diplomacy_get(ait, pplayer, aplayer);

  return QString::asprintf(
      "%s->%s(l%d,c%d,d%d%s): ", player_name(pplayer), player_name(aplayer),
      pplayer->ai_common.love[player_index(aplayer)], adip->countdown,
      adip->distance,
      adip->is_allied_with_enemy ? "?"
                                 : (adip->at_war_with_ally ? "!" : ""));
}

/**********************************************************************/ /**
   Log message for bodyguards. They will appear like this
     Polish Mech. Inf.[485] bodyguard (38,22){Riflemen:574@37,23}
   note that these messages are likely to wrap if long.
 **************************************************************************/
QString bodyguard_log_prefix(ai_type *ait, const unit *punit)
{
  const struct unit *pcharge;
  const struct city *pcity;
  int id = -1;
  int charge_x = -1;
  int charge_y = -1;
  const char *type = "guard";
  const char *s = "none";
  struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

  pcity = game_city_by_number(unit_data->charge);
  pcharge = game_unit_by_number(unit_data->charge);
  if (pcharge) {
    index_to_map_pos(&charge_x, &charge_y, tile_index(unit_tile(pcharge)));
    id = pcharge->id;
    type = "bodyguard";
    s = unit_rule_name(pcharge);
  } else if (pcity) {
    index_to_map_pos(&charge_x, &charge_y, tile_index(city_tile(pcity)));
    id = pcity->id;
    type = "cityguard";
    s = city_name_get(pcity);
  }
  /* else perhaps the charge died */

  return QString::asprintf(
      "%s %s[%d] %s (%d,%d){%s:%d@%d,%d} ",
      nation_rule_name(nation_of_unit(punit)), unit_rule_name(punit),
      punit->id, type, TILE_XY(unit_tile(punit)), s, id, charge_x, charge_y);
}
