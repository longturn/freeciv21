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
#include "log.h"       // QtMsgType
#include "net_types.h" // announce_type

// common
#include "fc_types.h"
#include "game.h"

// Qt
#include <QHostAddress>

struct conn_list;

struct server_arguments {
  // metaserver information
  bool metaserver_no_send;
  QString metaserver_addr;
  bool metaconnection_persistent;
  QString identity_name;
  unsigned short int metaserver_port;
  // Address in case a local socket is used
  QString local_addr;
  // address this server is to listen on
  QHostAddress bind_addr;
  // this server's listen port
  int port;
  bool user_specified_port;
  // address to bind when connecting to the metaserver (nullptr => bind_addr)
  QString bind_meta_addr;
  // filenames
  QString log_filename;
  QString ranklog_filename;
  QString load_filename;
  QString script_filename;
  QString saves_pathname;
  QString scenarios_pathname;
  QString ruleset;
  QString serverid;
  // quit if there no players after a given time interval
  int quitidle;
  // exit the server on game ending
  bool exit_on_end;
  bool timetrack; // defaults to FALSE
  // authentication options
  bool fcdb_enabled;        // defaults to FALSE
  QString fcdb_conf;        // freeciv database configuration file
  bool auth_enabled;        // defaults to FALSE
  bool auth_allow_guests;   // defaults to FALSE
  bool auth_allow_newusers; // defaults to FALSE
  enum announce_type announce;
};

// used in savegame values
#define SPECENUM_NAME server_states
#define SPECENUM_VALUE0 S_S_INITIAL
#define SPECENUM_VALUE1 S_S_RUNNING
#define SPECENUM_VALUE2 S_S_OVER
#include "specenum_gen.h"

/* Structure for holding global server data.
 *
 * TODO: Lots more variables could be added here. */
extern struct civserver {
  int playable_nations;
  int nbarbarians;

  /* this counter creates all the city and unit numbers used.
   * arbitrarily starts at 101, but at 65K wraps to 1.
   * use identity_number()
   */
#define IDENTITY_NUMBER_SKIP (100)
  unsigned short identity_number;

  char game_identifier[MAX_LEN_GAME_IDENTIFIER];

  struct unit_wait_list *unit_waits;
} server;

void init_game_seed();
void srv_init();
void server_quit();
void save_game_auto(const char *save_reason, enum autosave_type type);

enum server_states server_state();
void set_server_state(enum server_states newstate);

void check_for_full_turn_done();
bool check_for_game_over();
bool game_was_started();

int server_plr_tile_city_id_get(const struct tile *ptile,
                                const struct player *pplayer);

server_setting_id server_ss_by_name(const char *name);
const char *server_ss_name_get(server_setting_id id);
enum sset_type server_ss_type_get(server_setting_id id);
bool server_ss_val_bool_get(server_setting_id id);
int server_ss_val_int_get(server_setting_id id);
unsigned int server_ss_val_bitwise_get(server_setting_id id);

bool server_packet_input(struct connection *pconn, void *packet, int type);
void start_game();
const char *pick_random_player_name(const struct nation_type *pnation);
void player_nation_defaults(struct player *pplayer,
                            struct nation_type *pnation, bool set_name);
void send_all_info(struct conn_list *dest);

void begin_turn(bool is_new_turn);
void begin_phase(bool is_new_phase);
void end_phase();
void end_turn();

void identity_number_release(int id);
void identity_number_reserve(int id);
int identity_number();

void srv_ready();
void srv_scores();

void server_game_init(bool keep_ruleset_value);
void server_game_free();
const char *aifill(int amount);

extern struct server_arguments srvarg;

extern bool force_end_of_sniff;

void update_nations_with_startpos();

known_type mapimg_server_tile_known(const struct tile *ptile,
                                    const struct player *pplayer,
                                    bool knowledge);
terrain *mapimg_server_tile_terrain(const struct tile *ptile,
                                    const struct player *pplayer,
                                    bool knowledge);
player *mapimg_server_tile_owner(const struct tile *ptile,
                                 const struct player *pplayer,
                                 bool knowledge);
player *mapimg_server_tile_city(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);
player *mapimg_server_tile_unit(const struct tile *ptile,
                                const struct player *pplayer,
                                bool knowledge);

int mapimg_server_plrcolor_count();
rgbcolor *mapimg_server_plrcolor_get(int i);
