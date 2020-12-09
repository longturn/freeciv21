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



struct pf_path;
struct tile;
struct unit;
struct unit_list;

enum goto_tile_state {
  GTS_TURN_STEP,
  GTS_MP_LEFT,
  GTS_EXHAUSTED_MP,

  GTS_COUNT,
  GTS_INVALID = -1,
};

void init_client_goto(void);
void free_client_goto(void);

void enter_goto_state(struct unit_list *punits);
void exit_goto_state(void);

void goto_unit_killed(struct unit *punit);

bool goto_is_active(void);
bool goto_get_turns(int *min, int *max);
bool goto_tile_state(const struct tile *ptile, enum goto_tile_state *state,
                     int *turns, bool *waypoint);
bool goto_add_waypoint(void);
bool goto_pop_waypoint(void);

bool is_valid_goto_destination(const struct tile *ptile);
bool is_valid_goto_draw_line(struct tile *dest_tile);

void request_orders_cleared(struct unit *punit);
void send_goto_path(struct unit *punit, struct pf_path *path,
                    struct unit_order *last_order);
void send_rally_path(struct city *pcity, struct unit *punit,
                     struct pf_path *path, struct unit_order *final_order);
bool send_goto_tile(struct unit *punit, struct tile *ptile);
bool send_rally_tile(struct city *pcity, struct tile *ptile);
bool send_attack_tile(struct unit *punit, struct tile *ptile);
void send_patrol_route(void);
void send_goto_route(void);
void send_connect_route(enum unit_activity activity, struct extra_type *tgt);

struct pf_path *path_to_nearest_allied_city(struct unit *punit);
struct tile *tile_before_end_path(struct unit *punit, struct tile *ptile);




