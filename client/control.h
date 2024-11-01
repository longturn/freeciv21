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

// common
#include "unitlist.h"

enum cursor_hover_state {
  HOVER_NONE = 0,
  HOVER_GOTO,
  HOVER_PARADROP,
  HOVER_CONNECT,
  HOVER_PATROL,
  HOVER_ACT_SEL_TGT,
  HOVER_GOTO_SEL_TGT,
  HOVER_DEBUG_TILE,
};

// Selecting unit from a stack without popup.
enum quickselect_type {
  SELECT_POPUP = 0,
  SELECT_SEA,
  SELECT_LAND,
  SELECT_APPEND,
  SELECT_FOCUS
};

void control_init();
void control_free();
void control_unit_killed(struct unit *punit);

void unit_change_battlegroup(struct unit *punit, int battlegroup);
void unit_register_battlegroup(struct unit *punit);

extern enum cursor_hover_state hover_state;
extern enum unit_activity connect_activity;
extern struct extra_type *connect_tgt;
extern action_id goto_last_action;
extern int goto_last_tgt;
extern int goto_last_sub_tgt;
extern enum unit_orders goto_last_order;
extern bool non_ai_unit_focus;

bool can_unit_do_connect(struct unit *punit, enum unit_activity activity,
                         struct extra_type *tgt);

int check_recursive_road_connect(struct tile *ptile,
                                 const struct extra_type *pextra,
                                 const struct unit *punit,
                                 const struct player *pplayer, int rec);

void do_move_unit(struct unit *punit, struct unit *target_unit);
void do_unit_goto(struct tile *ptile);
void do_unit_paradrop_to(struct unit *punit, struct tile *ptile);
void do_unit_patrol_to(struct tile *ptile);
void do_unit_connect(struct tile *ptile, enum unit_activity activity,
                     struct extra_type *tgt);
void do_map_click(struct tile *ptile, enum quickselect_type qtype);
void control_mouse_cursor(struct tile *ptile);

void set_hover_state(const std::vector<unit *> &units,
                     enum cursor_hover_state state,
                     enum unit_activity connect_activity,
                     struct extra_type *tgt, int last_tgt,
                     int goto_last_sub_tgt, action_id goto_last_action,
                     enum unit_orders goto_last_order);
void clear_hover_state();
void request_center_focus_unit();
void request_unit_non_action_move(struct unit *punit,
                                  struct tile *dest_tile);
void request_move_unit_direction(struct unit *punit, int dir);
void request_new_unit_activity(struct unit *punit, enum unit_activity act);
void request_new_unit_activity_targeted(struct unit *punit,
                                        enum unit_activity act,
                                        struct extra_type *tgt);
void request_unit_load(struct unit *pcargo, struct unit *ptransporter,
                       struct tile *ptile);
void request_unit_unload(struct unit *pcargo);
void request_unit_ssa_set(const struct unit *punit,
                          enum server_side_agent agent);
void request_unit_autosettlers(const struct unit *punit);
void request_unit_build_city(struct unit *punit);
void request_unit_caravan_action(struct unit *punit, action_id action);
void request_unit_change_homecity(struct unit *punit);
void request_unit_connect(enum unit_activity activity,
                          struct extra_type *tgt);
void request_unit_disband(struct unit *punit);
void request_unit_fortify(struct unit *punit);
void request_unit_goto(enum unit_orders last_order, action_id act_id,
                       int sub_tgt_id);
void request_unit_move_done();
void request_unit_paradrop(const std::vector<unit *> &units);
void request_unit_patrol();
void request_unit_pillage(struct unit *punit);
void request_unit_sentry(struct unit *punit);
struct unit *request_unit_unload_all(struct unit *punit);
void request_unit_airlift(struct unit *punit, struct city *pcity);
void request_units_return();
void request_unit_upgrade(struct unit *punit);
void request_unit_convert(struct unit *punit);
void request_units_wait(const std::vector<unit *> &units);
void request_unit_wakeup(struct unit *punit);

enum unit_select_type_mode { SELTYPE_SINGLE, SELTYPE_SAME, SELTYPE_ALL };

enum unit_select_location_mode {
  SELLOC_TILE, // Tile.
  SELLOC_CONT, // Continent.
  SELLOC_WORLD // World.
};

void request_unit_select(const std::vector<unit *> &punits,
                         enum unit_select_type_mode seltype,
                         enum unit_select_location_mode selloc);

void request_do_action(action_id action, int actor_id, int target_id,
                       int sub_tgt, const char *name);
void request_action_details(action_id action, int actor_id, int target_id);
void request_toggle_city_outlines();
void request_toggle_city_output();
void request_toggle_map_grid();
void request_toggle_map_borders();
void request_toggle_map_native();
void request_toggle_city_names();
void request_toggle_city_growth();
void request_toggle_city_productions();
void request_toggle_city_buycost();
void request_toggle_city_trade_routes();

void wakeup_sentried_units(struct tile *ptile);
void clear_unit_orders(struct unit *punit);

bool unit_is_in_focus(const struct unit *punit);
struct unit *get_focus_unit_on_tile(const struct tile *ptile);
struct unit *head_of_units_in_focus();
std::vector<unit *> &get_units_in_focus();
int get_num_units_in_focus();

void unit_focus_set(struct unit *punit);
void unit_focus_set_and_select(struct unit *punit);
void unit_focus_add(struct unit *punit);
void unit_focus_urgent(struct unit *punit);

void unit_focus_advance();
void unit_focus_update();

void set_auto_center_enabled(bool enabled);
void auto_center_on_focus_unit();

unit *find_visible_unit(const ::tile *ptile);
void set_units_in_combat(struct unit *pattacker, struct unit *pdefender);
int blink_active_unit();
int blink_turn_done_button();

bool should_ask_server_for_actions(const struct unit *punit);
void action_selection_no_longer_in_progress(const int old_actor_id);
void action_decision_clear_want(const int old_actor_id);
void action_selection_next_in_focus(const int old_actor_id);
void action_decision_request(struct unit *actor_unit);

void key_cancel_action();
void key_center_capital();
void key_city_names_toggle();
void key_city_growth_toggle();
void key_city_productions_toggle();
void key_city_buycost_toggle();
void key_city_trade_routes_toggle();
void key_end_turn();
void key_city_outlines_toggle();
void key_city_output_toggle();
void key_map_grid_toggle();
void key_map_borders_toggle();
void key_map_native_toggle();
void key_recall_previous_focus_unit();
void key_unit_move(enum direction8 gui_dir);
void key_unit_airbase();
void key_unit_auto_explore();
void key_unit_auto_settle();
void key_unit_connect(enum unit_activity activity, struct extra_type *tgt);
void key_unit_action_select();
void key_unit_action_select_tgt();
void key_unit_convert();
void key_unit_done();
void key_unit_fallout();
void key_unit_fortify();
void key_unit_fortress();
void key_unit_goto();
void key_unit_homecity();
void key_unit_irrigate();
void key_unit_cultivate();
void key_unit_mine();
void key_unit_plant();
void key_unit_patrol();
void key_unit_paradrop();
void key_unit_pillage();
void key_unit_sentry();
void key_unit_transform();
void key_unit_unload_all();
void key_unit_wait();
void key_unit_wakeup_others();

void finish_city(struct tile *ptile, const char *name);
void cancel_city(struct tile *ptile);
