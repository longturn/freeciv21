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

/* utility */
#include "rand.h"

/* server */
#include "srv_main.h"

struct section_file;
struct extra_type;
struct base_type;
struct road_type;

enum sgf_version { SAVEGAME_2, SAVEGAME_3 };

enum tile_special_type {
  S_IRRIGATION,
  S_MINE,
  S_POLLUTION,
  S_HUT,
  S_FARMLAND,
  S_FALLOUT,

  /* internal values not saved */
  S_LAST,

  S_OLD_FORTRESS,
  S_OLD_AIRBASE,
  S_OLD_ROAD,
  S_OLD_RAILROAD,
  S_OLD_RIVER
};

struct loaddata {
  struct section_file *file;
  const char *secfile_options;
  int version;

  /* loaded in sg_load_savefile(); needed in sg_load_player() */
  struct {
    const char **order;
    size_t size;
  } improvement;
  /* loaded in sg_load_savefile(); needed in sg_load_player() */
  struct {
    const char **order;
    size_t size;
  } technology;
  /* loaded in sg_load_savefile(); needed in sg_load_player() */
  struct {
    const char **order;
    size_t size;
  } activities;
  /* loaded in sg_load_savefile(); needed in sg_load_player() */
  struct {
    const char **order;
    size_t size;
  } trait;
  /* loaded in sg_load_savefile(); needed in sg_load_map(), ... */
  struct {
    struct extra_type **order;
    size_t size;
  } extra;
  /* loaded in sg_load_savefile(); needed in sg_load_players_basic() */
  struct {
    struct multiplier **order;
    size_t size;
  } multiplier;
  /* loaded in sg_load_savefile(); needed in sg_load_map(), ...
   * Deprecated in 3.0 (savegame3.c) */
  struct {
    enum tile_special_type *order;
    size_t size;
  } special;
  /* loaded in sg_load_savefile(); needed in sg_load_map(), ...
   * Deprecated in 3.0 (savegame3.c) */
  struct {
    struct base_type **order;
    size_t size;
  } base;
  /* loaded in sg_load_savefile(); needed in sg_load_map(), ...
   * Deprecated in 3.0 (savegame3.c) */
  struct {
    struct road_type **order;
    size_t size;
  } road;
  /* loaded in sg_load_savefile(); needed in sg_load_(), ... */
  struct {
    struct specialist **order;
    size_t size;
  } specialist;
  /* loaded in sg_load_savefile(); needed in sg_load_player_main(), ... */
  struct {
    enum diplstate_type *order;
    size_t size;
  } ds_t;
  /* loaded in sg_load_savefile(); needed in sg_load_player_unit(), ... */
  struct {
    action_id *order;
    size_t size;
  } action;
  /* loaded in sg_load_savefile(); needed in sg_load_player_unit(), ... */
  struct {
    enum action_decision *order;
    size_t size;
  } act_dec;
  /* loaded in sg_load_savefile(); needed in sg_load_player_unit(), ... */
  struct {
    enum server_side_agent *order;
    size_t size;
  } ssa;

  /* loaded in sg_load_game(); needed in sg_load_random(), ... */
  enum server_states server_state;

  /* loaded in sg_load_random(); needed in sg_load_sanitycheck() */
  RANDOM_STATE rstate;

  /* loaded in sg_load_map_worked(); needed in sg_load_player_cities() */
  int *worked_tiles;
};

#define log_sg qCritical

#define sg_check_ret(...)                                                   \
  if (!sg_success) {                                                        \
    return;                                                                 \
  }
#define sg_check_ret_val(_val)                                              \
  if (!sg_success) {                                                        \
    return _val;                                                            \
  }

#define sg_warn(condition, message, ...)                                    \
  if (!(condition)) {                                                       \
    log_sg(message, ##__VA_ARGS__);                                         \
  }
#define sg_warn_ret(condition, message, ...)                                \
  if (!(condition)) {                                                       \
    log_sg(message, ##__VA_ARGS__);                                         \
    return;                                                                 \
  }
#define sg_warn_ret_val(condition, _val, message, ...)                      \
  if (!(condition)) {                                                       \
    log_sg(message, ##__VA_ARGS__);                                         \
    return _val;                                                            \
  }

#define sg_failure_ret(condition, message, ...)                             \
  if (!(condition)) {                                                       \
    sg_success = false;                                                     \
    log_sg(message, ##__VA_ARGS__);                                         \
    sg_check_ret();                                                         \
  }
#define sg_failure_ret_val(condition, _val, message, ...)                   \
  if (!(condition)) {                                                       \
    sg_success = false;                                                     \
    log_sg(message, ##__VA_ARGS__);                                         \
    sg_check_ret_val(_val);                                                 \
  }

void sg_load_compat(struct loaddata *loading, enum sgf_version format_class);
void sg_load_post_load_compat(struct loaddata *loading,
                              enum sgf_version format_class);
int current_compat_ver();

#define hex_chars "0123456789abcdef"

char bin2ascii_hex(int value, int halfbyte_wanted);
int ascii_hex2bin(char ch, int halfbyte);

int char2num(char ch);

enum tile_special_type special_by_rule_name(const char *name);
const char *special_rule_name(enum tile_special_type type);
struct extra_type *special_extra_get(int spe);

struct extra_type *resource_by_identifier(const char identifier);

enum ai_level ai_level_convert(int old_level);
enum barbarian_type barb_type_convert(int old_type);

void set_unit_activity_base(struct unit *punit, Base_type_id base);
void set_unit_activity_road(struct unit *punit, Road_type_id road);

#define ORDER_OLD_BUILD_CITY (-1)
#define ORDER_OLD_DISBAND (-2)
#define ORDER_OLD_BUILD_WONDER (-3)
#define ORDER_OLD_TRADE_ROUTE (-4)
#define ORDER_OLD_HOMECITY (-5)
int sg_order_to_action(int order, struct unit *act_unit,
                       struct tile *tgt_tile);
