// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// generated
#include <fc_config.h>

// utility
#include "log.h"

// common
#include "fc_types.h"
#include "improvement.h"      // improvement_iterate()
#include "name_translation.h" // struct name_translation
#include "player.h"           // players_iterate()
#include "requirements.h"     // struct requirement_vector
#include "unit.h"             // struct unit_order
#include "vision.h"           // struct vision

// Qt
#include <QtLogging> // QtMsgType

// std
#include <array>  // std:array
#include <ctime>  // time_t
#include <vector> // std:vector

enum production_class_type {
  PCT_UNIT,
  PCT_NORMAL_IMPROVEMENT,
  PCT_WONDER,
  PCT_LAST
};

/* Changing the max radius requires updating network capabilities and results
 * in incompatible savefiles. */
#define CITY_MAP_MIN_RADIUS 0
#define CITY_MAP_DEFAULT_RADIUS 2
#define CITY_MAP_MAX_RADIUS 5

// The city includes all tiles dx^2 + dy^2 <= CITY_MAP_*_RADIUS_SQ
#define CITY_MAP_DEFAULT_RADIUS_SQ                                          \
  (CITY_MAP_DEFAULT_RADIUS * CITY_MAP_DEFAULT_RADIUS + 1)
#define CITY_MAP_MIN_RADIUS_SQ                                              \
  (CITY_MAP_MIN_RADIUS * CITY_MAP_MIN_RADIUS + 1)
#define CITY_MAP_MAX_RADIUS_SQ                                              \
  (CITY_MAP_MAX_RADIUS * CITY_MAP_MAX_RADIUS + 1)
// the id for the city center
#define CITY_MAP_CENTER_RADIUS_SQ -1
// the tile index of the city center
#define CITY_MAP_CENTER_TILE_INDEX 0

// Maximum diameter of the workable city area.
#define CITY_MAP_MAX_SIZE (CITY_MAP_MAX_RADIUS * 2 + 1)

#define INCITE_IMPOSSIBLE_COST (1000 * 1000 * 1000)

/*
 * Size of the biggest possible city.
 *
 * The constant may be changed since it isn't externally visible.
 *
 * The city size is saved as unsigned char. Therefore, MAX_CITY_SIZE should
 * be below 255!
 */
#define MAX_CITY_SIZE 0xFF

// Iterate a city map, from the center (the city) outwards
struct iter_index {
  int dx, dy, dist;
};

/* City map coordinates are positive integers shifted by the maximum
 * radius the game engine allows (not the current ruleset) */
#define CITY_REL2ABS(_coor) (_coor + CITY_MAP_MAX_RADIUS)
#define CITY_ABS2REL(_coor) (_coor - CITY_MAP_MAX_RADIUS)

bool city_tile_index_to_xy(int *city_map_x, int *city_map_y,
                           int city_tile_index, int city_radius_sq);
int city_tile_xy_to_index(int city_map_x, int city_map_y,
                          int city_radius_sq);

int rs_max_city_radius_sq();
int city_map_radius_sq_get(const struct city *pcity);
void city_map_radius_sq_set(struct city *pcity, int radius_sq);
int city_map_tiles(int city_radius_sq);
#define city_map_tiles_from_city(_pcity)                                    \
  city_map_tiles(city_map_radius_sq_get(_pcity))

void citylog_map_data(QtMsgType level, int radius_sq, int *map_data);
void citylog_map_workers(QtMsgType level, struct city *pcity);

/* Iterate over the tiles of a city map. Starting at a given city radius
 * (the city center is _radius_sq_min = 0) outward to the tiles of
 * _radius_sq_max. (_x, _y) will be the valid elements of
 * [0, CITY_MAP_MAX_SIZE] taking into account the city radius. */
#define city_map_iterate_outwards_radius_sq_index(                          \
    _radius_sq_min, _radius_sq_max, _index, _x, _y)                         \
  {                                                                         \
    fc_assert(_radius_sq_min <= _radius_sq_max);                            \
    int _x = 0, _y = 0, _index;                                             \
    int _x##_y##_index = city_map_tiles(_radius_sq_min);                    \
    while (                                                                 \
        city_tile_index_to_xy(&_x, &_y, _x##_y##_index, _radius_sq_max)) {  \
      _index = _x##_y##_index;                                              \
      _x##_y##_index++;

#define city_map_iterate_outwards_radius_sq_index_end                       \
  }                                                                         \
  }

// Same as above, but don't set index
#define city_map_iterate_outwards_radius_sq(_radius_sq_min, _radius_sq_max, \
                                            _x, _y)                         \
  {                                                                         \
    fc_assert(_radius_sq_min <= _radius_sq_max);                            \
    int _x = 0, _y = 0;                                                     \
    int _x##_y##_index = city_map_tiles(_radius_sq_min);                    \
    while (                                                                 \
        city_tile_index_to_xy(&_x, &_y, _x##_y##_index, _radius_sq_max)) {  \
      _x##_y##_index++;

#define city_map_iterate_outwards_radius_sq_end                             \
  }                                                                         \
  }

/* Iterate a city map. This iterates over all city positions in the city
 * map starting at the city center (i.e., positions that are workable by
 * the city) using the index (_index) and  the coordinates (_x, _y). It
 * is an abbreviation for city_map_iterate_outwards_radius_sq(_end). */
#define city_map_iterate(_radius_sq, _index, _x, _y)                        \
  city_map_iterate_outwards_radius_sq_index(CITY_MAP_CENTER_RADIUS_SQ,      \
                                            _radius_sq, _index, _x, _y)

#define city_map_iterate_end city_map_iterate_outwards_radius_sq_index_end

#define city_map_iterate_without_index(_radius_sq, _x, _y)                  \
  city_map_iterate_outwards_radius_sq(CITY_MAP_CENTER_RADIUS_SQ,            \
                                      _radius_sq, _x, _y)

#define city_map_iterate_without_index_end                                  \
  city_map_iterate_outwards_radius_sq_end

// Iterate the tiles between two radii of a city map.
#define city_map_iterate_radius_sq(_radius_sq_min, _radius_sq_max, _x, _y)  \
  city_map_iterate_outwards_radius_sq(_radius_sq_min, _radius_sq_max, _x, _y)

#define city_map_iterate_radius_sq_end                                      \
  city_map_iterate_outwards_radius_sq_end

/* Iterate a city map in checked real map coordinates.
 * _radius_sq is the squared city radius.
 * _city_tile is the center of the (possible) city.
 * (_index) will be the city tile index in the intervall
 * [0, city_map_tiles(_radius_sq)] */
#define city_tile_iterate_index(_radius_sq, _city_tile, _tile, _index)      \
  {                                                                         \
    city_map_iterate_outwards_radius_sq_index(CITY_MAP_CENTER_RADIUS_SQ,    \
                                              _radius_sq, _index, _x,       \
                                              _y) struct tile *_tile =      \
        city_map_to_tile(_city_tile, _radius_sq, _x, _y);                   \
    if (nullptr != _tile) {

#define city_tile_iterate_index_end                                         \
  }                                                                         \
  }                                                                         \
  city_map_iterate_outwards_radius_sq_index_end;

// simple extension to skip the city center.
#define city_tile_iterate_skip_center(_radius_sq, _city_tile, _tile,        \
                                      _index, _x, _y)                       \
  {                                                                         \
    city_map_iterate(_radius_sq, _index, _x, _y)                            \
    {                                                                       \
      if (!is_city_center_index(_index)) {                                  \
        struct tile *_tile =                                                \
            city_map_to_tile(_city_tile, _radius_sq, _x, _y);               \
        if (nullptr != _tile) {

#define city_tile_iterate_skip_center_end                                   \
  }                                                                         \
  }                                                                         \
  }                                                                         \
  city_map_iterate_end;                                                     \
  }

/* Does the same thing as city_tile_iterate_index, but keeps the city
 * coordinates hidden. */
#define city_tile_iterate(_radius_sq, _city_tile, _tile)                    \
  {                                                                         \
    city_map_iterate_outwards_radius_sq(                                    \
        CITY_MAP_CENTER_RADIUS_SQ, _radius_sq, _x, _y) struct tile *_tile = \
        city_map_to_tile(_city_tile, _radius_sq, _x, _y);                   \
    if (nullptr != _tile) {

#define city_tile_iterate_end                                               \
  }                                                                         \
  }                                                                         \
  city_map_iterate_outwards_radius_sq_end;

/* Improvement status (for cities' lists of improvements)
 * (replaced Impr_Status) */

struct built_status {
  int turn;              // turn built, negative for old state
#define I_NEVER (-1)     // Improvement never built
#define I_DESTROYED (-2) // Improvement built and destroyed
};

/* How much this output type is penalized for unhappy cities: not at all,
 * surplus knocked down to 0, or all production removed. */
enum output_unhappy_penalty {
  UNHAPPY_PENALTY_NONE,
  UNHAPPY_PENALTY_SURPLUS,
  UNHAPPY_PENALTY_ALL_PRODUCTION
};

struct output_type {
  int index;
  const char *name; // Untranslated name
  const char *id;   // Identifier string (for rulesets, etc.)
  bool harvested;   // Is this output type gathered by city workers?
  enum output_unhappy_penalty unhappy_penalty;
};

enum citizen_category {
  CITIZEN_HAPPY,
  CITIZEN_CONTENT,
  CITIZEN_UNHAPPY,
  CITIZEN_ANGRY,
  CITIZEN_LAST,
  CITIZEN_SPECIALIST = CITIZEN_LAST,
};

// Ways city output can be lost. Not currently part of network protocol.
enum output_loss {
  OLOSS_WASTE, // regular corruption or waste
  OLOSS_SIZE,  /* notradesize/fulltradesize */
  OLOSS_LAST
};

/* This enumerators are used at client side only (so changing it doesn't
 * break the compability) to mark that the city need specific gui updates
 * (e.g. city dialog, or city report). */
enum city_updates {
  CU_NO_UPDATE = 0,
  CU_UPDATE_REPORT = 1 << 0,
  CU_UPDATE_DIALOG = 1 << 1,
  CU_POPUP_DIALOG = 1 << 2
};

/// Used to cache the value of waste effects to speed up governors
struct cached_waste {
  int level = 0;           // EFT_OUTPUT_WASTE
  int relative = 0;        // EFT_OUTPUT_WASTE_PCT
  int by_distance = 0;     // EFT_OUTPUT_WASTE_BY_DISTANCE
  int by_rel_distance = 0; // EFT_OUTPUT_WASTE_BY_REL_DISTANCE
};

struct tile_cache; // defined and only used within city.cpp

struct adv_city; // defined in server/advisors/infracache.h

struct cm_parameter; // defined in common/aicore/cm.h

struct city {
  char name[MAX_LEN_CITYNAME];
  struct tile *tile;       // May be nullptr, should check!
  struct player *owner;    // Cannot be nullptr.
  struct player *original; // Cannot be nullptr.
  int id;
  int style;
  enum capital_type capital;

  // the people
  citizens size;
  citizens feel[CITIZEN_LAST][FEELING_LAST];

  // Specialists
  citizens specialists[SP_MAX];

  citizens martial_law;       // Citizens pacified by martial law.
  citizens unit_happy_upkeep; // Citizens angered by military action.

  citizens *nationality; // Nationality of the citizens.

  // trade routes
  struct trade_route_list *routes;

  /* Tile output, regardless of if the tile is actually worked. It is used
   * as cache for the output of the tiles within the city map.
   * (see city_tile_cache_update() and city_tile_cache_get_output()) */
  struct tile_cache *tile_cache;
  /* The memory allocated for tile_cache is valid for this squared city
   * radius. */
  int tile_cache_radius_sq;

  // the productions
  int surplus[O_LAST];         // Final surplus in each category.
  int waste[O_LAST];           /* Waste/corruption in each category. */
  int unhappy_penalty[O_LAST]; // Penalty from unhappy cities.
  int prod[O_LAST];         // Production is total minus waste and penalty.
  int citizen_base[O_LAST]; // Base production from citizens.
  int usage[O_LAST];        // Amount of each resource being used.

  /* Surplus saved for use during city processing loop */
  int saved_surplus[O_LAST];

  // Cached values for CPU savings.
  int bonus[O_LAST];

  // the physics
  int food_stock;
  int shield_stock;
  int pollution;      // not saved
  int illness_trade;  /* not saved; illness due to trade; it is
                         calculated within the server and send to
                         the clients as the clients do not have all
                         information about the trade cities */
  int turn_plague;    // last turn with an illness in the city
  int city_radius_sq; // current squared city radius

  // turn states
  int airlift;
  int bought_shields;
  bool did_buy;
  bool did_sell;
  bool was_happy;

  int anarchy; // anarchy rounds count
  int rapture; // rapture rounds count
  int turn_founded;
  int turn_last_built;

  int before_change_shields; /* If changed this turn, shields before penalty
                              */
  int caravan_shields;       // If caravan has helped city to build wonder.
  int disbanded_shields;     // If you disband unit in a city. Count them
  int last_turns_shield_surplus; // The surplus we had last turn.

  struct built_status built[B_LAST];

  struct universal production;

  // If changed this turn, what we changed from
  struct universal changed_from;

  struct worklist worklist;

  bv_city_options city_options;

  struct unit_list *units_supported;

  int history; // Cumulative culture

  struct worker_task_list *task_reqs;

  int steal; // diplomats steal once; for spies, gets harder

  struct {
    int length;
    // If true, rally point is active until owner cancels or loses city.
    bool persistent;
    // Orders should be cleared if an enemy is met.
    bool vigilant;
    struct unit_order *orders;
  } rally_point;

  struct cm_parameter *cm_parameter;

  union {
    struct {
      /* Only used in the server (./ai/ and ./server/). */

      float migration_score;   // updated by city_migration_score.
      int mgr_score_calc_turn; // turn the migration score was calculated

      int illness;

      // If > 0, workers will not be rearranged until they are unfrozen.
      int workers_frozen;

      /* If set, workers need to be arranged when the city is unfrozen.
       * Set inside auto_arrange_workers() and city_freeze_workers_queue().
       */
      bool needs_arrange;

      /* If set, city needs to be refreshed at a later time.
       * Set inside city_refresh() and city_refresh_queue_add(). */
      bool needs_refresh;

      // the city map is synced with the client.
      bool synced;

      bool debug; // not saved

      struct adv_city *adv;
      void *ais[FREECIV_AI_MOD_LAST];

      struct vision *vision;

      /* when city production was changed by player */
      time_t prod_change_timestamp;
      int prod_change_turn;
    } server;

    struct {
      /* Only used at the client (the server is omniscient; ./client/). */
      bool full; // Did we get a full city info packet (owner or investigate)
      bool occupied;
      int walls;
      bool happy;
      bool unhappy;
      int city_image;
      int culture;
      int buy_cost;

      /* The color is an index into the city_colors array in mapview_common
       */
      bool colored;
      int color_index;

      /* info for dipl/spy investigation */
      struct unit_list *info_units_supported;
      struct unit_list *info_units_present;
      /* Before popup the city dialog, units go there. In normal process,
       * these pointers are set to nullptr. */
      struct unit_list *collecting_info_units_supported;
      struct unit_list *collecting_info_units_present;

      // Updates needed for the city.
      enum city_updates need_updates;

      unsigned char first_citizen_index;
    } client;
  };
};

struct citystyle {
  struct name_translation name;
  char graphic[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char citizens_graphic[MAX_LEN_NAME];
  char citizens_graphic_alt[MAX_LEN_NAME];
  struct requirement_vector reqs;
};

extern struct citystyle *city_styles;
extern const Output_type_id num_output_types;
extern struct output_type output_types[];

// get 'struct city_list' and related functions:
#define SPECLIST_TAG city
#define SPECLIST_TYPE struct city
#include "speclist.h"

#define city_list_iterate(citylist, pcity)                                  \
  TYPED_LIST_ITERATE(struct city, citylist, pcity)
#define city_list_iterate_end LIST_ITERATE_END

#define cities_iterate(pcity)                                               \
  {                                                                         \
    players_iterate(pcity##_player)                                         \
    {                                                                       \
      city_list_iterate(pcity##_player->cities, pcity)                      \
      {

#define cities_iterate_end                                                  \
  }                                                                         \
  city_list_iterate_end;                                                    \
  }                                                                         \
  players_iterate_end;                                                      \
  }

#define city_list_iterate_safe(citylist, _city)                             \
  {                                                                         \
    int _city##_size = city_list_size(citylist);                            \
                                                                            \
    if (_city##_size > 0) {                                                 \
      int _city##_numbers[_city##_size];                                    \
      int _city##_index = 0;                                                \
                                                                            \
      city_list_iterate(citylist, _city)                                    \
      {                                                                     \
        _city##_numbers[_city##_index++] = _city->id;                       \
      }                                                                     \
      city_list_iterate_end;                                                \
                                                                            \
      for (_city##_index = 0; _city##_index < _city##_size;                 \
           _city##_index++) {                                               \
        struct city *_city =                                                \
            game_city_by_number(_city##_numbers[_city##_index]);            \
                                                                            \
        if (nullptr != _city) {

#define city_list_iterate_safe_end                                          \
  }                                                                         \
  }                                                                         \
  }                                                                         \
  }

// output type functions

const char *get_output_identifier(Output_type_id output);
const char *get_output_name(Output_type_id output);
struct output_type *get_output_type(Output_type_id output);
Output_type_id output_type_by_identifier(const char *id);
void add_specialist_output(
    const struct city *pcity, int *output,
    const std::vector<std::array<int, O_LAST>> *pcsoutputs = nullptr);
void set_city_production(
    struct city *pcity, const std::vector<city *> &gov_centers,
    const std::array<cached_waste, O_LAST> *pcwaste = nullptr);

// properties

const char *city_name_get(const struct city *pcity);
struct player *city_owner(const struct city *pcity);
struct tile *city_tile(const struct city *pcity);

citizens city_size_get(const struct city *pcity);
void city_size_add(struct city *pcity, int add);
void city_size_set(struct city *pcity, citizens size);

citizens city_specialists(const struct city *pcity);

int player_base_citizen_happiness(const struct player *pplayer);
citizens player_content_citizens(const struct player *pplayer);
citizens player_angry_citizens(const struct player *pplayer);

int city_population(const struct city *pcity);
int city_total_impr_gold_upkeep(const struct city *pcity);
int city_total_unit_gold_upkeep(const struct city *pcity);
int city_unit_unhappiness(const unit *punit, int *free_unhappy);
bool city_happy(
    const struct city *pcity); // generally use celebrating instead
bool city_unhappy(const struct city *pcity); // anarchy???
bool base_city_celebrating(const struct city *pcity);
bool city_celebrating(const struct city *pcity); // love the king ???
bool city_rapture_grow(const struct city *pcity);
bool city_is_occupied(const struct city *pcity);

// city related improvement and unit functions

int city_improvement_upkeep(const struct city *pcity,
                            const struct impr_type *pimprove);

bool can_city_build_improvement_direct(const struct city *pcity,
                                       const struct impr_type *pimprove);
bool can_city_build_improvement_later(const struct city *pcity,
                                      const struct impr_type *pimprove);
bool can_city_build_improvement_now(const struct city *pcity,
                                    const struct impr_type *pimprove);

bool can_city_build_unit_direct(const struct city *pcity,
                                const struct unit_type *punittype);
bool can_city_build_unit_later(const struct city *pcity,
                               const struct unit_type *punittype);
bool can_city_build_unit_now(const struct city *pcity,
                             const struct unit_type *punittype);

bool can_city_build_direct(const struct city *pcity,
                           const struct universal *target);
bool can_city_build_later(const struct city *pcity,
                          const struct universal *target);
bool can_city_build_now(const struct city *pcity,
                        const struct universal *target);

int city_unit_slots_available(const struct city *pcity);
bool city_can_use_specialist(const struct city *pcity,
                             Specialist_type_id type);
bool city_has_building(const struct city *pcity,
                       const struct impr_type *pimprove);
bool is_capital(const struct city *pcity);
bool is_gov_center(const struct city *pcity);
bool city_got_defense_effect(const struct city *pcity,
                             const struct unit_type *attacker);

int city_production_build_shield_cost(const struct city *pcity);
bool city_production_build_units(const struct city *pcity,
                                 bool add_production, int *num_units);
int city_production_unit_veteran_level(struct city *pcity,
                                       const struct unit_type *punittype);

bool city_production_has_flag(const struct city *pcity,
                              enum impr_flag_id flag);
int city_production_turns_to_build(const struct city *pcity,
                                   bool include_shield_stock);

bool city_production_gets_caravan_shields(const struct universal *tgt);

int city_change_production_penalty(const struct city *pcity,
                                   const struct universal *target);
int city_turns_to_build(const struct city *pcity,
                        const struct universal *target,
                        bool include_shield_stock);
int city_turns_to_grow(const struct city *pcity);
bool city_can_grow_to(const struct city *pcity, int pop_size);
bool city_can_change_build(const struct city *pcity);

void city_choose_build_default(struct city *pcity);

// textual representation of buildings

const char *
city_improvement_name_translation(const struct city *pcity,
                                  const struct impr_type *pimprove);
const char *city_production_name_translation(const struct city *pcity);

// city map functions
bool is_valid_city_coords(const int city_radius_sq, const int city_map_x,
                          const int city_map_y);
bool city_map_includes_tile(const struct city *const pcity,
                            const struct tile *map_tile);
bool city_base_to_city_map(int *city_map_x, int *city_map_y,
                           const struct city *const pcity,
                           const struct tile *map_tile);
bool city_tile_to_city_map(int *city_map_x, int *city_map_y,
                           const int city_radius_sq,
                           const struct tile *city_center,
                           const struct tile *map_tile);

struct tile *city_map_to_tile(const struct tile *city_center,
                              int city_radius_sq, int city_map_x,
                              int city_map_y);

// Initialization functions
int compare_iter_index(const void *a, const void *b);
void generate_city_map_indices();
void free_city_map_index();
void city_production_caravan_shields_init();

// output on spot
int city_tile_output(const struct city *pcity, const struct tile *ptile,
                     bool is_celebrating, Output_type_id otype);
int city_tile_output_now(const struct city *pcity, const struct tile *ptile,
                         Output_type_id otype);

bool base_city_can_work_tile(const struct player *restriction,
                             const struct city *pcity,
                             const struct tile *ptile);
bool city_can_work_tile(const struct city *pcity, const struct tile *ptile);

bool citymindist_prevents_city_on_tile(const struct tile *ptile);

bool city_can_be_built_here(const struct tile *ptile,
                            const struct unit *punit);
bool city_can_be_built_tile_only(const struct tile *ptile);

// list functions
struct city *city_list_find_number(struct city_list *This, int id);
struct city *city_list_find_name(struct city_list *This, const char *name);

// city style functions
const char *city_style_rule_name(const int style);

int city_style_by_rule_name(const char *s);

struct city *is_enemy_city_tile(const struct tile *ptile,
                                const struct player *pplayer);
struct city *is_allied_city_tile(const struct tile *ptile,
                                 const struct player *pplayer);
struct city *is_non_attack_city_tile(const struct tile *ptile,
                                     const struct player *pplayer);
struct city *is_non_allied_city_tile(const struct tile *ptile,
                                     const struct player *pplayer);

bool is_unit_near_a_friendly_city(const struct unit *punit);
bool is_friendly_city_near(const struct player *owner,
                           const struct tile *ptile);
bool city_exists_within_max_city_map(const struct tile *ptile,
                                     bool may_be_on_center);

// granary size as a function of city size
int city_granary_size(int city_size);

void city_add_improvement(struct city *pcity,
                          const struct impr_type *pimprove);
void city_remove_improvement(struct city *pcity,
                             const struct impr_type *pimprove);

// city update functions
void city_refresh_from_main_map(
    struct city *pcity, bool *workers_map,
    const std::vector<city *> &gov_centers,
    const std::array<cached_waste, O_LAST> *pcwaste = nullptr,
    const std::vector<std::array<int, O_LAST>> *pcsoutputs = nullptr);
int city_waste(const struct city *pcity, Output_type_id otype, int total,
               int *breakdown, const std::vector<city *> &gov_centers,
               const cached_waste *pcwaste = nullptr);
Specialist_type_id best_specialist(Output_type_id otype,
                                   const struct city *pcity);
int get_final_city_output_bonus(const struct city *pcity,
                                Output_type_id otype);
bool city_built_last_turn(const struct city *pcity);

/* city creation / destruction */
struct city *create_city_virtual(struct player *pplayer, struct tile *ptile,
                                 const char *name);
void destroy_city_virtual(struct city *pcity);
bool city_is_virtual(const struct city *pcity);

// misc
bool is_city_option_set(const struct city *pcity, enum city_options option);
void city_styles_alloc(int num);
void city_styles_free();

void add_tax_income(const struct player *pplayer, int trade, int *output);
int get_city_tithes_bonus(const struct city *pcity);
int city_pollution_types(const struct city *pcity, int shield_total,
                         int trade_total, int *pollu_prod, int *pollu_trade,
                         int *pollu_pop, int *pollu_mod);
int city_pollution(const struct city *pcity, int shield_total,
                   int trade_total);
int city_illness_calc(const struct city *pcity, int *ill_base, int *ill_size,
                      int *ill_trade, int *ill_pollution);
bool city_had_recent_plague(const struct city *pcity);
int city_build_slots(const struct city *pcity);
int city_airlift_max(const struct city *pcity);

bool city_exist(int id);

/*
 * Iterates over all improvements, skipping those not yet built in the
 * given city.
 */
#define city_built_iterate(_pcity, _p)                                      \
  improvement_iterate(_p)                                                   \
  {                                                                         \
    if ((_pcity)->built[improvement_index(_p)].turn <= I_NEVER) {           \
      continue;                                                             \
    }

#define city_built_iterate_end                                              \
  }                                                                         \
  improvement_iterate_end;

// Iterates over all output types in the game.
#define output_type_iterate(output)                                         \
  {                                                                         \
    int ioutput;                                                            \
                                                                            \
    for (ioutput = 0; ioutput < O_LAST; ioutput++) {                        \
      Output_type_id output = (enum output_type_id) ioutput;

#define output_type_iterate_end                                             \
  }                                                                         \
  }

// ===

bool is_city_center(const struct city *pcity, const struct tile *ptile);
#define is_city_center_index(city_tile_index)                               \
  (CITY_MAP_CENTER_TILE_INDEX == city_tile_index)

void *city_ai_data(const struct city *pcity, const struct ai_type *ai);
void city_set_ai_data(struct city *pcity, const struct ai_type *ai,
                      void *data);
