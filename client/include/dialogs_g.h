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
                              const struct text_tag_list *tags,
                              struct tile *ptile);
void popup_notify_dialog(const char *caption, const char *headline,
                         const char *lines);
void popup_connect_msg(const char *headline, const char *message);
void popup_races_dialog(struct player *pplayer);
void popdown_races_dialog(void);
void unit_select_dialog_popup(struct tile *ptile);
void unit_select_dialog_update(); // Defined in update_queue.c.
void unit_select_dialog_update_real(void *unused);
void races_toggles_set_sensitive(void);
void races_update_pickable(bool nationset_change);
void popup_action_selection(struct unit *actor_unit,
                            struct city *target_city,
                            struct unit *target_unit,
                            struct tile *target_tile,
                            struct extra_type *target_extra,
                            const struct act_prob *act_probs);

int action_selection_actor_unit(void);
int action_selection_target_city(void);
int action_selection_target_unit(void);
int action_selection_target_tile(void);
int action_selection_target_extra(void);
void action_selection_close(void);
void action_selection_refresh(struct unit *actor_unit,
                              struct city *target_city,
                              struct unit *target_unit,
                              struct tile *target_tile,
                              struct extra_type *target_extra,
                              const struct act_prob *act_probs);
void action_selection_no_longer_in_progress_gui_specific(int actor_unit_id);
void popup_incite_dialog(struct unit *actor, struct city *pcity, int cost,
                         const struct action *paction);
void popup_bribe_dialog(struct unit *actor, struct unit *punit, int cost,
                        const struct action *paction);
void popup_sabotage_dialog(struct unit *actor, struct city *pcity,
                           const struct action *paction);
void popup_pillage_dialog(struct unit *punit, bv_extras extras);
void popup_upgrade_dialog(struct unit_list *punits);
void popup_disband_dialog(struct unit_list *punits);
void popup_tileset_suggestion_dialog(void);
void popup_soundset_suggestion_dialog(void);
void popup_musicset_suggestion_dialog(void);
bool popup_theme_suggestion_dialog(const char *theme_name);
void show_tech_gained_dialog(Tech_type_id tech);
void popdown_all_game_dialogs(void);
