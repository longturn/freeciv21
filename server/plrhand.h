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

struct connection;
struct conn_list;
struct nation_type;
struct player;
struct rgbcolor;
struct section_file;
struct unit_list;

struct player *server_create_player(int player_id, const char *ai_tname,
                                    struct rgbcolor *prgbcolor,
                                    bool allow_ai_type_fallbacking);
const struct rgbcolor *player_preferred_color(struct player *pplayer);
bool player_color_changeable(const struct player *pplayer,
                             const char **reason);
void assign_player_colors();
void server_player_set_color(struct player *pplayer,
                             const struct rgbcolor *prgbcolor);
const char *player_color_ftstr(struct player *pplayer);
void server_player_init(struct player *pplayer, bool initmap,
                        bool needs_team);
void give_midgame_initial_units(struct player *pplayer, struct tile *ptile);
void server_remove_player(struct player *pplayer);
void kill_player(struct player *pplayer);
void update_revolution(struct player *pplayer);
void government_change(struct player *pplayer, struct government *gov,
                       bool revolution_finished);
int revolution_length(struct government *gov, struct player *plr);
void update_capital(struct player *pplayer);

struct player_economic player_limit_to_max_rates(struct player *pplayer);

void server_player_set_name(struct player *pplayer, const char *name);
bool server_player_set_name_full(const struct connection *caller,
                                 struct player *pplayer,
                                 const struct nation_type *pnation,
                                 const char *name, char *error_buf,
                                 size_t error_buf_len);

struct nation_type *pick_a_nation(const struct nation_list *choices,
                                  bool ignore_conflicts, bool needs_startpos,
                                  enum barbarian_type barb_type);
bool nation_is_in_current_set(const struct nation_type *pnation);
bool client_can_pick_nation(const struct nation_type *nation);
void count_playable_nations();
void send_nation_availability(struct conn_list *dest, bool nationset_change);
void fit_nationset_to_players();

void check_player_max_rates(struct player *pplayer);
void make_contact(struct player *pplayer1, struct player *pplayer2,
                  struct tile *ptile);
void maybe_make_contact(struct tile *ptile, struct player *pplayer);
void enter_war(struct player *pplayer, struct player *pplayer2);
void player_update_last_war_action(struct player *pplayer);

void player_info_freeze();
void player_info_thaw();

void send_player_all_c(struct player *src, struct conn_list *dest);
void send_player_info_c(struct player *src, struct conn_list *dest);
void send_player_diplstate_c(struct player *src, struct conn_list *dest);

struct conn_list *player_reply_dest(struct player *pplayer);

void shuffle_players();
void set_shuffled_players(int *shuffled_players);
struct player *shuffled_player(int i);
void reset_all_start_commands(bool plrchange);

#define shuffled_players_iterate(NAME_pplayer)                              \
  do {                                                                      \
    int MY_i;                                                               \
    struct player *NAME_pplayer;                                            \
    log_debug("shuffled_players_iterate @ %s line %d", __FILE__,            \
              __FC_LINE__);                                                 \
    for (MY_i = 0; MY_i < MAX_NUM_PLAYER_SLOTS; MY_i++) {                   \
      NAME_pplayer = shuffled_player(MY_i);                                 \
      if (NAME_pplayer != nullptr) {

#define shuffled_players_iterate_end                                        \
  }                                                                         \
  }                                                                         \
  }                                                                         \
  while (false)

#define phase_players_iterate(pplayer)                                      \
  do {                                                                      \
    shuffled_players_iterate(pplayer)                                       \
    {                                                                       \
      if (is_player_phase(pplayer, game.info.phase)) {

#define phase_players_iterate_end                                           \
  }                                                                         \
  }                                                                         \
  shuffled_players_iterate_end;                                             \
  }                                                                         \
  while (false)                                                             \
    ;

#define alive_phase_players_iterate(pplayer)                                \
  do {                                                                      \
    phase_players_iterate(pplayer)                                          \
    {                                                                       \
      if (pplayer->is_alive) {

#define alive_phase_players_iterate_end                                     \
  }                                                                         \
  }                                                                         \
  phase_players_iterate_end                                                 \
  }                                                                         \
  while (false)                                                             \
    ;

bool civil_war_possible(struct player *pplayer, bool conquering_city,
                        bool honour_server_option);
bool civil_war_triggered(struct player *pplayer);
struct player *civil_war(struct player *pplayer);

void update_players_after_alliance_breakup(
    struct player *pplayer, struct player *pplayer2,
    const struct unit_list *pplayer_seen_units,
    const struct unit_list *pplayer2_seen_units);

int normal_player_count();

void player_status_add(struct player *plr, enum player_status status);
bool player_status_check(struct player *plr, enum player_status status);
void player_status_reset(struct player *plr);

const char *player_delegation_get(const struct player *pplayer);
void player_delegation_set(struct player *pplayer, const char *username);
bool player_delegation_active(const struct player *pplayer);

void send_delegation_info(const struct connection *pconn);

struct player *player_by_user_delegated(const char *name);

void handle_diplomacy_cancel_pact_explicit(struct player *pplayer,
                                           int other_player_id,
                                           enum clause_type clause,
                                           bool protect_alliances);

// player colors
void playercolor_init();
void playercolor_free();

int playercolor_count();
void playercolor_add(struct rgbcolor *prgbcolor);
struct rgbcolor *playercolor_get(int id);

void player_set_to_ai_mode(struct player *pplayer,
                           enum ai_level skill_level);
void player_set_under_human_control(struct player *pplayer);
