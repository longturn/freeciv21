/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include "fc_types.h"

#include <QString>

#include <vector>

struct player_spaceship;

/****************************************************************************
  These functions return static strings with generally useful text.
****************************************************************************/
const QString get_tile_output_text(const struct tile *ptile);
const QString popup_info_text(struct tile *ptile);
const QString get_nearest_city_text(struct city *pcity, int sq_dist);
const QString unit_description(struct unit *punit);
const QString get_airlift_text(const std::vector<unit *> &units,
                               const struct city *pdest);
const QString science_dialog_text();
const QString get_science_target_text(double *percent);
const QString get_science_goal_text(Tech_type_id goal);
const QString get_info_label_text(bool moreinfo);
const QString get_info_label_text_popup();
bool get_units_upgrade_info(char *buf, size_t bufsz,
                            const std::vector<unit *> &punits);
const QString get_spaceship_descr(struct player_spaceship *pship);
QString get_score_text(const struct player *pplayer);

const QString get_act_sel_action_custom_text(struct action *paction,
                                             const struct act_prob prob,
                                             const struct unit *actor_unit,
                                             const struct city *target_city);
const QString act_sel_action_tool_tip(const struct action *paction,
                                      const struct act_prob prob);

QString text_happiness_buildings(const struct city *pcity);
const QString text_happiness_nationality(const struct city *pcity);
const QString text_happiness_cities(const struct city *pcity);
const QString text_happiness_luxuries(const struct city *pcity);
const QString text_happiness_units(const struct city *pcity);
QString text_happiness_wonders(const struct city *pcity);
int get_bulbs_per_turn(int *pours, bool *pteam, int *ptheirs);
