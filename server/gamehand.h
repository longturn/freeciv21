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
#ifndef FC__GAMEHAND_H
#define FC__GAMEHAND_H



struct section_file;
struct connection;
struct conn_list;

void init_new_game(void);
void send_year_to_clients(void);
void send_game_info(struct conn_list *dest);

void send_scenario_info(struct conn_list *dest);
void send_scenario_description(struct conn_list *dest);

enum unit_role_id crole_to_role_id(char crole);
struct unit_type *crole_to_unit_type(char crole, struct player *pplayer);

int update_timeout(void);
void increase_timeout_because_unit_moved(void);

const char *new_challenge_filename(struct connection *pc);



#endif /* FC__GAMEHAND_H */
