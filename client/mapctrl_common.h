/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// common
#include "map.h" // enum direction8

// client
#include "control.h" // quickselect_type

extern bool rbutton_down;
extern bool rectangle_active;
extern bool tiles_hilited_cities;

extern bool keyboardless_goto_button_down;
extern bool keyboardless_goto_active;
extern struct tile *keyboardless_goto_start_tile;

void cancel_selection_rectangle();

void key_city_overlay(int canvas_x, int canvas_y);
void key_city_show_open(struct city *pcity);
void key_city_hide_open(struct city *pcity);

bool clipboard_copy_production(struct tile *ptile);
void clipboard_paste_production(struct city *pcity);
void upgrade_canvas_clipboard();

void release_goto_button(int canvas_x, int canvas_y);
void maybe_activate_keyboardless_goto(int canvas_x, int canvas_y);

bool get_turn_done_button_state();
bool can_end_turn();
void action_button_pressed(int canvas_x, int canvas_y,
                           enum quickselect_type qtype);
void wakeup_button_pressed(int canvas_x, int canvas_y);
void adjust_workers_button_pressed(int canvas_x, int canvas_y);
void recenter_button_pressed(int canvas_x, int canvas_y);
void update_turn_done_button_state();
void update_line(int canvas_x, int canvas_y);

extern struct city *city_workers_display;
