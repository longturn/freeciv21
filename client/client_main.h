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
#include "fc_types.h"

#include "packets.h" /* enum report_type */
#include "worklist.h"

/*
 * Every TIMER_INTERVAL milliseconds real_timer_callback is
 * called. TIMER_INTERVAL has to stay 500 because real_timer_callback
 * also updates the timeout info.
 */
#define TIMER_INTERVAL (int) (real_timer_callback() * 1000)

/* Client states (see also enum server_states in srv_main.h).
 * Changing those values don't break the network compatibility.
 *
 * C_S_INITIAL:      Client boot, only used once on program start.
 * C_S_DISCONNECTED: The state when the client is not connected
 *                   to a server.  In this state, neither game nor ruleset
 *                   is in effect.
 * C_S_PREPARING:    Connected in pregame.  Game and ruleset are done.
 * C_S_RUNNING:      Connected ith game in progress.
 * C_S_OVER:         Connected with game over.
 */
enum client_states {
  C_S_INITIAL,
  C_S_DISCONNECTED,
  C_S_PREPARING,
  C_S_RUNNING,
  C_S_OVER,
};

int client_main(int argc, char *argv[]);

void client_packet_input(void *packet, int type);

void send_report_request(enum report_type type);
void send_attribute_block_request(void);
void send_turn_done(void);

void user_ended_turn(void);

void set_client_state(enum client_states newstate);
enum client_states client_state(void);
void set_server_busy(bool busy);
bool is_server_busy(void);

void client_remove_cli_conn(struct connection *pconn);
void client_remove_all_cli_conn(void);

extern QString logfile;
extern QString scriptfile;
extern QString savefile;
extern QString sound_plugin_name;
extern QString sound_set_name;
extern QString music_set_name;
extern QString server_host;
extern QString user_name;
extern char password[MAX_LEN_PASSWORD];
extern QString cmd_metaserver;
extern int server_port;
extern bool auto_connect;
extern bool auto_spawn;
extern bool waiting_for_end_turn;

#ifdef FREECIV_DEBUG
extern bool hackless;
#endif /* FREECIV_DEBUG */

struct global_worklist_list; /* Defined in global_worklist.[ch]. */

/* Structure for holding global client data.
 *
 * TODO: Lots more variables could be added here. */
extern struct civclient {
  /* this is the client's connection to the server */
  struct connection conn;
  struct global_worklist_list *worklists;
} client;

bool client_is_observer(void);
bool client_is_global_observer(void);
int client_player_number(void);
bool client_has_player(void);
struct player *client_player(void);
bool client_map_is_known_and_seen(const struct tile *ptile,
                                  const struct player *pplayer,
                                  enum vision_layer vlayer);
void set_miliseconds_to_turndone(int miliseconds);
int get_seconds_to_turndone(void);
bool is_waiting_turn_change(void);
void start_turn_change_wait(void);
void stop_turn_change_wait(void);
int get_seconds_to_new_turn(void);
double real_timer_callback(void);
bool can_client_control(void);
bool can_client_issue_orders(void);
bool can_client_change_view(void);
bool can_meet_with_player(const struct player *pplayer);
bool can_intel_with_player(const struct player *pplayer);

void client_exit(void);

bool is_client_quitting(void);
void start_quitting(void);

/* Set in GUI code. */
extern const char *const gui_character_encoding;
extern const bool gui_use_transliteration;


