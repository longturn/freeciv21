/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#pragma once

// utility
#include "support.h" // bool type

// common
#include "actions.h"
#include "fc_types.h"
#include "featured_text.h" // struct text_tag_list
#include "nation.h"        // Nation_type_id
#include "terrain.h"       // enum tile_special_type
#include "unitlist.h"

struct packet_nations_selected_info;

void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const text_tag_list *tags, tile *ptile);
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines);
void popup_connect_msg(const char *headline, const char *message);
void popup_races_dialog(player *pplayer);
void popdown_races_dialog();
void unit_select_dialog_popup(tile *ptile);
void unit_select_dialog_update(); // Defined in update_queue.c.
void unit_select_dialog_update_real(void *unused);
void races_toggles_set_sensitive();
void races_update_pickable(bool nationset_change);
// void popup_combat_info(int attacker_unit_id, int defender_unit_id,
//                int attacker_hp, int defender_hp,
//                bool make_att_veteran, bool make_def_veteran);
void popup_action_selection(unit *actor_unit, city *target_city,
                            unit *target_unit, tile *target_tile,
                            extra_type *target_extra,
                            const act_prob *act_probs);

int action_selection_actor_unit();
int action_selection_target_city();
int action_selection_target_unit();
int action_selection_target_tile();
int action_selection_target_extra();
void action_selection_close();
void action_selection_refresh(unit *actor_unit, city *target_city,
                              unit *target_unit, tile *target_tile,
                              extra_type *target_extra,
                              const act_prob *act_probs);
void action_selection_no_longer_in_progress_gui_specific(
    [[maybe_unused]] int actor_unit_id);
void popup_incite_dialog(unit *actor, city *pcity, int cost,
                         const action *paction);
void popup_bribe_dialog(unit *actor, unit *punit, int cost,
                        const action *paction);
void popup_sabotage_dialog(unit *actor, city *pcity, const action *paction);
void popup_pillage_dialog(unit *punit, bv_extras extras);
void popup_upgrade_dialog(unit_list *punits);
void popup_disband_dialog(unit_list *punits);
void popup_tileset_suggestion_dialog();
void popup_soundset_suggestion_dialog();
void popup_musicset_suggestion_dialog();
bool popup_theme_suggestion_dialog([[maybe_unused]] const char *theme_name);
void show_tech_gained_dialog([[maybe_unused]] Tech_type_id tech);
// bool handmade_scenario_warning();
void popdown_all_game_dialogs();
// bool request_transport(unit *pcargo, tile *ptile);
// void update_infra_dialog(); // defined in gui_interface.cpp
