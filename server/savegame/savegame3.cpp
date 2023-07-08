/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2021 Freeciv21 and Freeciv
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

/*
  This file includes the definition of a new savegame format introduced with
  3.0. It is defined by the mandatory option '+version3'. The main load
  function checks if this option is present. If not, the old (pre-3.0)
  loading routines are used.
  The format version is also saved in the settings section of the savefile,
  as an integer (savefile.version). The integer is used to determine the
  version of the savefile.

  Structure of this file:

  - The main function for saving is savegame3_save().

  - The real work is done by savegame3_load() and savegame3_save_real().
    This function call all submodules (settings, players, etc.)

  - The remaining part of this file is split into several sections:
     * helper functions
     * save / load functions for all submodules (and their subsubmodules)

  - If possible, all functions for load / save submodules should exit in
    pairs named sg_load_<submodule> and sg_save_<submodule>. If one is not
    needed please add a comment why.

  - The submodules can be further divided as:
    sg_load_<submodule>_<subsubmodule>

  - If needed (due to static variables in the *.c files) these functions
    can be located in the corresponding source files (as done for the
  settings and the event_cache).

  Creating a savegame:

  (nothing at the moment)

  Loading a savegame:

  - The status of the process is saved within the static variable
    'sg_success'. This variable is set to TRUE within savegame3_load().
    If you encounter an error use sg_failure_*() to set it to FALSE and
    return an error message. Furthermore, sg_check_* should be used at the
    start of each (submodule) function to return if previous functions
  failed.

  - While the loading process dependencies between different modules exits.
    They can be handled within the struct loaddata *loading which is used as
    first argument for all sg_load_*() function. Please indicate the
    dependencies within the definition of this struct.

*/

#include <fc_config.h>

#include <QBitArray>
#include <QSet>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

// utility
#include "bitvector.h"
#include "capability.h"
#include "fcintl.h"
#include "idex.h"
#include "log.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h" // bool type
#include "timing.h"

// generated
#include "fc_version.h"

// common
#include "achievements.h"
#include "ai.h"
#include "citizens.h"
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "mapimg.h"
#include "movement.h"
#include "multipliers.h"
#include "packets.h"
#include "research.h"
#include "rgbcolor.h"
#include "specialist.h"
#include "style.h"
#include "unit.h"
#include "unitlist.h"
#include "version.h"

// server
#include "citizenshand.h"
#include "citytools.h"
#include "cityturn.h"
#include "diplhand.h"
#include "maphand.h"
#include "meta.h"
#include "notify.h"
#include "plrhand.h"
#include "report.h"
#include "ruleset.h"
#include "sanitycheck.h"
#include "savecompat.h"
#include "settings.h"
#include "spacerace.h"
#include "srv_main.h"
#include "techtools.h"
#include "unittools.h"

/* server/advisors */
#include "advbuilding.h"
#include "advdata.h"
#include "infracache.h"

/* server/generator */
#include "mapgen_utils.h"

/* server/scripting */
#include "script_server.h"

// ai
#include "aitraits.h"
#include "difficulty.h"

#include "savegame3.h"

extern bool sg_success;

#ifdef FREECIV_TESTMATIC
#define SAVE_DUMMY_TURN_CHANGE_TIME 1
#endif

/*
 * This loops over the entire map to save data. It collects all the data of
 * a line using GET_XY_CHAR and then executes the macro SECFILE_INSERT_LINE.
 *
 * Parameters:
 *   ptile:         current tile within the line (used by GET_XY_CHAR)
 *   GET_XY_CHAR:   macro returning the map character for each position
 *   secfile:       a secfile struct
 *   secpath, ...:  path as used for sprintf() with arguments; the last item
 *                  will be the y coordinate
 * Example:
 *   SAVE_MAP_CHAR(ptile, terrain2char(ptile->terrain), file, "map.t%04d");
 */
#define SAVE_MAP_CHAR(ptile, GET_XY_CHAR, secfile, secpath, ...)            \
  {                                                                         \
    char _line[wld.map.xsize + 1];                                          \
    int _nat_x, _nat_y;                                                     \
                                                                            \
    for (_nat_y = 0; _nat_y < wld.map.ysize; _nat_y++) {                    \
      for (_nat_x = 0; _nat_x < wld.map.xsize; _nat_x++) {                  \
        struct tile *ptile =                                                \
            native_pos_to_tile(&(wld.map), _nat_x, _nat_y);                 \
        fc_assert_action(ptile != nullptr, continue);                       \
        _line[_nat_x] = (GET_XY_CHAR);                                      \
        sg_failure_ret(QChar::isPrint(_line[_nat_x] & 0x7f),                \
                       "Trying to write invalid map data at position "      \
                       "(%d, %d) for path %s: '%c' (%d)",                   \
                       _nat_x, _nat_y, secpath, _line[_nat_x],              \
                       _line[_nat_x]);                                      \
      }                                                                     \
      _line[wld.map.xsize] = '\0';                                          \
      secfile_insert_str(secfile, _line, secpath, ##__VA_ARGS__, _nat_y);   \
    }                                                                       \
  }

/*
 * This loops over the entire map to load data. It inputs a line of data
 * using the macro SECFILE_LOOKUP_LINE and then loops using the macro
 * SET_XY_CHAR to load each char into the map at (map_x, map_y). Internal
 * variables ch, map_x, map_y, nat_x, and nat_y are allocated within the
 * macro but definable by the caller.
 *
 * Parameters:
 *   ch:            a variable to hold a char (data for a single position,
 *                  used by SET_XY_CHAR)
 *   ptile:         current tile within the line (used by SET_XY_CHAR)
 *   SET_XY_CHAR:   macro to load the map character at each (map_x, map_y)
 *   secfile:       a secfile struct
 *   secpath, ...:  path as used for sprintf() with arguments; the last item
 *                  will be the y coordinate
 * Example:
 *   LOAD_MAP_CHAR(ch, ptile,
 *                 map_get_player_tile(ptile, plr)->terrain
 *                   = char2terrain(ch), file, "player%d.map_t%04d", plrno);
 *
 * Note: some (but not all) of the code this is replacing used to skip over
 *       lines that did not exist. This allowed for backward-compatibility.
 *       We could add another parameter that specified whether it was OK to
 *       skip the data, but there's not really much advantage to exiting
 *       early in this case. Instead, we let any map data type to be empty,
 *       and just print an informative warning message about it.
 */
#define LOAD_MAP_CHAR(ch, ptile, SET_XY_CHAR, secfile, secpath, ...)        \
  {                                                                         \
    int _nat_x, _nat_y;                                                     \
    bool _printed_warning = false;                                          \
    for (_nat_y = 0; _nat_y < wld.map.ysize; _nat_y++) {                    \
      const char *_line =                                                   \
          secfile_lookup_str(secfile, secpath, ##__VA_ARGS__, _nat_y);      \
      if (nullptr == _line) {                                               \
        char buf[64];                                                       \
        fc_snprintf(buf, sizeof(buf), secpath, ##__VA_ARGS__, _nat_y);      \
        qDebug("Line not found='%s'", buf);                                 \
        _printed_warning = true;                                            \
        continue;                                                           \
      } else if (strlen(_line) != wld.map.xsize) {                          \
        char buf[64];                                                       \
        fc_snprintf(buf, sizeof(buf), secpath, ##__VA_ARGS__, _nat_y);      \
        qDebug("Line too short (expected %d got %lu)='%s'", wld.map.xsize,  \
               (unsigned long) qstrlen(_line), buf);                        \
        _printed_warning = true;                                            \
        continue;                                                           \
      }                                                                     \
      for (_nat_x = 0; _nat_x < wld.map.xsize; _nat_x++) {                  \
        const char ch = _line[_nat_x];                                      \
        struct tile *ptile =                                                \
            native_pos_to_tile(&(wld.map), _nat_x, _nat_y);                 \
        (SET_XY_CHAR);                                                      \
      }                                                                     \
    }                                                                       \
    if (_printed_warning) {                                                 \
      /* TRANS: Minor error message. */                                     \
      log_sg(_("Saved game contains incomplete map data. This can"          \
               " happen with old saved games, or it may indicate an"        \
               " invalid saved game file. Proceed at your own risk."));     \
    }                                                                       \
  }

// Iterate on the extras half-bytes
#define halfbyte_iterate_extras(e, num_extras_types)                        \
  {                                                                         \
    int e;                                                                  \
    for (e = 0; 4 * e < (num_extras_types); e++) {

#define halfbyte_iterate_extras_end                                         \
  }                                                                         \
  }

struct savedata {
  struct section_file *file;
  char secfile_options[512];

  // set by the caller
  const char *save_reason;
  bool scenario;

  // Set in sg_save_game(); needed in sg_save_map_*(); ...
  bool save_players;
};

#define TOKEN_SIZE 10

static const char savefile_options_default[] = " +version3";
/* The following savefile option are added if needed:
 *  - nothing at current version
 * See also calls to sg_save_savefile_options(). */

static void savegame3_save_real(struct section_file *file,
                                const char *save_reason, bool scenario);
static struct loaddata *loaddata_new(struct section_file *file);
static void loaddata_destroy(struct loaddata *loading);

static struct savedata *savedata_new(struct section_file *file,
                                     const char *save_reason, bool scenario);
static void savedata_destroy(struct savedata *saving);

static enum unit_orders char2order(char order);
static char order2char(enum unit_orders order);
static enum direction8 char2dir(char dir);
static char dir2char(enum direction8 dir);
static char activity2char(enum unit_activity activity);
static enum unit_activity char2activity(char activity);
static char *quote_block(const void *const data, int length);
static int unquote_block(const char *const quoted_, void *dest,
                         int dest_length);
static void worklist_load(struct section_file *file, struct worklist *pwl,
                          const char *path, ...);
static void worklist_save(struct section_file *file,
                          const struct worklist *pwl, int max_length,
                          const char *path, ...);
static void unit_ordering_calc();
static void unit_ordering_apply();
static void sg_extras_set(bv_extras *extras, char ch,
                          struct extra_type **idx);
static char sg_extras_get(bv_extras extras, struct extra_type *presource,
                          const int *idx);
static struct terrain *char2terrain(char ch);
static char terrain2char(const struct terrain *pterrain);
static Tech_type_id technology_load(struct section_file *file,
                                    const char *path, int plrno);
static void technology_save(struct section_file *file, const char *path,
                            int plrno, Tech_type_id tech);

static void sg_load_savefile(struct loaddata *loading);
static void sg_save_savefile(struct savedata *saving);
static void sg_save_savefile_options(struct savedata *saving,
                                     const char *option);

static void sg_load_game(struct loaddata *loading);
static void sg_save_game(struct savedata *saving);

static void sg_load_ruledata(struct loaddata *loading);
static void sg_save_ruledata(struct savedata *saving);

static void sg_load_random(struct loaddata *loading);
static void sg_save_random(struct savedata *saving);

static void sg_load_script(struct loaddata *loading);
static void sg_save_script(struct savedata *saving);

static void sg_load_scenario(struct loaddata *loading);
static void sg_save_scenario(struct savedata *saving);

static void sg_load_settings(struct loaddata *loading);
static void sg_save_settings(struct savedata *saving);

static void sg_load_map(struct loaddata *loading);
static void sg_save_map(struct savedata *saving);
static void sg_load_map_tiles(struct loaddata *loading);
static void sg_save_map_tiles(struct savedata *saving);
static void sg_load_map_tiles_extras(struct loaddata *loading);
static void sg_save_map_tiles_extras(struct savedata *saving);

static void sg_load_map_startpos(struct loaddata *loading);
static void sg_save_map_startpos(struct savedata *saving);
static void sg_load_map_owner(struct loaddata *loading);
static void sg_save_map_owner(struct savedata *saving);
static void sg_load_map_worked(struct loaddata *loading);
static void sg_save_map_worked(struct savedata *saving);
static void sg_load_map_known(struct loaddata *loading);
static void sg_save_map_known(struct savedata *saving);

static void sg_load_players_basic(struct loaddata *loading);
static void sg_load_players(struct loaddata *loading);
static void sg_load_player_main(struct loaddata *loading,
                                struct player *plr);
static void sg_load_player_cities(struct loaddata *loading,
                                  struct player *plr);
static bool sg_load_player_city(struct loaddata *loading, struct player *plr,
                                struct city *pcity, const char *citystr);
static void sg_load_player_city_citizens(struct loaddata *loading,
                                         struct player *plr,
                                         struct city *pcity,
                                         const char *citystr);
static void sg_load_player_units(struct loaddata *loading,
                                 struct player *plr);
static bool sg_load_player_unit(struct loaddata *loading, struct player *plr,
                                struct unit *punit, const char *unitstr);
static void sg_load_player_units_transport(struct loaddata *loading,
                                           struct player *plr);
static void sg_load_player_attributes(struct loaddata *loading,
                                      struct player *plr);
static void sg_load_player_vision(struct loaddata *loading,
                                  struct player *plr);
static bool sg_load_player_vision_city(struct loaddata *loading,
                                       struct player *plr,
                                       struct vision_site *pdcity,
                                       const char *citystr);
static void sg_save_players(struct savedata *saving);
static void sg_save_player_main(struct savedata *saving, struct player *plr);
static void sg_save_player_cities(struct savedata *saving,
                                  struct player *plr);
static void sg_save_player_units(struct savedata *saving,
                                 struct player *plr);
static void sg_save_player_attributes(struct savedata *saving,
                                      struct player *plr);
static void sg_save_player_vision(struct savedata *saving,
                                  struct player *plr);

static void sg_load_researches(struct loaddata *loading);
static void sg_save_researches(struct savedata *saving);

static void sg_load_event_cache(struct loaddata *loading);
static void sg_save_event_cache(struct savedata *saving);

static void sg_load_treaties(struct loaddata *loading);
static void sg_save_treaties(struct savedata *saving);

static void sg_load_history(struct loaddata *loading);
static void sg_save_history(struct savedata *saving);

static void sg_load_mapimg(struct loaddata *loading);
static void sg_save_mapimg(struct savedata *saving);

static void sg_load_sanitycheck(struct loaddata *loading);
static void sg_save_sanitycheck(struct savedata *saving);

/**
   Main entry point for saving a game in savegame3 format.
 */
void savegame3_save(struct section_file *sfile, const char *save_reason,
                    bool scenario)
{
  fc_assert_ret(sfile != nullptr);

  civtimer *savetimer = timer_new(TIMER_CPU, TIMER_DEBUG);
  timer_start(savetimer);

  qDebug("saving game in new format ...");
  savegame3_save_real(sfile, save_reason, scenario);

  timer_stop(savetimer);
  qCDebug(timers_category, "Creating secfile in %.3f seconds.",
          timer_read_seconds(savetimer));
  timer_destroy(savetimer);
}

/* =======================================================================
 * Basic load / save functions.
 * ======================================================================= */

/**
   Really loading the savegame.
 */
void savegame3_load(struct section_file *file)
{
  struct loaddata *loading;
  bool was_send_city_suppressed, was_send_tile_suppressed;

  // initialise loading
  was_send_city_suppressed = send_city_suppression(true);
  was_send_tile_suppressed = send_tile_suppression(true);
  loading = loaddata_new(file);
  sg_success = true;

  // Load the savegame data.
  // [compat]
  sg_load_compat(loading, SAVEGAME_3);
  // [scenario]
  sg_load_scenario(loading);
  // [savefile]
  sg_load_savefile(loading);
  // [game]
  sg_load_game(loading);
  // [random]
  sg_load_random(loading);
  // [settings]
  sg_load_settings(loading);
  // [ruledata]
  sg_load_ruledata(loading);
  // [players] (basic data)
  sg_load_players_basic(loading);
  // [map]; needs width and height loaded by [settings]
  sg_load_map(loading);
  // [research]
  sg_load_researches(loading);
  // [player<i>]
  sg_load_players(loading);
  // [event_cache]
  sg_load_event_cache(loading);
  // [treaties]
  sg_load_treaties(loading);
  // [history]
  sg_load_history(loading);
  // [mapimg]
  sg_load_mapimg(loading);
  // [script] -- must come last as may reference game objects
  sg_load_script(loading);
  // [post_load_compat]; needs the game loaded by [savefile]
  sg_load_post_load_compat(loading, SAVEGAME_3);

  // Sanity checks for the loaded game.
  sg_load_sanitycheck(loading);

  // deinitialise loading
  loaddata_destroy(loading);
  send_tile_suppression(was_send_tile_suppressed);
  send_city_suppression(was_send_city_suppressed);

  if (!sg_success) {
    qCritical("Failure loading savegame!");
    // Try to get the server back to a vaguely sane state
    server_game_free();
    server_game_init(false);
    load_rulesets(nullptr, nullptr, false, nullptr, true, false, true);
  }
}

/**
   Really save the game to a file.
 */
static void savegame3_save_real(struct section_file *file,
                                const char *save_reason, bool scenario)
{
  struct savedata *saving;

  // initialise loading
  saving = savedata_new(file, save_reason, scenario);
  sg_success = true;

  // [scenario]
  /* This should be first section so scanning through all scenarios just for
   * names and descriptions would go faster. */
  sg_save_scenario(saving);
  // [savefile]
  sg_save_savefile(saving);
  // [game]
  sg_save_game(saving);
  // [random]
  sg_save_random(saving);
  // [script]
  sg_save_script(saving);
  // [settings]
  sg_save_settings(saving);
  // [ruledata]
  sg_save_ruledata(saving);
  // [map]
  sg_save_map(saving);
  // [player<i>]
  sg_save_players(saving);
  // [research]
  sg_save_researches(saving);
  // [event_cache]
  sg_save_event_cache(saving);
  // [treaty<i>]
  sg_save_treaties(saving);
  // [history]
  sg_save_history(saving);
  // [mapimg]
  sg_save_mapimg(saving);

  // Sanity checks for the saved game.
  sg_save_sanitycheck(saving);

  // deinitialise saving
  savedata_destroy(saving);

  if (!sg_success) {
    qCritical("Failure saving savegame!");
  }
}

/**
   Create new loaddata item for given section file.
 */
static struct loaddata *loaddata_new(struct section_file *file)
{
  struct loaddata *loading = new loaddata{};
  loading->file = file;
  loading->secfile_options = nullptr;

  loading->improvement.order = nullptr;
  loading->improvement.size = -1;
  loading->technology.order = nullptr;
  loading->technology.size = -1;
  loading->activities.order = nullptr;
  loading->activities.size = -1;
  loading->trait.order = nullptr;
  loading->trait.size = -1;
  loading->extra.order = nullptr;
  loading->extra.size = -1;
  loading->multiplier.order = nullptr;
  loading->multiplier.size = -1;
  loading->specialist.order = nullptr;
  loading->specialist.size = -1;
  loading->action.order = nullptr;
  loading->action.size = -1;
  loading->act_dec.order = nullptr;
  loading->act_dec.size = -1;
  loading->ssa.order = nullptr;
  loading->ssa.size = -1;

  loading->server_state = S_S_INITIAL;
  loading->rstate = fc_rand_state();
  loading->worked_tiles = nullptr;

  return loading;
}

/**
   Free resources allocated for loaddata item.
 */
static void loaddata_destroy(struct loaddata *loading)
{
  delete[] loading->improvement.order;
  delete[] loading->technology.order;
  delete[] loading->activities.order;
  delete[] loading->trait.order;
  delete[] loading->extra.order;
  delete[] loading->multiplier.order;
  delete[] loading->specialist.order;
  delete[] loading->action.order;
  delete[] loading->act_dec.order;
  delete[] loading->ssa.order;
  delete[] loading->worked_tiles;
  delete loading;
  loading = nullptr;
}

/**
   Create new savedata item for given file.
 */
static struct savedata *savedata_new(struct section_file *file,
                                     const char *save_reason, bool scenario)
{
  struct savedata *saving =
      static_cast<savedata *>(calloc(1, sizeof(*saving)));
  saving->file = file;
  saving->secfile_options[0] = '\0';

  saving->save_reason = save_reason;
  saving->scenario = scenario;

  saving->save_players = false;

  return saving;
}

/**
   Free resources allocated for savedata item
 */
static void savedata_destroy(struct savedata *saving) { free(saving); }

/* =======================================================================
 * Helper functions.
 * ======================================================================= */

/**
   Returns an order for a character identifier.  See also order2char.
 */
static enum unit_orders char2order(char order)
{
  switch (order) {
  case 'm':
  case 'M':
    return ORDER_MOVE;
  case 'w':
  case 'W':
    return ORDER_FULL_MP;
  case 'a':
  case 'A':
    return ORDER_ACTIVITY;
  case 'x':
  case 'X':
    return ORDER_ACTION_MOVE;
  case 'p':
  case 'P':
    return ORDER_PERFORM_ACTION;
  }

  // This can happen if the savegame is invalid.
  return ORDER_LAST;
}

/**
   Returns a character identifier for an order.  See also char2order.
 */
static char order2char(enum unit_orders order)
{
  switch (order) {
  case ORDER_MOVE:
    return 'm';
  case ORDER_FULL_MP:
    return 'w';
  case ORDER_ACTIVITY:
    return 'a';
  case ORDER_ACTION_MOVE:
    return 'x';
  case ORDER_PERFORM_ACTION:
    return 'p';
  case ORDER_LAST:
    break;
  }

  fc_assert(false);
  return '?';
}

/**
   Returns a direction for a character identifier.  See also dir2char.
 */
static enum direction8 char2dir(char dir)
{
  // Numberpad values for the directions.
  switch (dir) {
  case '1':
    return DIR8_SOUTHWEST;
  case '2':
    return DIR8_SOUTH;
  case '3':
    return DIR8_SOUTHEAST;
  case '4':
    return DIR8_WEST;
  case '6':
    return DIR8_EAST;
  case '7':
    return DIR8_NORTHWEST;
  case '8':
    return DIR8_NORTH;
  case '9':
    return DIR8_NORTHEAST;
  }

  // This can happen if the savegame is invalid.
  return direction8_invalid();
}

/**
   Returns a character identifier for a direction.  See also char2dir.
 */
static char dir2char(enum direction8 dir)
{
  // Numberpad values for the directions.
  switch (dir) {
  case DIR8_NORTH:
    return '8';
  case DIR8_SOUTH:
    return '2';
  case DIR8_EAST:
    return '6';
  case DIR8_WEST:
    return '4';
  case DIR8_NORTHEAST:
    return '9';
  case DIR8_NORTHWEST:
    return '7';
  case DIR8_SOUTHEAST:
    return '3';
  case DIR8_SOUTHWEST:
    return '1';
  }

  fc_assert(false);
  return '?';
}

/**
   Returns a character identifier for an activity.  See also char2activity.
 */
static char activity2char(enum unit_activity activity)
{
  switch (activity) {
  case ACTIVITY_IDLE:
    return 'w';
  case ACTIVITY_POLLUTION:
    return 'p';
  case ACTIVITY_OLD_ROAD:
    return 'r';
  case ACTIVITY_MINE:
    return 'm';
  case ACTIVITY_PLANT:
    return 'M';
  case ACTIVITY_IRRIGATE:
    return 'i';
  case ACTIVITY_CULTIVATE:
    return 'I';
  case ACTIVITY_FORTIFIED:
    return 'f';
  case ACTIVITY_FORTRESS:
    return 't';
  case ACTIVITY_SENTRY:
    return 's';
  case ACTIVITY_OLD_RAILROAD:
    return 'l';
  case ACTIVITY_PILLAGE:
    return 'e';
  case ACTIVITY_GOTO:
    return 'g';
  case ACTIVITY_EXPLORE:
    return 'x';
  case ACTIVITY_TRANSFORM:
    return 'o';
  case ACTIVITY_AIRBASE:
    return 'a';
  case ACTIVITY_FORTIFYING:
    return 'y';
  case ACTIVITY_FALLOUT:
    return 'u';
  case ACTIVITY_BASE:
    return 'b';
  case ACTIVITY_GEN_ROAD:
    return 'R';
  case ACTIVITY_CONVERT:
    return 'c';
  case ACTIVITY_UNKNOWN:
  case ACTIVITY_PATROL_UNUSED:
    return '?';
  case ACTIVITY_LAST:
    break;
  }

  fc_assert(false);
  return '?';
}

/**
   Returns an activity for a character identifier.  See also activity2char.
 */
static enum unit_activity char2activity(char activity)
{
  int a;

  for (a = 0; a < ACTIVITY_LAST; a++) {
    char achar = activity2char(static_cast<unit_activity>(a));

    if (activity == achar) {
      return static_cast<unit_activity>(a);
    }
  }

  // This can happen if the savegame is invalid.
  return ACTIVITY_LAST;
}

/**
   Quote the memory block denoted by data and length so it consists only of
   " a-f0-9:". The returned string has to be freed by the caller using
 free().
 */
static char *quote_block(const void *const data, int length)
{
  char *buffer = new char[length * 3 + 10];
  size_t offset;
  int i;

  sprintf(buffer, "%d:", length);
  offset = qstrlen(buffer);

  for (i = 0; i < length; i++) {
    sprintf(buffer + offset, "%02x ", ((unsigned char *) data)[i]);
    offset += 3;
  }
  return buffer;
}

/**
   Unquote a string. The unquoted data is written into dest. If the unquoted
   data will be larger than dest_length the function aborts. It returns the
   actual length of the unquoted block.
 */
static int unquote_block(const char *const quoted_, void *dest,
                         int dest_length)
{
  int i, length, parsed, tmp;
  char *endptr;
  const char *quoted = quoted_;

  parsed = sscanf(quoted, "%d", &length);
  fc_assert_ret_val(1 == parsed, 0);

  if (length > dest_length) {
    return 0;
  }
  quoted = strchr(quoted, ':');
  fc_assert_ret_val(quoted != nullptr, 0);
  quoted++;

  for (i = 0; i < length; i++) {
    tmp = strtol(quoted, &endptr, 16);
    fc_assert_ret_val((endptr - quoted) == 2, 0);
    fc_assert_ret_val(*endptr == ' ', 0);
    fc_assert_ret_val((tmp & 0xff) == tmp, 0);
    (static_cast<unsigned char *>(dest))[i] = tmp;
    quoted += 3;
  }
  return length;
}

/**
   Load the worklist elements specified by path to the worklist pointed to
   by 'pwl'. 'pwl' should be a pointer to an existing worklist.
 */
static void worklist_load(struct section_file *file, struct worklist *pwl,
                          const char *path, ...)
{
  int i;
  const char *kind;
  const char *name;
  char path_str[1024];
  va_list ap;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  fc_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  worklist_init(pwl);
  pwl->length =
      secfile_lookup_int_default(file, 0, "%s.wl_length", path_str);

  for (i = 0; i < pwl->length; i++) {
    kind = secfile_lookup_str(file, "%s.wl_kind%d", path_str, i);

    /* We lookup the production value by name. An invalid entry isn't a
     * fatal error; we just truncate the worklist. */
    name =
        secfile_lookup_str_default(file, "-", "%s.wl_value%d", path_str, i);
    pwl->entries[i] = universal_by_rule_name(kind, name);
    if (pwl->entries[i].kind == universals_n_invalid()) {
      log_sg("%s.wl_value%d: unknown \"%s\" \"%s\".", path_str, i, kind,
             name);
      pwl->length = i;
      break;
    }
  }
}

/**
   Save the worklist elements specified by path from the worklist pointed to
   by 'pwl'. 'pwl' should be a pointer to an existing worklist.
 */
static void worklist_save(struct section_file *file,
                          const struct worklist *pwl, int max_length,
                          const char *path, ...)
{
  char path_str[1024];
  int i;
  va_list ap;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  fc_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  secfile_insert_int(file, pwl->length, "%s.wl_length", path_str);

  for (i = 0; i < pwl->length; i++) {
    const struct universal *entry = pwl->entries + i;
    secfile_insert_str(file, universal_type_rule_name(entry), "%s.wl_kind%d",
                       path_str, i);
    secfile_insert_str(file, universal_rule_name(entry), "%s.wl_value%d",
                       path_str, i);
  }

  fc_assert_ret(max_length <= MAX_LEN_WORKLIST);

  /* We want to keep savegame in tabular format, so each line has to be
   * of equal length. Fill table up to maximum worklist size. */
  for (i = pwl->length; i < max_length; i++) {
    secfile_insert_str(file, "", "%s.wl_kind%d", path_str, i);
    secfile_insert_str(file, "", "%s.wl_value%d", path_str, i);
  }
}

/**
   Assign values to ord_city and ord_map for each unit, so the values can be
   saved.
 */
static void unit_ordering_calc()
{
  int j;

  players_iterate(pplayer)
  {
    // to avoid junk values for unsupported units:
    unit_list_iterate(pplayer->units, punit) { punit->server.ord_city = 0; }
    unit_list_iterate_end;
    city_list_iterate(pplayer->cities, pcity)
    {
      j = 0;
      unit_list_iterate(pcity->units_supported, punit)
      {
        punit->server.ord_city = j++;
      }
      unit_list_iterate_end;
    }
    city_list_iterate_end;
  }
  players_iterate_end;

  whole_map_iterate(&(wld.map), ptile)
  {
    j = 0;
    unit_list_iterate(ptile->units, punit) { punit->server.ord_map = j++; }
    unit_list_iterate_end;
  }
  whole_map_iterate_end;
}

/**
   For each city and tile, sort unit lists according to ord_city and ord_map
   values.
 */
static void unit_ordering_apply()
{
  players_iterate(pplayer)
  {
    city_list_iterate(pplayer->cities, pcity)
    {
      unit_list_sort_ord_city(pcity->units_supported);
    }
    city_list_iterate_end;
  }
  players_iterate_end;

  whole_map_iterate(&(wld.map), ptile)
  {
    unit_list_sort_ord_map(ptile->units);
  }
  whole_map_iterate_end;
}

/**
   Helper function for loading extras from a savegame.

   'ch' gives the character loaded from the savegame. Extras are packed
   in four to a character in hex notation. 'index' is a mapping of
   savegame bit -> base bit.
 */
static void sg_extras_set(bv_extras *extras, char ch,
                          struct extra_type **idx)
{
  int i, bin;
  const char *pch = strchr(hex_chars, ch);

  if (!pch || ch == '\0') {
    log_sg("Unknown hex value: '%c' (%d)", ch, ch);
    bin = 0;
  } else {
    bin = pch - hex_chars;
  }

  for (i = 0; i < 4; i++) {
    struct extra_type *pextra = idx[i];

    if (pextra == nullptr) {
      continue;
    }
    if ((bin & (1 << i))
        && (wld.map.server.have_huts
            || !is_extra_caused_by(pextra, EC_HUT))) {
      BV_SET(*extras, extra_index(pextra));
    }
  }
}

/**
   Helper function for saving extras into a savegame.

   Extras are packed in four to a character in hex notation. 'index'
   specifies which set of extras are included in this character.
 */
static char sg_extras_get(bv_extras extras, struct extra_type *presource,
                          const int *idx)
{
  int i, bin = 0;

  for (i = 0; i < 4; i++) {
    int extra = idx[i];

    if (extra < 0) {
      break;
    }

    if (BV_ISSET(extras, extra)
        /* An invalid resource, a resource that can't exist at the tile's
         * current terrain, isn't in the bit extra vector. Save it so it
         * can return if the tile's terrain changes to something it can
         * exist on. */
        || extra_by_number(extra) == presource) {
      bin |= (1 << i);
    }
  }

  return hex_chars[bin];
}

/**
   Dereferences the terrain character.  See terrains[].identifier
     example: char2terrain('a') => T_ARCTIC
 */
static struct terrain *char2terrain(char ch)
{
  // terrain_by_identifier plus fatal error
  if (ch == TERRAIN_UNKNOWN_IDENTIFIER) {
    return T_UNKNOWN;
  }
  terrain_type_iterate(pterrain)
  {
    if (pterrain->identifier_load == ch) {
      return pterrain;
    }
  }
  terrain_type_iterate_end;

  qFatal("Unknown terrain identifier '%c' in savegame.", ch);
  exit(EXIT_FAILURE);
}

/**
   References the terrain character.  See terrains[].identifier
     example: terrain2char(T_ARCTIC) => 'a'
 */
static char terrain2char(const struct terrain *pterrain)
{
  if (pterrain == T_UNKNOWN) {
    return TERRAIN_UNKNOWN_IDENTIFIER;
  } else {
    return pterrain->identifier;
  }
}

/**
   Load technology from path_name and if doesn't exist (because savegame
   is too old) load from path.
 */
static Tech_type_id technology_load(struct section_file *file,
                                    const char *path, int plrno)
{
  char path_with_name[128];
  const char *name;
  struct advance *padvance;

  fc_snprintf(path_with_name, sizeof(path_with_name), "%s_name", path);

  name = secfile_lookup_str(file, path_with_name, plrno);

  if (!name || name[0] == '\0') {
    // used by researching_saved
    return A_UNKNOWN;
  }
  if (fc_strcasecmp(name, "A_FUTURE") == 0) {
    return A_FUTURE;
  }
  if (fc_strcasecmp(name, "A_NONE") == 0) {
    return A_NONE;
  }
  if (fc_strcasecmp(name, "A_UNSET") == 0) {
    return A_UNSET;
  }

  padvance = advance_by_rule_name(name);
  sg_failure_ret_val(nullptr != padvance, A_NONE,
                     "%s: unknown technology \"%s\".", path_with_name, name);

  return advance_number(padvance);
}

/**
   Save technology in secfile entry called path_name.
 */
static void technology_save(struct section_file *file, const char *path,
                            int plrno, Tech_type_id tech)
{
  char path_with_name[128];
  const char *name;

  fc_snprintf(path_with_name, sizeof(path_with_name), "%s_name", path);

  switch (tech) {
  case A_UNKNOWN: // used by researching_saved
    name = "";
    break;
  case A_NONE:
    name = "A_NONE";
    break;
  case A_UNSET:
    name = "A_UNSET";
    break;
  case A_FUTURE:
    name = "A_FUTURE";
    break;
  default:
    name = advance_rule_name(advance_by_number(tech));
    break;
  }

  secfile_insert_str(file, name, path_with_name, plrno);
}

/* =======================================================================
 * Load / save savefile data.
 * ======================================================================= */

/**
   Load '[savefile]'.
 */
static void sg_load_savefile(struct loaddata *loading)
{
  int i;
  const char *terr_name;
  bool ruleset_datafile;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Load savefile options.
  loading->secfile_options =
      secfile_lookup_str(loading->file, "savefile.options");

  /* We don't need these entries, but read them anyway to avoid
   * warnings about unread secfile entries. */
  (void) secfile_entry_by_path(loading->file, "savefile.reason");
  (void) secfile_entry_by_path(loading->file, "savefile.revision");

  ruleset_datafile = game.scenario.datafile[0] == '\0';

  if (!game.scenario.is_scenario || game.scenario.ruleset_locked) {
    const char *ruleset, *alt_dir;

    ruleset = secfile_lookup_str_default(
        loading->file, GAME_DEFAULT_RULESETDIR, "savefile.rulesetdir");

    // Load ruleset.
    sz_strlcpy(game.server.rulesetdir, ruleset);
    if (!strcmp("default", game.server.rulesetdir)) {
      /* Here 'default' really means current default.
       * Saving happens with real ruleset name, so savegames containing this
       * are special scenarios. */
      sz_strlcpy(game.server.rulesetdir, GAME_DEFAULT_RULESETDIR);
      qDebug("Savegame specified ruleset '%s'. Really loading '%s'.",
             ruleset, game.server.rulesetdir);
    }

    alt_dir = secfile_lookup_str_default(loading->file, nullptr,
                                         "savefile.ruleset_alt_dir");

    if (!load_rulesets(nullptr, alt_dir, false, nullptr, true, false,
                       ruleset_datafile)) {
      if (alt_dir) {
        sg_failure_ret(false,
                       _("Failed to load either of rulesets '%s' or '%s' "
                         "needed for savegame."),
                       ruleset, alt_dir);
      } else {
        sg_failure_ret(false,
                       _("Failed to load ruleset '%s' needed for savegame."),
                       ruleset);
      }
    }
  } else {
    if (!load_rulesets(nullptr, nullptr, false, nullptr, true, false,
                       ruleset_datafile)) {
      // Failed to load correct ruleset
      sg_failure_ret(false, _("Failed to load ruleset '%s'."),
                     game.server.rulesetdir);
    }
  }

  /* Remove all aifill players. Correct number of them get created later
   * with correct skill level etc. */
  (void) aifill(0);

  if (game.scenario.is_scenario && !game.scenario.ruleset_locked) {
    const char *req_caps;

    req_caps = secfile_lookup_str_default(loading->file, "",
                                          "scenario.ruleset_caps");
    qstrncpy(game.scenario.req_caps, req_caps,
             sizeof(game.scenario.req_caps) - 1);
    game.scenario.req_caps[sizeof(game.scenario.req_caps) - 1] = '\0';

    if (!has_capabilities(req_caps, game.ruleset_capabilities)) {
      // Current ruleset lacks required capabilities.
      qInfo(_("Scenario requires ruleset capabilities: %s"), req_caps);
      qInfo(_("Ruleset has capabilities: %s"), game.ruleset_capabilities);
      qCritical(_("Current ruleset not compatible with the scenario."));
      sg_success = false;
      return;
    }
  }

  // Time to load scenario specific luadata
  if (game.scenario.datafile[0] != '\0') {
    if (!fc_strcasecmp("none", game.scenario.datafile)) {
      game.server.luadata = nullptr;
    } else {
      char testfile[MAX_LEN_PATH];
      struct section_file *secfile;

      fc_snprintf(testfile, sizeof(testfile), "%s.luadata",
                  game.scenario.datafile);

      auto found = fileinfoname(get_scenario_dirs(), testfile);

      if (found.isEmpty()) {
        qCritical(_("Can't find scenario luadata file %s.luadata."),
                  game.scenario.datafile);
        sg_success = false;
        return;
      }

      secfile = secfile_load(found, false);
      if (secfile == nullptr) {
        qCritical(_("Failed to load scenario luadata file %s.luadata"),
                  game.scenario.datafile);
        sg_success = false;
        return;
      }

      game.server.luadata = secfile;
    }
  }

  /* This is in the savegame only if the game has been started before
   * savegame3.c time, and in that case it's TRUE. If it's missing, it's to
   * be considered FALSE. */
  game.server.last_updated_year = secfile_lookup_bool_default(
      loading->file, false, "savefile.last_updated_as_year");

  // Load improvements.
  loading->improvement.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.improvement_size");
  if (loading->improvement.size) {
    loading->improvement.order =
        secfile_lookup_str_vec(loading->file, &loading->improvement.size,
                               "savefile.improvement_vector");
    sg_failure_ret(loading->improvement.size != 0,
                   "Failed to load improvement order: %s", secfile_error());
  }

  // Load technologies.
  loading->technology.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.technology_size");
  if (loading->technology.size) {
    loading->technology.order =
        secfile_lookup_str_vec(loading->file, &loading->technology.size,
                               "savefile.technology_vector");
    sg_failure_ret(loading->technology.size != 0,
                   "Failed to load technology order: %s", secfile_error());
  }

  // Load Activities.
  loading->activities.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.activities_size");
  if (loading->activities.size) {
    loading->activities.order =
        secfile_lookup_str_vec(loading->file, &loading->activities.size,
                               "savefile.activities_vector");
    sg_failure_ret(loading->activities.size != 0,
                   "Failed to load activity order: %s", secfile_error());
  }

  // Load traits.
  loading->trait.size =
      secfile_lookup_int_default(loading->file, 0, "savefile.trait_size");
  if (loading->trait.size) {
    loading->trait.order = secfile_lookup_str_vec(
        loading->file, &loading->trait.size, "savefile.trait_vector");
    sg_failure_ret(loading->trait.size != 0,
                   "Failed to load trait order: %s", secfile_error());
  }

  // Load extras.
  loading->extra.size =
      secfile_lookup_int_default(loading->file, 0, "savefile.extras_size");
  if (loading->extra.size) {
    const char **modname;
    size_t nmod;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->extra.size,
                                     "savefile.extras_vector");
    sg_failure_ret(loading->extra.size != 0,
                   "Failed to load extras order: %s", secfile_error());
    sg_failure_ret(!(game.control.num_extra_types < loading->extra.size),
                   "Number of extras defined by the ruleset (= %d) are "
                   "lower than the number in the savefile (= %d).",
                   game.control.num_extra_types, (int) loading->extra.size);
    // make sure that the size of the array is divisible by 4
    nmod = 4 * ((loading->extra.size + 3) / 4);
    loading->extra.order = new extra_type *[nmod]();
    for (j = 0; j < loading->extra.size; j++) {
      loading->extra.order[j] = extra_type_by_rule_name(modname[j]);
    }
    delete[] modname;
    for (; j < nmod; j++) {
      loading->extra.order[j] = nullptr;
    }
  }

  // Load multipliers.
  loading->multiplier.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.multipliers_size");
  if (loading->multiplier.size) {
    const char **modname;
    int j;

    modname =
        secfile_lookup_str_vec(loading->file, &loading->multiplier.size,
                               "savefile.multipliers_vector");
    sg_failure_ret(loading->multiplier.size != 0,
                   "Failed to load multipliers order: %s", secfile_error());
    /* It's OK for the set of multipliers in the savefile to differ
     * from those in the ruleset. */
    loading->multiplier.order = new multiplier *[loading->multiplier.size]();
    for (j = 0; j < loading->multiplier.size; j++) {
      loading->multiplier.order[j] = multiplier_by_rule_name(modname[j]);
      if (!loading->multiplier.order[j]) {
        qDebug("Multiplier \"%s\" in savegame but not in ruleset, "
               "discarding",
               modname[j]);
      }
    }
    delete[] modname;
  }

  // Load specialists.
  loading->specialist.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.specialists_size");
  if (loading->specialist.size) {
    const char **modname;
    size_t nmod;
    int j;

    modname =
        secfile_lookup_str_vec(loading->file, &loading->specialist.size,
                               "savefile.specialists_vector");
    sg_failure_ret(loading->specialist.size != 0,
                   "Failed to load specialists order: %s", secfile_error());
    sg_failure_ret(
        !(game.control.num_specialist_types < loading->specialist.size),
        "Number of specialists defined by the ruleset (= %d) are "
        "lower than the number in the savefile (= %d).",
        game.control.num_specialist_types, (int) loading->specialist.size);
    // make sure that the size of the array is divisible by 4
    /* That's not really needed with specialists at the moment, but done this
     * way for consistency with other types, and to be prepared for the time
     * it needs to be this way. */
    nmod = 4 * ((loading->specialist.size + 3) / 4);
    loading->specialist.order = new specialist *[nmod]();
    for (j = 0; j < loading->specialist.size; j++) {
      loading->specialist.order[j] = specialist_by_rule_name(modname[j]);
    }
    delete[] modname;
    for (; j < nmod; j++) {
      loading->specialist.order[j] = nullptr;
    }
  }

  // Load action order.
  loading->action.size =
      secfile_lookup_int_default(loading->file, 0, "savefile.action_size");

  sg_failure_ret(loading->action.size > 0, "Failed to load action order: %s",
                 secfile_error());

  if (loading->action.size) {
    const char **modname;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->action.size,
                                     "savefile.action_vector");

    loading->action.order = new action_id[loading->action.size]();

    for (j = 0; j < loading->action.size; j++) {
      struct action *real_action = action_by_rule_name(modname[j]);

      if (real_action) {
        loading->action.order[j] = real_action->id;
      } else {
        log_sg("Unknown action \'%s\'", modname[j]);
        loading->action.order[j] = ACTION_NONE;
      }
    }

    delete[] modname;
  }

  // Load action decision order.
  loading->act_dec.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.action_decision_size");

  sg_failure_ret(loading->act_dec.size > 0,
                 "Failed to load action decision order: %s",
                 secfile_error());

  if (loading->act_dec.size) {
    const char **modname;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->act_dec.size,
                                     "savefile.action_decision_vector");

    loading->act_dec.order = new action_decision[loading->act_dec.size]();

    for (j = 0; j < loading->act_dec.size; j++) {
      loading->act_dec.order[j] =
          action_decision_by_name(modname[j], fc_strcasecmp);
    }

    delete[] modname;
  }

  // Load server side agent order.
  loading->ssa.size = secfile_lookup_int_default(
      loading->file, 0, "savefile.server_side_agent_size");

  sg_failure_ret(loading->ssa.size > 0,
                 "Failed to load server side agent order: %s",
                 secfile_error());

  if (loading->ssa.size) {
    const char **modname;
    int j;

    modname = secfile_lookup_str_vec(loading->file, &loading->ssa.size,
                                     "savefile.server_side_agent_list");
    loading->ssa.order = new server_side_agent[loading->ssa.size]();

    for (j = 0; j < loading->ssa.size; j++) {
      loading->ssa.order[j] =
          server_side_agent_by_name(modname[j], fc_strcasecmp);
    }

    delete[] modname;
  }

  terrain_type_iterate(pterr) { pterr->identifier_load = '\0'; }
  terrain_type_iterate_end;

  i = 0;
  while ((terr_name = secfile_lookup_str_default(
              loading->file, nullptr, "savefile.terrident%d.name", i))
         != nullptr) {
    struct terrain *pterr = terrain_by_rule_name(terr_name);

    if (pterr != nullptr) {
      const char *iptr = secfile_lookup_str_default(
          loading->file, nullptr, "savefile.terrident%d.identifier", i);

      pterr->identifier_load = *iptr;
    } else {
      qCritical("Identifier for unknown terrain type %s.", terr_name);
    }
    i++;
  }

  terrain_type_iterate(pterr)
  {
    terrain_type_iterate(pterr2)
    {
      if (pterr != pterr2 && pterr->identifier_load != '\0') {
        sg_failure_ret((pterr->identifier_load != pterr2->identifier_load),
                       "%s and %s share a saved identifier",
                       terrain_rule_name(pterr), terrain_rule_name(pterr2));
      }
    }
    terrain_type_iterate_end;
  }
  terrain_type_iterate_end;
}

/**
   Save '[savefile]'.
 */
static void sg_save_savefile(struct savedata *saving)
{
  int i;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Save savefile options.
  sg_save_savefile_options(saving, savefile_options_default);

  secfile_insert_int(saving->file, current_compat_ver(), "savefile.version");

  // Save reason of the savefile generation.
  secfile_insert_str(saving->file, saving->save_reason, "savefile.reason");

  // Save as accurate freeciv revision information as possible
  secfile_insert_str(saving->file, freeciv_datafile_version(),
                     "savefile.revision");

  /* Save rulesetdir at this point as this ruleset is required by this
   * savefile. */
  secfile_insert_str(saving->file, game.server.rulesetdir,
                     "savefile.rulesetdir");

  if (game.control.version[0] != '\0') {
    /* Current ruleset has version information, save it.
     * This is never loaded, but exist in savegame file only for debugging
     * purposes. */
    secfile_insert_str(saving->file, game.control.version,
                       "savefile.rulesetversion");
  }

  if (game.control.alt_dir[0] != '\0') {
    secfile_insert_str(saving->file, game.control.alt_dir,
                       "savefile.ruleset_alt_dir");
  }

  if (game.server.last_updated_year) {
    secfile_insert_bool(saving->file, true, "savefile.last_updated_as_year");
  }

  /* Save improvement order in savegame, so we are not dependent on ruleset
   * order. If the game isn't started improvements aren't loaded so we can
   * not save the order. */
  secfile_insert_int(saving->file, improvement_count(),
                     "savefile.improvement_size");
  if (improvement_count() > 0) {
    const char *buf[improvement_count()];

    improvement_iterate(pimprove)
    {
      buf[improvement_index(pimprove)] = improvement_rule_name(pimprove);
    }
    improvement_iterate_end;

    secfile_insert_str_vec(saving->file, buf, improvement_count(),
                           "savefile.improvement_vector");
  }

  /* Save technology order in savegame, so we are not dependent on ruleset
   * order. If the game isn't started advances aren't loaded so we can not
   * save the order. */
  secfile_insert_int(saving->file, game.control.num_tech_types,
                     "savefile.technology_size");
  if (game.control.num_tech_types > 0) {
    const char *buf[game.control.num_tech_types];

    buf[A_NONE] = "A_NONE";
    advance_iterate(A_FIRST, a)
    {
      buf[advance_index(a)] = advance_rule_name(a);
    }
    advance_iterate_end;
    secfile_insert_str_vec(saving->file, buf, game.control.num_tech_types,
                           "savefile.technology_vector");
  }

  // Save activities order in the savegame.
  secfile_insert_int(saving->file, ACTIVITY_LAST,
                     "savefile.activities_size");
  if (ACTIVITY_LAST > 0) {
    const char **modname;
    int j;

    i = 0;

    modname = new const char *[ACTIVITY_LAST]();

    for (j = 0; j < ACTIVITY_LAST; j++) {
      modname[i++] = unit_activity_name(static_cast<unit_activity>(j));
    }

    secfile_insert_str_vec(saving->file, modname, ACTIVITY_LAST,
                           "savefile.activities_vector");
    delete[] modname;
  }

  // Save specialists order in the savegame.
  secfile_insert_int(saving->file, specialist_count(),
                     "savefile.specialists_size");
  {
    const char **modname;
    i = 0;
    modname = new const char *[specialist_count()]();

    specialist_type_iterate(sp)
    {
      modname[i++] = specialist_rule_name(specialist_by_number(sp));
    }
    specialist_type_iterate_end;

    secfile_insert_str_vec(saving->file, modname, specialist_count(),
                           "savefile.specialists_vector");

    delete[] modname;
  }

  // Save trait order in savegame.
  secfile_insert_int(saving->file, TRAIT_COUNT, "savefile.trait_size");
  {
    const char **modname;
    enum trait tr;
    int j;

    modname = new const char *[TRAIT_COUNT]();

    for (tr = trait_begin(), j = 0; tr != trait_end();
         tr = trait_next(tr), j++) {
      modname[j] = trait_name(tr);
    }

    secfile_insert_str_vec(saving->file, modname, TRAIT_COUNT,
                           "savefile.trait_vector");
    delete[] modname;
  }

  // Save extras order in the savegame.
  secfile_insert_int(saving->file, game.control.num_extra_types,
                     "savefile.extras_size");
  if (game.control.num_extra_types > 0) {
    const char **modname;
    i = 0;
    modname = new const char *[game.control.num_extra_types]();

    extra_type_iterate(pextra) { modname[i++] = extra_rule_name(pextra); }
    extra_type_iterate_end;

    secfile_insert_str_vec(saving->file, modname,
                           game.control.num_extra_types,
                           "savefile.extras_vector");
    delete[] modname;
  }

  // Save multipliers order in the savegame.
  secfile_insert_int(saving->file, multiplier_count(),
                     "savefile.multipliers_size");
  if (multiplier_count() > 0) {
    const char **modname;
    i = 0;
    modname = new const char *[multiplier_count()]();

    multipliers_iterate(pmul)
    {
      modname[multiplier_index(pmul)] = multiplier_rule_name(pmul);
    }
    multipliers_iterate_end;

    secfile_insert_str_vec(saving->file, modname, multiplier_count(),
                           "savefile.multipliers_vector");
    delete[] modname;
  }

  // Save diplstate type order in the savegame.
  secfile_insert_int(saving->file, DS_LAST, "savefile.diplstate_type_size");
  if (DS_LAST > 0) {
    const char **modname;
    int j;

    i = 0;
    modname = new const char *[DS_LAST]();

    for (j = 0; j < DS_LAST; j++) {
      modname[i++] = diplstate_type_name(static_cast<diplstate_type>(j));
    }

    secfile_insert_str_vec(saving->file, modname, DS_LAST,
                           "savefile.diplstate_type_vector");
    delete[] modname;
  }

  // Save city_option order in the savegame.
  secfile_insert_int(saving->file, CITYO_LAST, "savefile.city_options_size");
  if (CITYO_LAST > 0) {
    const char **modname;
    int j;

    i = 0;
    modname = new const char *[CITYO_LAST]();

    for (j = 0; j < CITYO_LAST; j++) {
      modname[i++] = city_options_name(static_cast<city_options>(j));
    }

    secfile_insert_str_vec(saving->file, modname, CITYO_LAST,
                           "savefile.city_options_vector");
    delete[] modname;
  }

  // Save action order in the savegame.
  secfile_insert_int(saving->file, NUM_ACTIONS, "savefile.action_size");
  if (NUM_ACTIONS > 0) {
    const char **modname;
    int j;

    i = 0;
    modname = new const char *[NUM_ACTIONS]();

    for (j = 0; j < NUM_ACTIONS; j++) {
      modname[i++] = action_id_rule_name(j);
    }

    secfile_insert_str_vec(saving->file, modname, NUM_ACTIONS,
                           "savefile.action_vector");
    delete[] modname;
  }

  // Save action decision order in the savegame.
  secfile_insert_int(saving->file, ACT_DEC_COUNT,
                     "savefile.action_decision_size");
  if (ACT_DEC_COUNT > 0) {
    const char **modname;
    int j;

    i = 0;
    modname = new const char *[ACT_DEC_COUNT]();

    for (j = 0; j < ACT_DEC_COUNT; j++) {
      modname[i++] = action_decision_name(static_cast<action_decision>(j));
    }

    secfile_insert_str_vec(saving->file, modname, ACT_DEC_COUNT,
                           "savefile.action_decision_vector");
    delete[] modname;
  }

  // Save server side agent order in the savegame.
  secfile_insert_int(saving->file, SSA_COUNT,
                     "savefile.server_side_agent_size");
  if (SSA_COUNT > 0) {
    const char **modname;
    int j;

    i = 0;
    modname = new const char *[SSA_COUNT]();

    for (j = 0; j < SSA_COUNT; j++) {
      modname[i++] =
          server_side_agent_name(static_cast<server_side_agent>(j));
    }

    secfile_insert_str_vec(saving->file, modname, SSA_COUNT,
                           "savefile.server_side_agent_list");
    delete[] modname;
  }

  // Save terrain character mapping in the savegame.
  i = 0;
  terrain_type_iterate(pterr)
  {
    char buf[2];

    secfile_insert_str(saving->file, terrain_rule_name(pterr),
                       "savefile.terrident%d.name", i);
    buf[0] = terrain_identifier(pterr);
    buf[1] = '\0';
    secfile_insert_str(saving->file, buf, "savefile.terrident%d.identifier",
                       i++);
  }
  terrain_type_iterate_end;
}

/**
   Save options for this savegame. sg_load_savefile_options() is not defined.
 */
static void sg_save_savefile_options(struct savedata *saving,
                                     const char *option)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (option == nullptr) {
    // no additional option
    return;
  }

  sz_strlcat(saving->secfile_options, option);
  secfile_replace_str(saving->file, saving->secfile_options,
                      "savefile.options");
}

/* =======================================================================
 * Load / save game status.
 * ======================================================================= */

/**
   Load '[ruledata]'.
 */
static void sg_load_ruledata(struct loaddata *loading)
{
  int i;
  const char *name;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  for (i = 0; (name = secfile_lookup_str_default(
                   loading->file, nullptr, "ruledata.government%d.name", i));
       i++) {
    struct government *gov = government_by_rule_name(name);

    if (gov != nullptr) {
      gov->changed_to_times = secfile_lookup_int_default(
          loading->file, 0, "ruledata.government%d.changes", i);
    }
  }
}

/**
   Load '[game]'.
 */
static void sg_load_game(struct loaddata *loading)
{
  const char *str;
  const char *level;
  int i;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Load server state.
  str = secfile_lookup_str_default(loading->file, "S_S_INITIAL",
                                   "game.server_state");
  loading->server_state = server_states_by_name(str, strcmp);
  if (!server_states_is_valid(loading->server_state)) {
    // Don't take any risk!
    loading->server_state = S_S_INITIAL;
  }

  game.server.additional_phase_seconds =
      secfile_lookup_int_default(loading->file, 0, "game.phase_seconds");

  str = secfile_lookup_str_default(
      loading->file, default_meta_patches_string(), "game.meta_patches");
  set_meta_patches_string(str);

  if (srvarg.metaserver_addr == QLatin1String(DEFAULT_META_SERVER_ADDR)) {
    /* Do not overwrite this if the user requested a specific metaserver
     * from the command line (option --Metaserver). */
    srvarg.metaserver_addr = QString::fromUtf8(secfile_lookup_str_default(
        loading->file, DEFAULT_META_SERVER_ADDR, "game.meta_server"));
  }

  if (srvarg.serverid.isEmpty()) {
    /* Do not overwrite this if the user requested a specific metaserver
     * from the command line (option --serverid). */
    srvarg.serverid = QString::fromUtf8(
        secfile_lookup_str_default(loading->file, "", "game.serverid"));
  }
  sz_strlcpy(server.game_identifier,
             secfile_lookup_str_default(loading->file, "", "game.id"));
  /* We are not checking game_identifier legality just yet.
   * That's done when we are sure that rand seed has been initialized,
   * so that we can generate new game_identifier, if needed.
   * See sq_load_sanitycheck(). */

  level = secfile_lookup_str_default(loading->file, nullptr, "game.level");
  if (level != nullptr) {
    game.info.skill_level = ai_level_by_name(level, fc_strcasecmp);
  } else {
    game.info.skill_level = ai_level_invalid();
  }

  if (!ai_level_is_valid(static_cast<ai_level>(game.info.skill_level))) {
    game.info.skill_level =
        ai_level_convert(GAME_HARDCODED_DEFAULT_SKILL_LEVEL);
  }
  str =
      secfile_lookup_str_default(loading->file, nullptr, "game.phase_mode");
  if (str != nullptr) {
    game.info.phase_mode = phase_mode_type_by_name(str, fc_strcasecmp);
    if (!phase_mode_type_is_valid(game.info.phase_mode)) {
      qCritical("Illegal phase mode \"%s\"", str);
      game.info.phase_mode =
          static_cast<phase_mode_type>(GAME_DEFAULT_PHASE_MODE);
    }
  } else {
    qCritical("Phase mode missing");
  }

  str = secfile_lookup_str_default(loading->file, nullptr,
                                   "game.phase_mode_stored");
  if (str != nullptr) {
    game.server.phase_mode_stored =
        phase_mode_type_by_name(str, fc_strcasecmp);
    if (!phase_mode_type_is_valid(
            static_cast<phase_mode_type>(game.server.phase_mode_stored))) {
      qCritical("Illegal stored phase mode \"%s\"", str);
      game.server.phase_mode_stored = GAME_DEFAULT_PHASE_MODE;
    }
  } else {
    qCritical("Stored phase mode missing");
  }
  game.info.phase =
      secfile_lookup_int_default(loading->file, 0, "game.phase");
  game.server.scoreturn = secfile_lookup_int_default(
      loading->file, game.info.turn + GAME_DEFAULT_SCORETURN,
      "game.scoreturn");

  game.server.timeoutint = secfile_lookup_int_default(
      loading->file, GAME_DEFAULT_TIMEOUTINT, "game.timeoutint");
  game.server.timeoutintinc = secfile_lookup_int_default(
      loading->file, GAME_DEFAULT_TIMEOUTINTINC, "game.timeoutintinc");
  game.server.timeoutinc = secfile_lookup_int_default(
      loading->file, GAME_DEFAULT_TIMEOUTINC, "game.timeoutinc");
  game.server.timeoutincmult = secfile_lookup_int_default(
      loading->file, GAME_DEFAULT_TIMEOUTINCMULT, "game.timeoutincmult");
  game.server.timeoutcounter = secfile_lookup_int_default(
      loading->file, GAME_DEFAULT_TIMEOUTCOUNTER, "game.timeoutcounter");

  game.info.turn = secfile_lookup_int_default(loading->file, 0, "game.turn");
  sg_failure_ret(
      secfile_lookup_int(loading->file, &game.info.year, "game.year"), "%s",
      secfile_error());
  game.info.year_0_hack =
      secfile_lookup_bool_default(loading->file, false, "game.year_0_hack");

  game.info.globalwarming =
      secfile_lookup_int_default(loading->file, 0, "game.globalwarming");
  game.info.heating =
      secfile_lookup_int_default(loading->file, 0, "game.heating");
  game.info.warminglevel =
      secfile_lookup_int_default(loading->file, 0, "game.warminglevel");

  game.info.nuclearwinter =
      secfile_lookup_int_default(loading->file, 0, "game.nuclearwinter");
  game.info.cooling =
      secfile_lookup_int_default(loading->file, 0, "game.cooling");
  game.info.coolinglevel =
      secfile_lookup_int_default(loading->file, 0, "game.coolinglevel");

  // Global advances.
  str = secfile_lookup_str_default(loading->file, nullptr,
                                   "game.global_advances");
  if (str != nullptr) {
    sg_failure_ret(strlen(str) == loading->technology.size,
                   "Invalid length of 'game.global_advances' (%lu ~= %lu).",
                   (unsigned long) qstrlen(str),
                   (unsigned long) loading->technology.size);
    for (i = 0; i < loading->technology.size; i++) {
      sg_failure_ret(str[i] == '1' || str[i] == '0',
                     "Undefined value '%c' within 'game.global_advances'.",
                     str[i]);
      if (str[i] == '1') {
        struct advance *padvance =
            advance_by_rule_name(loading->technology.order[i]);

        if (padvance != nullptr) {
          game.info.global_advances[advance_number(padvance)] = true;
        }
      }
    }
  }

  game.info.is_new_game =
      !secfile_lookup_bool_default(loading->file, true, "game.save_players");

  game.server.turn_change_time =
      secfile_lookup_int_default(loading->file, 0,
                                 "game.last_turn_change_time")
      / 100;
}

/**
   Save '[ruledata]'.
 */
static void sg_save_ruledata(struct savedata *saving)
{
  int set_count = 0;

  for (auto &pgov : governments) {
    char path[256];

    fc_snprintf(path, sizeof(path), "ruledata.government%d", set_count++);

    secfile_insert_str(saving->file, government_rule_name(&pgov), "%s.name",
                       path);
    secfile_insert_int(saving->file, pgov.changed_to_times, "%s.changes",
                       path);
  };
}

/**
   Save '[game]'.
 */
static void sg_save_game(struct savedata *saving)
{
  enum server_states srv_state;
  char global_advances[game.control.num_tech_types + 1];
  int i;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  /* Game state: once the game is no longer a new game (ie, has been
   * started the first time), it should always be considered a running
   * game for savegame purposes. */
  if (saving->scenario && !game.scenario.players) {
    srv_state = S_S_INITIAL;
  } else {
    srv_state = game.info.is_new_game ? server_state() : S_S_RUNNING;
  }
  secfile_insert_str(saving->file, server_states_name(srv_state),
                     "game.server_state");

  if (game.server.phase_timer != nullptr) {
    secfile_insert_int(saving->file,
                       timer_read_seconds(game.server.phase_timer)
                           + game.server.additional_phase_seconds,
                       "game.phase_seconds");
  }

  secfile_insert_str(saving->file, get_meta_patches_string(),
                     "game.meta_patches");
  secfile_insert_str(saving->file, qUtf8Printable(meta_addr_port()),
                     "game.meta_server");

  secfile_insert_str(saving->file, server.game_identifier, "game.id");
  secfile_insert_str(saving->file, qUtf8Printable(srvarg.serverid),
                     "game.serverid");

  secfile_insert_str(
      saving->file,
      ai_level_name(static_cast<ai_level>(game.info.skill_level)),
      "game.level");
  secfile_insert_str(saving->file,
                     phase_mode_type_name(game.info.phase_mode),
                     "game.phase_mode");
  secfile_insert_str(saving->file,
                     phase_mode_type_name(static_cast<phase_mode_type>(
                         game.server.phase_mode_stored)),
                     "game.phase_mode_stored");
  secfile_insert_int(saving->file, game.info.phase, "game.phase");
  secfile_insert_int(saving->file, game.server.scoreturn, "game.scoreturn");

  secfile_insert_int(saving->file, game.server.timeoutint,
                     "game.timeoutint");
  secfile_insert_int(saving->file, game.server.timeoutintinc,
                     "game.timeoutintinc");
  secfile_insert_int(saving->file, game.server.timeoutinc,
                     "game.timeoutinc");
  secfile_insert_int(saving->file, game.server.timeoutincmult,
                     "game.timeoutincmult");
  secfile_insert_int(saving->file, game.server.timeoutcounter,
                     "game.timeoutcounter");

  secfile_insert_int(saving->file, game.info.turn, "game.turn");
  secfile_insert_int(saving->file, game.info.year, "game.year");
  secfile_insert_bool(saving->file, game.info.year_0_hack,
                      "game.year_0_hack");

  secfile_insert_int(saving->file, game.info.globalwarming,
                     "game.globalwarming");
  secfile_insert_int(saving->file, game.info.heating, "game.heating");
  secfile_insert_int(saving->file, game.info.warminglevel,
                     "game.warminglevel");

  secfile_insert_int(saving->file, game.info.nuclearwinter,
                     "game.nuclearwinter");
  secfile_insert_int(saving->file, game.info.cooling, "game.cooling");
  secfile_insert_int(saving->file, game.info.coolinglevel,
                     "game.coolinglevel");
  /* For debugging purposes only.
   * Do not save it if it's 0 (not known);
   * this confuses people reading this 'document' less than
   * saving 0. */
  if (game.server.seed != 0) {
    secfile_insert_int(saving->file, game.server.seed, "game.random_seed");
  }

  // Global advances.
  for (i = 0; i < game.control.num_tech_types; i++) {
    global_advances[i] = game.info.global_advances[i] ? '1' : '0';
  }
  global_advances[i] = '\0';
  secfile_insert_str(saving->file, global_advances, "game.global_advances");

  if (!game_was_started()) {
    saving->save_players = false;
  } else {
    if (saving->scenario) {
      saving->save_players = game.scenario.players;
    } else {
      saving->save_players = true;
    }
#ifndef SAVE_DUMMY_TURN_CHANGE_TIME
    secfile_insert_int(saving->file, game.server.turn_change_time * 100,
                       "game.last_turn_change_time");
#else  // SAVE_DUMMY_TURN_CHANGE_TIME
    secfile_insert_int(saving->file, game.info.turn * 10,
                       "game.last_turn_change_time");
#endif // SAVE_DUMMY_TURN_CHANGE_TIME
  }
  secfile_insert_bool(saving->file, saving->save_players,
                      "game.save_players");

  if (srv_state != S_S_INITIAL) {
    const char *ainames[ai_type_get_count()];

    i = 0;
    ai_type_iterate(ait)
    {
      ainames[i] = ait->name;
      i++;
    }
    ai_type_iterate_end;

    secfile_insert_str_vec(saving->file, ainames, i, "game.ai_types");
  }
}

/* =======================================================================
 * Load / save random status.
 * ======================================================================= */

/**
   Load '[random]'.
 */
static void sg_load_random(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (secfile_lookup_bool_default(loading->file, false, "random.saved")) {
    if (secfile_lookup_int(loading->file, nullptr, "random.index_J")) {
      qWarning().noquote() << QString::fromUtf8(
          _("Cannot load old random generator state. Seeding a new one."));
      fc_rand_seed(loading->rstate);
    } else if (auto *state =
                   secfile_lookup_str(loading->file, "random.state")) {
      std::stringstream ss;
      ss.imbue(std::locale::classic());
      ss.str(state);
      ss >> loading->rstate;
      if (ss.fail()) {
        qWarning().noquote() << QString::fromUtf8(
            _("Cannot load the random generator state. Seeding a new one."));
        fc_rand_seed(loading->rstate);
      }
    }
    fc_rand_set_state(loading->rstate);
  } else {
    // No random values - mark the setting.
    (void) secfile_entry_by_path(loading->file, "random.saved");

    /* We're loading a game without a seed (which is okay, if it's a
     * scenario). We need to generate the game seed now because it will be
     * needed later during the load. */
    init_game_seed();
    loading->rstate = fc_rand_state();
  }
}

/**
   Save '[random]'.
 */
static void sg_save_random(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (!saving->scenario || game.scenario.save_random) {
    auto rstate = fc_rand_state();

    std::stringstream ss;
    ss.imbue(std::locale::classic());
    ss << rstate;
    auto state = ss.str();

    secfile_insert_bool(saving->file, true, "random.saved");
    secfile_insert_str(saving->file, state.data(), "random.state");
  } else {
    secfile_insert_bool(saving->file, false, "random.saved");
  }
}

/* =======================================================================
 * Load / save lua script data.
 * ======================================================================= */

/**
   Load '[script]'.
 */
static void sg_load_script(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  script_server_state_load(loading->file);
}

/**
   Save '[script]'.
 */
static void sg_save_script(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  script_server_state_save(saving->file);
}

/* =======================================================================
 * Load / save scenario data.
 * ======================================================================= */

/**
   Load '[scenario]'.
 */
static void sg_load_scenario(struct loaddata *loading)
{
  const char *buf;
  int game_version;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Load version.
  game_version =
      secfile_lookup_int_default(loading->file, 0, "scenario.game_version");
  /* We require at least version 2.90.99 - and at that time we saved version
   * numbers as 10000*MAJOR+100*MINOR+PATCH */
  sg_failure_ret(29099 <= game_version, "Saved game is too old, at least "
                                        "version 2.90.99 required.");

  game.scenario.datafile[0] = '\0';

  sg_failure_ret(secfile_lookup_bool(loading->file,
                                     &game.scenario.is_scenario,
                                     "scenario.is_scenario"),
                 "%s", secfile_error());
  if (!game.scenario.is_scenario) {
    return;
  }

  buf = secfile_lookup_str_default(loading->file, "", "scenario.name");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario.name, buf);
  }

  buf = secfile_lookup_str_default(loading->file, "", "scenario.authors");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario.authors, buf);
  } else {
    game.scenario.authors[0] = '\0';
  }

  buf =
      secfile_lookup_str_default(loading->file, "", "scenario.description");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario_desc.description, buf);
  } else {
    game.scenario_desc.description[0] = '\0';
  }
  game.scenario.save_random = secfile_lookup_bool_default(
      loading->file, false, "scenario.save_random");
  game.scenario.players =
      secfile_lookup_bool_default(loading->file, true, "scenario.players");
  game.scenario.startpos_nations = secfile_lookup_bool_default(
      loading->file, false, "scenario.startpos_nations");
  game.scenario.prevent_new_cities = secfile_lookup_bool_default(
      loading->file, false, "scenario.prevent_new_cities");
  game.scenario.lake_flooding = secfile_lookup_bool_default(
      loading->file, true, "scenario.lake_flooding");
  game.scenario.handmade =
      secfile_lookup_bool_default(loading->file, false, "scenario.handmade");
  game.scenario.allow_ai_type_fallback = secfile_lookup_bool_default(
      loading->file, false, "scenario.allow_ai_type_fallback");

  game.scenario.ruleset_locked = secfile_lookup_bool_default(
      loading->file, true, "scenario.ruleset_locked");

  buf = secfile_lookup_str_default(loading->file, "", "scenario.datafile");
  if (buf[0] != '\0') {
    sz_strlcpy(game.scenario.datafile, buf);
  }

  sg_failure_ret(loading->server_state == S_S_INITIAL
                     || (loading->server_state == S_S_RUNNING
                         && game.scenario.players == true),
                 "Invalid scenario definition (server state '%s' and "
                 "players are %s).",
                 server_states_name(loading->server_state),
                 game.scenario.players ? "saved" : "not saved");
}

/**
   Save '[scenario]'.
 */
static void sg_save_scenario(struct savedata *saving)
{
  struct entry *mod_entry;
  int game_version;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  game_version =
      MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000 + PATCH_VERSION * 100;
  game_version += EMERGENCY_VERSION;
  secfile_insert_int(saving->file, game_version, "scenario.game_version");

  if (!saving->scenario || !game.scenario.is_scenario) {
    secfile_insert_bool(saving->file, false, "scenario.is_scenario");
    return;
  }

  secfile_insert_bool(saving->file, true, "scenario.is_scenario");

  // Name is mandatory to the level that is saved even if empty.
  mod_entry =
      secfile_insert_str(saving->file, game.scenario.name, "scenario.name");
  entry_str_set_gt_marking(mod_entry, true);

  // Authors list is saved only if it exist
  if (game.scenario.authors[0] != '\0') {
    mod_entry = secfile_insert_str(saving->file, game.scenario.authors,
                                   "scenario.authors");
    entry_str_set_gt_marking(mod_entry, true);
  }

  // Description is saved only if it exist
  if (game.scenario_desc.description[0] != '\0') {
    mod_entry =
        secfile_insert_str(saving->file, game.scenario_desc.description,
                           "scenario.description");
    entry_str_set_gt_marking(mod_entry, true);
  }

  secfile_insert_bool(saving->file, game.scenario.save_random,
                      "scenario.save_random");
  secfile_insert_bool(saving->file, game.scenario.players,
                      "scenario.players");
  secfile_insert_bool(saving->file, game.scenario.startpos_nations,
                      "scenario.startpos_nations");
  if (game.scenario.prevent_new_cities) {
    secfile_insert_bool(saving->file, game.scenario.prevent_new_cities,
                        "scenario.prevent_new_cities");
  }
  secfile_insert_bool(saving->file, game.scenario.lake_flooding,
                      "scenario.lake_flooding");
  if (game.scenario.handmade) {
    secfile_insert_bool(saving->file, game.scenario.handmade,
                        "scenario.handmade");
  }
  if (game.scenario.allow_ai_type_fallback) {
    secfile_insert_bool(saving->file, game.scenario.allow_ai_type_fallback,
                        "scenario.allow_ai_type_fallback");
  }

  if (game.scenario.datafile[0] != '\0') {
    secfile_insert_str(saving->file, game.scenario.datafile,
                       "scenario.datafile");
  }
  secfile_insert_bool(saving->file, game.scenario.ruleset_locked,
                      "scenario.ruleset_locked");
  if (!game.scenario.ruleset_locked && game.scenario.req_caps[0] != '\0') {
    secfile_insert_str(saving->file, game.scenario.req_caps,
                       "scenario.ruleset_caps");
  }
}

/* =======================================================================
 * Load / save game settings.
 * ======================================================================= */

/**
   Load '[settings]'.
 */
static void sg_load_settings(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  settings_game_load(loading->file, "settings");

  // Save current status of fogofwar.
  game.server.fogofwar_old = game.info.fogofwar;

  // Add all compatibility settings here.
}

/**
   Save [settings].
 */
static void sg_save_settings(struct savedata *saving)
{
  enum map_generator real_generator = wld.map.server.generator;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (saving->scenario) {
    wld.map.server.generator = MAPGEN_SCENARIO; // We want a scenario.
  }

  settings_game_save(saving->file, "settings");
  // Restore real map generator.
  wld.map.server.generator = real_generator;

  // Add all compatibility settings here.
}

/* =======================================================================
 * Load / save the main map.
 * ======================================================================= */

/**
   Load '[map'].
 */
static void sg_load_map(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  /* This defaults to TRUE even if map has not been generated.
   * We rely on that
   *   1) scenario maps have it explicitly right.
   *   2) when map is actually generated, it re-initialize this to FALSE. */
  wld.map.server.have_huts =
      secfile_lookup_bool_default(loading->file, true, "map.have_huts");
  game.scenario.have_resources =
      secfile_lookup_bool_default(loading->file, true, "map.have_resources");

  wld.map.server.have_resources = game.scenario.have_resources;

  if (S_S_INITIAL == loading->server_state
      && MAPGEN_SCENARIO == wld.map.server.generator) {
    /* Generator MAPGEN_SCENARIO is used;
     * this map was done with the map editor. */

    // Load tiles.
    sg_load_map_tiles(loading);
    sg_load_map_startpos(loading);
    sg_load_map_tiles_extras(loading);

    // Nothing more needed for a scenario.
    return;
  }

  if (S_S_INITIAL == loading->server_state) {
    // Nothing more to do if it is not a scenario but in initial state.
    return;
  }

  sg_load_map_tiles(loading);
  sg_load_map_startpos(loading);
  sg_load_map_tiles_extras(loading);
  sg_load_map_known(loading);
  sg_load_map_owner(loading);
  sg_load_map_worked(loading);
}

/**
   Save 'map'.
 */
static void sg_save_map(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (map_is_empty()) {
    // No map.
    return;
  }

  if (saving->scenario) {
    secfile_insert_bool(saving->file, wld.map.server.have_huts,
                        "map.have_huts");
    secfile_insert_bool(saving->file, game.scenario.have_resources,
                        "map.have_resources");
  } else {
    secfile_insert_bool(saving->file, true, "map.have_huts");
    secfile_insert_bool(saving->file, true, "map.have_resources");
  }

  /* For debugging purposes only.
   * Do not save it if it's 0 (not known);
   * this confuses people reading this 'document' less than
   * saving 0. */
  if (wld.map.server.seed) {
    secfile_insert_int(saving->file, wld.map.server.seed, "map.random_seed");
  }

  sg_save_map_tiles(saving);
  sg_save_map_startpos(saving);
  sg_save_map_tiles_extras(saving);
  sg_save_map_owner(saving);
  sg_save_map_worked(saving);
  sg_save_map_known(saving);
}

/**
   Load tiles of the map.
 */
static void sg_load_map_tiles(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  /* Initialize the map for the current topology. 'map.xsize' and
   * 'map.ysize' must be set. */
  map_init_topology();

  // Allocate map.
  main_map_allocate();

  // get the terrain type
  LOAD_MAP_CHAR(ch, ptile, ptile->terrain = char2terrain(ch), loading->file,
                "map.t%04d");
  assign_continent_numbers();

  // Check for special tile sprites.
  whole_map_iterate(&(wld.map), ptile)
  {
    const char *spec_sprite;
    const char *label;
    int nat_x, nat_y;

    index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
    spec_sprite = secfile_lookup_str(loading->file, "map.spec_sprite_%d_%d",
                                     nat_x, nat_y);
    label = secfile_lookup_str_default(loading->file, nullptr,
                                       "map.label_%d_%d", nat_x, nat_y);
    if (nullptr != ptile->spec_sprite) {
      ptile->spec_sprite = fc_strdup(spec_sprite);
    }
    if (label != nullptr) {
      tile_set_label(ptile, label);
    }
  }
  whole_map_iterate_end;
}

/**
   Save all map tiles
 */
static void sg_save_map_tiles(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Save the terrain type.
  SAVE_MAP_CHAR(ptile, terrain2char(ptile->terrain), saving->file,
                "map.t%04d");

  // Save special tile sprites.
  whole_map_iterate(&(wld.map), ptile)
  {
    int nat_x, nat_y;

    index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
    if (ptile->spec_sprite) {
      secfile_insert_str(saving->file, ptile->spec_sprite,
                         "map.spec_sprite_%d_%d", nat_x, nat_y);
    }
    if (ptile->label != nullptr) {
      secfile_insert_str(saving->file, ptile->label, "map.label_%d_%d",
                         nat_x, nat_y);
    }
  }
  whole_map_iterate_end;
}

/**
   Load extras to map
 */
static void sg_load_map_tiles_extras(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Load extras.
  halfbyte_iterate_extras(j, loading->extra.size)
  {
    LOAD_MAP_CHAR(
        ch, ptile,
        sg_extras_set(&ptile->extras, ch, loading->extra.order + 4 * j),
        loading->file, "map.e%02d_%04d", j);
  }
  halfbyte_iterate_extras_end;

  if (S_S_INITIAL != loading->server_state
      || MAPGEN_SCENARIO != wld.map.server.generator
      || game.scenario.have_resources) {
    whole_map_iterate(&(wld.map), ptile)
    {
      extra_type_by_cause_iterate(EC_RESOURCE, pres)
      {
        if (tile_has_extra(ptile, pres)) {
          tile_set_resource(ptile, pres);

          if (!terrain_has_resource(ptile->terrain, ptile->resource)) {
            BV_CLR(ptile->extras, extra_index(pres));
          }
        }
      }
      extra_type_by_cause_iterate_end;
    }
    whole_map_iterate_end;
  }
}

/**
   Save information about extras on map
 */
static void sg_save_map_tiles_extras(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Save extras.
  halfbyte_iterate_extras(j, game.control.num_extra_types)
  {
    int mod[4];
    int l;

    for (l = 0; l < 4; l++) {
      if (4 * j + 1 > game.control.num_extra_types) {
        mod[l] = -1;
      } else {
        mod[l] = 4 * j + l;
      }
    }
    SAVE_MAP_CHAR(ptile, sg_extras_get(ptile->extras, ptile->resource, mod),
                  saving->file, "map.e%02d_%04d", j);
  }
  halfbyte_iterate_extras_end;
}

/**
   Load starting positions for the players from a savegame file. There should
   be at least enough for every player.
 */
static void sg_load_map_startpos(struct loaddata *loading)
{
  struct nation_type *pnation;
  struct startpos *psp;
  struct tile *ptile;
  const char SEPARATOR = '#';
  const char *nation_names;
  int nat_x, nat_y;
  bool exclude;
  int i, startpos_count;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  startpos_count =
      secfile_lookup_int_default(loading->file, 0, "map.startpos_count");

  if (0 == startpos_count) {
    // Nothing to do.
    return;
  }

  for (i = 0; i < startpos_count; i++) {
    if (!secfile_lookup_int(loading->file, &nat_x, "map.startpos%d.x", i)
        || !secfile_lookup_int(loading->file, &nat_y, "map.startpos%d.y",
                               i)) {
      log_sg("Warning: Undefined coordinates for startpos %d", i);
      continue;
    }

    ptile = native_pos_to_tile(&(wld.map), nat_x, nat_y);
    if (nullptr == ptile) {
      qCritical("Start position native coordinates (%d, %d) do not exist "
                "in this map. Skipping...",
                nat_x, nat_y);
      continue;
    }

    exclude = secfile_lookup_bool_default(loading->file, false,
                                          "map.startpos%d.exclude", i);

    psp = map_startpos_new(ptile);

    nation_names =
        secfile_lookup_str(loading->file, "map.startpos%d.nations", i);
    if (nullptr != nation_names && '\0' != nation_names[0]) {
      const size_t size = qstrlen(nation_names) + 1;
      char buf[size], *start, *end;

      memcpy(buf, nation_names, size);
      for (start = buf - 1; nullptr != start; start = end) {
        start++;
        if ((end = strchr(start, SEPARATOR))) {
          *end = '\0';
        }

        pnation = nation_by_rule_name(start);
        if (NO_NATION_SELECTED != pnation) {
          if (exclude) {
            startpos_disallow(psp, pnation);
          } else {
            startpos_allow(psp, pnation);
          }
        } else {
          qDebug("Missing nation \"%s\".", start);
        }
      }
    }
  }

  if (0 < map_startpos_count() && loading->server_state == S_S_INITIAL
      && map_startpos_count() < game.server.max_players) {
    qDebug("Number of starts (%d) are lower than rules.max_players "
           "(%d), lowering rules.max_players.",
           map_startpos_count(), game.server.max_players);
    game.server.max_players = map_startpos_count();
  }

  /* Re-initialize nation availability in light of start positions.
   * This has to be after loading [scenario] and [map].startpos and
   * before we seek nations for players. */
  update_nations_with_startpos();
}

/**
   Save the map start positions.
 */
static void sg_save_map_startpos(struct savedata *saving)
{
  struct tile *ptile;
  const char SEPARATOR = '#';
  int i = 0;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (!game.server.save_options.save_starts) {
    return;
  }

  secfile_insert_int(saving->file, map_startpos_count(),
                     "map.startpos_count");

  for (auto *psp : qAsConst(*wld.map.startpos_table)) {
    int nat_x, nat_y;
    if (psp->exclude) {
      continue;
    }
    ptile = startpos_tile(psp);

    index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
    secfile_insert_int(saving->file, nat_x, "map.startpos%d.x", i);
    secfile_insert_int(saving->file, nat_y, "map.startpos%d.y", i);

    secfile_insert_bool(saving->file, startpos_is_excluding(psp),
                        "map.startpos%d.exclude", i);
    if (startpos_allows_all(psp)) {
      secfile_insert_str(saving->file, "", "map.startpos%d.nations", i);
    } else {
      QSet<const nation_type *> *nations = startpos_raw_nations(psp);
      char nation_names[MAX_LEN_NAME * nations->size()];

      nation_names[0] = '\0';
      for (const auto *pnation : qAsConst(*nations)) {
        if ('\0' == nation_names[0]) {
          fc_strlcpy(nation_names, nation_rule_name(pnation),
                     sizeof(nation_names));
        } else {
          cat_snprintf(nation_names, sizeof(nation_names), "%c%s", SEPARATOR,
                       nation_rule_name(pnation));
        }
      }
      secfile_insert_str(saving->file, nation_names,
                         "map.startpos%d.nations", i);
    }
    i++;
  }

  fc_assert(map_startpos_count() == i);
}

/**
   Load tile owner information
 */
static void sg_load_map_owner(struct loaddata *loading)
{
  int x, y;
  struct tile *claimer = nullptr;
  struct extra_type *placing = nullptr;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (game.info.is_new_game) {
    /* No owner/source information for a new game / scenario. */
    return;
  }

  // Owner, ownership source, and infra turns are stored as plain numbers
  for (y = 0; y < wld.map.ysize; y++) {
    const char *buffer1 =
        secfile_lookup_str(loading->file, "map.owner%04d", y);
    const char *buffer2 =
        secfile_lookup_str(loading->file, "map.source%04d", y);
    const char *buffer3 =
        secfile_lookup_str(loading->file, "map.eowner%04d", y);
    const char *buffer_placing = secfile_lookup_str_default(
        loading->file, nullptr, "map.placing%04d", y);
    const char *buffer_turns = secfile_lookup_str_default(
        loading->file, nullptr, "map.infra_turns%04d", y);
    const char *ptr1 = buffer1;
    const char *ptr2 = buffer2;
    const char *ptr3 = buffer3;
    const char *ptr_placing = buffer_placing;
    const char *ptr_turns = buffer_turns;

    sg_failure_ret(buffer1 != nullptr, "%s", secfile_error());
    sg_failure_ret(buffer2 != nullptr, "%s", secfile_error());
    sg_failure_ret(buffer3 != nullptr, "%s", secfile_error());

    for (x = 0; x < wld.map.xsize; x++) {
      char token1[TOKEN_SIZE];
      char token2[TOKEN_SIZE];
      char token3[TOKEN_SIZE];
      char token_placing[TOKEN_SIZE];
      char token_turns[TOKEN_SIZE];
      struct player *owner = nullptr;
      struct player *eowner = nullptr;
      int turns;
      int number;
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);
      char n[] = ",";
      scanin(const_cast<char **>(&ptr1), n, token1, sizeof(token1));
      sg_failure_ret(token1[0] != '\0',
                     "Map size not correct (map.owner%d).", y);
      if (strcmp(token1, "-") == 0) {
        owner = nullptr;
      } else {
        sg_failure_ret(str_to_int(token1, &number),
                       "Got map owner %s in (%d, %d).", token1, x, y);
        owner = player_by_number(number);
      }
      scanin(const_cast<char **>(&ptr2), n, token2, sizeof(token2));
      sg_failure_ret(token2[0] != '\0',
                     "Map size not correct (map.source%d).", y);
      if (strcmp(token2, "-") == 0) {
        claimer = nullptr;
      } else {
        sg_failure_ret(str_to_int(token2, &number),
                       "Got map source %s in (%d, %d).", token2, x, y);
        claimer = index_to_tile(&(wld.map), number);
      }
      scanin(const_cast<char **>(&ptr3), n, token3, sizeof(token3));
      sg_failure_ret(token3[0] != '\0',
                     "Map size not correct (map.eowner%d).", y);
      if (strcmp(token3, "-") == 0) {
        eowner = nullptr;
      } else {
        sg_failure_ret(str_to_int(token3, &number),
                       "Got base owner %s in (%d, %d).", token3, x, y);
        eowner = player_by_number(number);
      }

      if (ptr_placing != nullptr) {
        char n[] = ",";
        scanin(const_cast<char **>(&ptr_placing), n, token_placing,
               sizeof(token_placing));
        sg_failure_ret(token_placing[0] != '\0',
                       "Map size not correct (map.placing%d).", y);
        if (strcmp(token_placing, "-") == 0) {
          placing = nullptr;
        } else {
          sg_failure_ret(str_to_int(token_placing, &number),
                         "Got placing extra %s in (%d, %d).", token_placing,
                         x, y);
          placing = extra_by_number(number);
        }
      } else {
        placing = nullptr;
      }

      if (ptr_turns != nullptr) {
        char n[] = ",";
        scanin(const_cast<char **>(&ptr_turns), n, token_turns,
               sizeof(token_turns));
        sg_failure_ret(token_turns[0] != '\0',
                       "Map size not correct (map.infra_turns%d).", y);
        sg_failure_ret(str_to_int(token_turns, &number),
                       "Got infra_turns %s in (%d, %d).", token_turns, x, y);
        turns = number;
      } else {
        turns = 1;
      }

      map_claim_ownership(ptile, owner, claimer, false);
      tile_claim_bases(ptile, eowner);
      ptile->placing = placing;
      ptile->infra_turns = turns;
      log_debug("extras_owner(%d, %d) = %s", TILE_XY(ptile),
                player_name(eowner));
    }
  }
}

/**
   Save tile owner information
 */
static void sg_save_map_owner(struct savedata *saving)
{
  int x, y;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (saving->scenario && !saving->save_players) {
    // Nothing to do for a scenario without saved players.
    return;
  }

  // Store owner and ownership source as plain numbers.
  for (y = 0; y < wld.map.ysize; y++) {
    char line[wld.map.xsize * TOKEN_SIZE];

    line[0] = '\0';
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      if (!saving->save_players || tile_owner(ptile) == nullptr) {
        qstrcpy(token, "-");
      } else {
        fc_snprintf(token, sizeof(token), "%d",
                    player_number(tile_owner(ptile)));
      }
      strcat(line, token);
      if (x + 1 < wld.map.xsize) {
        strcat(line, ",");
      }
    }
    secfile_insert_str(saving->file, line, "map.owner%04d", y);
  }

  for (y = 0; y < wld.map.ysize; y++) {
    char line[wld.map.xsize * TOKEN_SIZE];

    line[0] = '\0';
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      if (ptile->claimer == nullptr) {
        qstrcpy(token, "-");
      } else {
        fc_snprintf(token, sizeof(token), "%d", tile_index(ptile->claimer));
      }
      strcat(line, token);
      if (x + 1 < wld.map.xsize) {
        strcat(line, ",");
      }
    }
    secfile_insert_str(saving->file, line, "map.source%04d", y);
  }

  for (y = 0; y < wld.map.ysize; y++) {
    char line[wld.map.xsize * TOKEN_SIZE];

    line[0] = '\0';
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      if (!saving->save_players || extra_owner(ptile) == nullptr) {
        qstrcpy(token, "-");
      } else {
        fc_snprintf(token, sizeof(token), "%d",
                    player_number(extra_owner(ptile)));
      }
      strcat(line, token);
      if (x + 1 < wld.map.xsize) {
        strcat(line, ",");
      }
    }
    secfile_insert_str(saving->file, line, "map.eowner%04d", y);
  }

  for (y = 0; y < wld.map.ysize; y++) {
    char line[wld.map.xsize * TOKEN_SIZE];

    line[0] = '\0';
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      if (ptile->placing == nullptr) {
        qstrcpy(token, "-");
      } else {
        fc_snprintf(token, sizeof(token), "%d",
                    extra_number(ptile->placing));
      }
      strcat(line, token);
      if (x + 1 < wld.map.xsize) {
        strcat(line, ",");
      }
    }
    secfile_insert_str(saving->file, line, "map.placing%04d", y);
  }

  for (y = 0; y < wld.map.ysize; y++) {
    char line[wld.map.xsize * TOKEN_SIZE];

    line[0] = '\0';
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);

      if (ptile->placing != nullptr) {
        fc_snprintf(token, sizeof(token), "%d", ptile->infra_turns);
      } else {
        fc_snprintf(token, sizeof(token), "0");
      }
      strcat(line, token);
      if (x + 1 < wld.map.xsize) {
        strcat(line, ",");
      }
    }
    secfile_insert_str(saving->file, line, "map.infra_turns%04d", y);
  }
}

/**
   Load worked tiles information
 */
static void sg_load_map_worked(struct loaddata *loading)
{
  int x, y;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  sg_failure_ret(loading->worked_tiles == nullptr,
                 "City worked map not loaded!");

  loading->worked_tiles = new int[MAP_INDEX_SIZE];

  for (y = 0; y < wld.map.ysize; y++) {
    const char *buffer =
        secfile_lookup_str(loading->file, "map.worked%04d", y);
    const char *ptr = buffer;

    sg_failure_ret(nullptr != buffer,
                   "Savegame corrupt - map line %d not found.", y);
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      int number;
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);
      char n[] = ",";
      scanin(const_cast<char **>(&ptr), n, token, sizeof(token));
      sg_failure_ret('\0' != token[0],
                     "Savegame corrupt - map size not correct.");
      if (strcmp(token, "-") == 0) {
        number = -1;
      } else {
        sg_failure_ret(str_to_int(token, &number) && 0 < number,
                       "Savegame corrupt - got tile worked by city "
                       "id=%s in (%d, %d).",
                       token, x, y);
      }

      loading->worked_tiles[ptile->index] = number;
    }
  }
}

/**
   Save worked tiles information
 */
static void sg_save_map_worked(struct savedata *saving)
{
  int x, y;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (saving->scenario && !saving->save_players) {
    // Nothing to do for a scenario without saved players.
    return;
  }

  // additionally save the tiles worked by the cities
  for (y = 0; y < wld.map.ysize; y++) {
    char line[wld.map.xsize * TOKEN_SIZE];

    line[0] = '\0';
    for (x = 0; x < wld.map.xsize; x++) {
      char token[TOKEN_SIZE];
      struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);
      struct city *pcity = tile_worked(ptile);

      if (pcity == nullptr) {
        qstrcpy(token, "-");
      } else {
        fc_snprintf(token, sizeof(token), "%d", pcity->id);
      }
      strcat(line, token);
      if (x < wld.map.xsize) {
        strcat(line, ",");
      }
    }
    secfile_insert_str(saving->file, line, "map.worked%04d", y);
  }
}

/**
   Load tile known status
 */
static void sg_load_map_known(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  players_iterate(pplayer)
  {
    /* Allocate player private map here; it is needed in different modules
     * besides this one ((i.e. sg_load_player_*()). */
    player_map_init(pplayer);
  }
  players_iterate_end;

  if (secfile_lookup_bool_default(loading->file, true, "game.save_known")) {
    int lines = player_slot_max_used_number() / 32 + 1, j, p, l, i;
    unsigned int *known = new unsigned int[lines * MAP_INDEX_SIZE]();

    for (l = 0; l < lines; l++) {
      for (j = 0; j < 8; j++) {
        for (i = 0; i < 4; i++) {
          /* Only bother trying to load the map for this halfbyte if at least
           * one of the corresponding player slots is in use. */
          if (player_slot_is_used(
                  player_slot_by_number(l * 32 + j * 4 + i))) {
            LOAD_MAP_CHAR(ch, ptile,
                          known[l * MAP_INDEX_SIZE + tile_index(ptile)] |=
                          ascii_hex2bin(ch, j),
                          loading->file, "map.k%02d_%04d", l * 8 + j);
            break;
          }
        }
      }
    }

    players_iterate(pplayer) { pplayer->tile_known->fill(false); }
    players_iterate_end;

    /* HACK: we read the known data from hex into 32-bit integers, and
     * now we convert it to the known tile data of each player. */
    whole_map_iterate(&(wld.map), ptile)
    {
      players_iterate(pplayer)
      {
        p = player_index(pplayer);
        l = player_index(pplayer) / 32;

        if (known[l * MAP_INDEX_SIZE + tile_index(ptile)]
            & (1u << (p % 32))) {
          map_set_known(ptile, pplayer);
        }
      }
      players_iterate_end;
    }
    whole_map_iterate_end;

    delete[] known;
  }
}

/**
   Save tile known status for whole map and all players
 */
static void sg_save_map_known(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (!saving->save_players) {
    secfile_insert_bool(saving->file, false, "game.save_known");
    return;
  } else {
    int lines = player_slot_max_used_number() / 32 + 1;

    secfile_insert_bool(saving->file, game.server.save_options.save_known,
                        "game.save_known");
    if (game.server.save_options.save_known) {
      int j, p, l, i;
      QScopedArrayPointer<unsigned int> known(
          new unsigned int[lines * MAP_INDEX_SIZE]());

      /* HACK: we convert the data into a 32-bit integer, and then save it as
       * hex. */

      whole_map_iterate(&(wld.map), ptile)
      {
        players_iterate(pplayer)
        {
          if (map_is_known(ptile, pplayer)) {
            p = player_index(pplayer);
            l = p / 32;
            known[l * MAP_INDEX_SIZE + tile_index(ptile)] |=
                (1u << (p % 32)); // "p % 32" = "p - l * 32"
          }
        }
        players_iterate_end;
      }
      whole_map_iterate_end;

      for (l = 0; l < lines; l++) {
        for (j = 0; j < 8; j++) {
          for (i = 0; i < 4; i++) {
            /* Only bother saving the map for this halfbyte if at least one
             * of the corresponding player slots is in use */
            if (player_slot_is_used(
                    player_slot_by_number(l * 32 + j * 4 + i))) {
              // put 4-bit segments of the 32-bit "known" field
              SAVE_MAP_CHAR(
                  ptile,
                  bin2ascii_hex(
                      known[l * MAP_INDEX_SIZE + tile_index(ptile)], j),
                  saving->file, "map.k%02d_%04d", l * 8 + j);
              break;
            }
          }
        }
      }
    }
  }
}

/* =======================================================================
 * Load / save player data.
 *
 * This is splitted into two parts as some data can only be loaded if the
 * number of players is known and the corresponding player slots are
 * defined.
 * ======================================================================= */

/**
   Load '[player]' (basic data).
 */
static void sg_load_players_basic(struct loaddata *loading)
{
  int i, k, nplayers;
  const char *str;
  bool shuffle_loaded = true;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (S_S_INITIAL == loading->server_state || game.info.is_new_game) {
    // Nothing more to do.
    return;
  }

  // Load destroyed wonders:
  str = secfile_lookup_str(loading->file, "players.destroyed_wonders");
  sg_failure_ret(str != nullptr, "%s", secfile_error());
  sg_failure_ret(strlen(str) == loading->improvement.size,
                 "Invalid length for 'players.destroyed_wonders' "
                 "(%lu ~= %lu)",
                 (unsigned long) qstrlen(str),
                 (unsigned long) loading->improvement.size);
  for (k = 0; k < loading->improvement.size; k++) {
    sg_failure_ret(str[k] == '1' || str[k] == '0',
                   "Undefined value '%c' within "
                   "'players.destroyed_wonders'.",
                   str[k]);

    if (str[k] == '1') {
      struct impr_type *pimprove =
          improvement_by_rule_name(loading->improvement.order[k]);
      if (pimprove) {
        game.info.great_wonder_owners[improvement_index(pimprove)] =
            WONDER_DESTROYED;
      }
    }
  }

  server.identity_number = secfile_lookup_int_default(
      loading->file, server.identity_number, "players.identity_number_used");

  // First remove all defined players.
  players_iterate(pplayer) { server_remove_player(pplayer); }
  players_iterate_end;

  // Now, load the players from the savefile.
  player_slots_iterate(pslot)
  {
    struct player *pplayer;
    struct rgbcolor *prgbcolor = nullptr;
    int pslot_id = player_slot_index(pslot);

    if (nullptr
        == secfile_section_lookup(loading->file, "player%d", pslot_id)) {
      continue;
    }

    // Get player AI type.
    str = secfile_lookup_str(loading->file, "player%d.ai_type",
                             player_slot_index(pslot));
    sg_failure_ret(str != nullptr, "%s", secfile_error());

    // Get player color
    if (!rgbcolor_load(loading->file, &prgbcolor, "player%d.color",
                       pslot_id)) {
      if (game_was_started()) {
        log_sg("Game has started, yet player %d has no color defined.",
               pslot_id);
        // This will be fixed up later
      } else {
        qDebug("No color defined for player %d.", pslot_id);
        /* Colors will be assigned on game start, or at end of savefile
         * loading if game has already started */
      }
    }

    // Create player.
    pplayer = server_create_player(player_slot_index(pslot), str, prgbcolor,
                                   game.scenario.allow_ai_type_fallback);
    sg_failure_ret(pplayer != nullptr, "Invalid AI type: '%s'!", str);

    server_player_init(pplayer, false, false);

    // Free the color definition.
    rgbcolor_destroy(prgbcolor);

    // Multipliers (policies)

    /* First initialise player values with ruleset defaults; this will
     * cover any in the ruleset not known when the savefile was created. */
    multipliers_iterate(pmul)
    {
      pplayer->multipliers[multiplier_index(pmul)] =
          pplayer->multipliers_target[multiplier_index(pmul)] = pmul->def;
    }
    multipliers_iterate_end;

    // Now override with any values from the savefile.
    for (k = 0; k < loading->multiplier.size; k++) {
      const struct multiplier *pmul = loading->multiplier.order[k];

      if (pmul) {
        Multiplier_type_id idx = multiplier_index(pmul);
        int val = secfile_lookup_int_default(loading->file, pmul->def,
                                             "player%d.multiplier%d.val",
                                             player_slot_index(pslot), k);
        int rval = (((CLIP(pmul->start, val, pmul->stop) - pmul->start)
                     / pmul->step)
                    * pmul->step)
                   + pmul->start;

        if (rval != val) {
          qDebug("Player %d had illegal value for multiplier \"%s\": "
                 "was %d, clamped to %d",
                 pslot_id, multiplier_rule_name(pmul), val, rval);
        }
        pplayer->multipliers[idx] = rval;

        val = secfile_lookup_int_default(
            loading->file, pplayer->multipliers[idx],
            "player%d.multiplier%d.target", player_slot_index(pslot), k);
        rval = (((CLIP(pmul->start, val, pmul->stop) - pmul->start)
                 / pmul->step)
                * pmul->step)
               + pmul->start;

        if (rval != val) {
          qDebug("Player %d had illegal value for multiplier_target "
                 " \"%s\": was %d, clamped to %d",
                 pslot_id, multiplier_rule_name(pmul), val, rval);
        }
        pplayer->multipliers_target[idx] = rval;
      } // else silently discard multiplier not in current ruleset
    }

    // Must be loaded before tile owner is set.
    pplayer->server.border_vision = secfile_lookup_bool_default(
        loading->file, false, "player%d.border_vision",
        player_slot_index(pslot));
  }
  player_slots_iterate_end;

  // check number of players
  nplayers =
      secfile_lookup_int_default(loading->file, 0, "players.nplayers");
  sg_failure_ret(player_count() == nplayers,
                 "The value of players.nplayers "
                 "(%d) from the loaded game does not match the number of "
                 "players present (%d).",
                 nplayers, player_count());

  // Load team informations.
  players_iterate(pplayer)
  {
    int team;
    struct team_slot *tslot = nullptr;

    sg_failure_ret(secfile_lookup_int(loading->file, &team,
                                      "player%d.team_no",
                                      player_number(pplayer))
                       && (tslot = team_slot_by_number(team)),
                   "Invalid team definition for player %s (nb %d).",
                   player_name(pplayer), player_number(pplayer));
    team_add_player(pplayer, team_new(tslot));
  }
  players_iterate_end;

  /* Loading the shuffle list is quite complex. At the time of saving the
   * shuffle data is saved as
   *   shuffled_player_<number> = player_slot_id
   * where number is an increasing number and player_slot_id is a number
   * between 0 and the maximum number of player slots. Now we have to create
   * a list
   *   shuffler_players[number] = player_slot_id
   * where all player slot IDs are used exactly one time. The code below
   * handles this ... */
  if (secfile_lookup_int_default(loading->file, -1,
                                 "players.shuffled_player_%d", 0)
      >= 0) {
    int shuffled_players[MAX_NUM_PLAYER_SLOTS];
    bool shuffled_player_set[MAX_NUM_PLAYER_SLOTS];

    player_slots_iterate(pslot)
    {
      int plrid = player_slot_index(pslot);

      // Array to save used numbers.
      shuffled_player_set[plrid] = false;
      /* List of all player IDs (needed for set_shuffled_players()). It is
       * initialised with the value -1 to indicate that no value is set. */
      shuffled_players[plrid] = -1;
    }
    player_slots_iterate_end;

    // Load shuffled player list.
    for (i = 0; i < player_count(); i++) {
      int shuffle = secfile_lookup_int_default(
          loading->file, -1, "players.shuffled_player_%d", i);

      if (shuffle == -1) {
        log_sg("Missing player shuffle information (index %d) "
               "- reshuffle player list!",
               i);
        shuffle_loaded = false;
        break;
      } else if (shuffled_player_set[shuffle]) {
        log_sg("Player shuffle %d used two times "
               "- reshuffle player list!",
               shuffle);
        shuffle_loaded = false;
        break;
      }
      // Set this ID as used.
      shuffled_player_set[shuffle] = true;

      // Save the player ID in the shuffle list.
      shuffled_players[i] = shuffle;
    }

    if (shuffle_loaded) {
      // Insert missing numbers.
      int shuffle_index = player_count();

      for (i = 0; i < MAX_NUM_PLAYER_SLOTS; i++) {
        if (!shuffled_player_set[i]) {
          shuffled_players[shuffle_index] = i;
          shuffle_index++;
        }
        /* shuffle_index must not grow behind the size of shuffled_players.
         */
        sg_failure_ret(shuffle_index <= MAX_NUM_PLAYER_SLOTS,
                       "Invalid player shuffle data!");
      }

#ifdef FREECIV_DEBUG
      log_debug("[load shuffle] player_count() = %d", player_count());
      player_slots_iterate(pslot)
      {
        int plrid = player_slot_index(pslot);
        log_debug("[load shuffle] id: %3d => slot: %3d | slot %3d: %s",
                  plrid, shuffled_players[plrid], plrid,
                  shuffled_player_set[plrid] ? "is used" : "-");
      }
      player_slots_iterate_end;
#endif // FREECIV_DEBUG

      // Set shuffle list from savegame.
      set_shuffled_players(shuffled_players);
    }
  }

  if (!shuffle_loaded) {
    /* No shuffled players included or error loading them, so shuffle them
     * (this may include scenarios). */
    shuffle_players();
  }
}

/**
   Load '[player]'.
 */
static void sg_load_players(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (game.info.is_new_game) {
    // Nothing to do.
    return;
  }

  players_iterate(pplayer)
  {
    sg_load_player_main(loading, pplayer);
    sg_load_player_cities(loading, pplayer);
    sg_load_player_units(loading, pplayer);
    sg_load_player_attributes(loading, pplayer);

    // Check the success of the functions above.
    sg_check_ret();

    // print out some informations
    if (is_ai(pplayer)) {
      qInfo(_("%s has been added as %s level AI-controlled player "
              "(%s)."),
            player_name(pplayer),
            ai_level_translated_name(pplayer->ai_common.skill_level),
            ai_name(pplayer->ai));
    } else {
      qInfo(_("%s has been added as human player."), player_name(pplayer));
    }
  }
  players_iterate_end;

  /* Also load the transport status of the units here. It must be a special
   * case as all units must be known (unit on an allied transporter). */
  players_iterate(pplayer)
  {
    // Load unit transport status.
    sg_load_player_units_transport(loading, pplayer);
  }
  players_iterate_end;

  /* Savegame may contain nation assignments that are incompatible with the
   * current nationset -- for instance, if it predates the introduction of
   * nationsets. Ensure they are compatible, one way or another. */
  fit_nationset_to_players();

  /* Some players may have invalid nations in the ruleset. Once all players
   * are loaded, pick one of the remaining nations for them. */
  players_iterate(pplayer)
  {
    if (pplayer->nation == NO_NATION_SELECTED) {
      player_set_nation(
          pplayer, pick_a_nation(nullptr, false, true, NOT_A_BARBARIAN));
      // TRANS: Minor error message: <Leader> ... <Poles>.
      log_sg(_("%s had invalid nation; changing to %s."),
             player_name(pplayer), nation_plural_for_player(pplayer));

      ai_traits_init(pplayer);
    }
  }
  players_iterate_end;

  // Sanity check alliances, prevent allied-with-ally-of-enemy.
  players_iterate_alive(plr)
  {
    players_iterate_alive(aplayer)
    {
      if (pplayers_allied(plr, aplayer)) {
        enum dipl_reason can_ally =
            pplayer_can_make_treaty(plr, aplayer, DS_ALLIANCE);
        if (can_ally == DIPL_ALLIANCE_PROBLEM_US
            || can_ally == DIPL_ALLIANCE_PROBLEM_THEM) {
          log_sg("Illegal alliance structure detected: "
                 "%s alliance to %s reduced to peace treaty.",
                 nation_rule_name(nation_of_player(plr)),
                 nation_rule_name(nation_of_player(aplayer)));
          player_diplstate_get(plr, aplayer)->type = DS_PEACE;
          player_diplstate_get(aplayer, plr)->type = DS_PEACE;
        }
      }
    }
    players_iterate_alive_end;
  }
  players_iterate_alive_end;

  /* Update cached city illness. This can depend on trade routes,
   * so can't be calculated until all players have been loaded. */
  if (game.info.illness_on) {
    cities_iterate(pcity)
    {
      pcity->server.illness = city_illness_calc(
          pcity, nullptr, nullptr, &(pcity->illness_trade), nullptr);
    }
    cities_iterate_end;
  }

  /* Update all city information.  This must come after all cities are
   * loaded (in player_load) but before player (dumb) cities are loaded
   * in player_load_vision(). */
  players_iterate(plr)
  {
    city_list_iterate(plr->cities, pcity)
    {
      city_refresh(pcity);
      sanity_check_city(pcity);
      CALL_PLR_AI_FUNC(city_got, plr, plr, pcity);
    }
    city_list_iterate_end;
  }
  players_iterate_end;

  /* Since the cities must be placed on the map to put them on the
     player map we do this afterwards */
  players_iterate(pplayer)
  {
    sg_load_player_vision(loading, pplayer);
    // Check the success of the function above.
    sg_check_ret();
  }
  players_iterate_end;

  // Check shared vision.
  players_iterate(pplayer)
  {
    BV_CLR_ALL(pplayer->gives_shared_vision);
    BV_CLR_ALL(pplayer->server.really_gives_vision);
  }
  players_iterate_end;

  players_iterate(pplayer)
  {
    int plr1 = player_index(pplayer);

    players_iterate(pplayer2)
    {
      int plr2 = player_index(pplayer2);
      if (secfile_lookup_bool_default(
              loading->file, false,
              "player%d.diplstate%d.gives_shared_vision", plr1, plr2)) {
        give_shared_vision(pplayer, pplayer2);
      }
    }
    players_iterate_end;
  }
  players_iterate_end;

  initialize_globals();
  unit_ordering_apply();

  // All vision is ready; this calls city_thaw_workers_queue().
  map_calculate_borders();

  // Make sure everything is consistent.
  players_iterate(pplayer)
  {
    unit_list_iterate(pplayer->units, punit)
    {
      if (!can_unit_continue_current_activity(punit)) {
        log_sg("Unit doing illegal activity in savegame!");
        log_sg(
            "Activity: %s, Target: %s", unit_activity_name(punit->activity),
            punit->activity_target ? extra_rule_name(punit->activity_target)
                                   : "missing");
        punit->activity = ACTIVITY_IDLE;
      }
    }
    unit_list_iterate_end;
  }
  players_iterate_end;

  cities_iterate(pcity)
  {
    city_refresh(pcity);
    city_thaw_workers(pcity); // may auto_arrange_workers()
  }
  cities_iterate_end;

  /* Player colors are always needed once game has started. Pre-2.4 savegames
   * lack them. This cannot be in compatibility conversion layer as we need
   * all the player data available to be able to assign best colors. */
  if (game_was_started()) {
    assign_player_colors();
  }
}

/**
   Save '[player]'.
 */
static void sg_save_players(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if ((saving->scenario && !saving->save_players) || !game_was_started()) {
    /* Nothing to do for a scenario without saved players or a game in
     * INITIAL state. */
    return;
  }

  secfile_insert_int(saving->file, player_count(), "players.nplayers");

  /* Save destroyed wonders as bitvector. Note that improvement order
   * is saved in 'savefile.improvement.order'. */
  {
    char destroyed[B_LAST + 1];

    improvement_iterate(pimprove)
    {
      if (is_great_wonder(pimprove) && great_wonder_is_destroyed(pimprove)) {
        destroyed[improvement_index(pimprove)] = '1';
      } else {
        destroyed[improvement_index(pimprove)] = '0';
      }
    }
    improvement_iterate_end;
    destroyed[improvement_count()] = '\0';
    secfile_insert_str(saving->file, destroyed, "players.destroyed_wonders");
  }

  secfile_insert_int(saving->file, server.identity_number,
                     "players.identity_number_used");

  // Save player order.
  {
    int i = 0;
    shuffled_players_iterate(pplayer)
    {
      secfile_insert_int(saving->file, player_number(pplayer),
                         "players.shuffled_player_%d", i);
      i++;
    }
    shuffled_players_iterate_end;
  }

  // Sort units.
  unit_ordering_calc();

  // Save players.
  players_iterate(pplayer)
  {
    sg_save_player_main(saving, pplayer);
    sg_save_player_cities(saving, pplayer);
    sg_save_player_units(saving, pplayer);
    sg_save_player_attributes(saving, pplayer);
    sg_save_player_vision(saving, pplayer);
  }
  players_iterate_end;
}

/**
   Main player data loading function
 */
static void sg_load_player_main(struct loaddata *loading, struct player *plr)
{
  const char **slist;
  int i, plrno = player_number(plr);
  const char *str;
  struct government *gov;
  const char *level;
  const char *barb_str;
  size_t nval;
  const char *kind;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Basic player data.
  str = secfile_lookup_str(loading->file, "player%d.name", plrno);
  sg_failure_ret(str != nullptr, "%s", secfile_error());
  server_player_set_name(plr, str);
  sz_strlcpy(plr->username,
             secfile_lookup_str_default(loading->file, "",
                                        "player%d.username", plrno));
  sg_failure_ret(secfile_lookup_bool(loading->file, &plr->unassigned_user,
                                     "player%d.unassigned_user", plrno),
                 "%s", secfile_error());
  sz_strlcpy(plr->server.orig_username,
             secfile_lookup_str_default(loading->file, "",
                                        "player%d.orig_username", plrno));
  sz_strlcpy(plr->ranked_username,
             secfile_lookup_str_default(loading->file, "",
                                        "player%d.ranked_username", plrno));
  sg_failure_ret(secfile_lookup_bool(loading->file, &plr->unassigned_ranked,
                                     "player%d.unassigned_ranked", plrno),
                 "%s", secfile_error());
  str = secfile_lookup_str_default(loading->file, "",
                                   "player%d.delegation_username", plrno);
  // Defaults to no delegation.
  if (strlen(str)) {
    player_delegation_set(plr, str);
  }

  // Player flags
  BV_CLR_ALL(plr->flags);
  slist =
      secfile_lookup_str_vec(loading->file, &nval, "player%d.flags", plrno);
  for (i = 0; i < nval; i++) {
    const char *sval = slist[i];
    enum plr_flag_id fid = plr_flag_id_by_name(sval, fc_strcasecmp);

    BV_SET(plr->flags, fid);
  }
  delete[] slist;

  // Nation
  str = secfile_lookup_str(loading->file, "player%d.nation", plrno);
  player_set_nation(plr, nation_by_rule_name(str));
  if (plr->nation != nullptr) {
    ai_traits_init(plr);
  }

  // Government
  str = secfile_lookup_str(loading->file, "player%d.government_name", plrno);
  gov = government_by_rule_name(str);
  sg_failure_ret(gov != nullptr, "Player%d: unsupported government \"%s\".",
                 plrno, str);
  plr->government = gov;

  // Target government
  str = secfile_lookup_str(loading->file, "player%d.target_government_name",
                           plrno);
  if (str != nullptr) {
    plr->target_government = government_by_rule_name(str);
  } else {
    plr->target_government = nullptr;
  }
  plr->revolution_finishes = secfile_lookup_int_default(
      loading->file, -1, "player%d.revolution_finishes", plrno);

  sg_failure_ret(secfile_lookup_bool(loading->file,
                                     &plr->server.got_first_city,
                                     "player%d.got_first_city", plrno),
                 "%s", secfile_error());

  /* Load diplomatic data (diplstate + embassy + vision).
   * Shared vision is loaded in sg_load_players(). */
  BV_CLR_ALL(plr->real_embassy);
  players_iterate(pplayer)
  {
    char buf[32];
    struct player_diplstate *ds = player_diplstate_get(plr, pplayer);
    i = player_index(pplayer);

    // load diplomatic status
    fc_snprintf(buf, sizeof(buf), "player%d.diplstate%d", plrno, i);

    ds->type = static_cast<diplstate_type>(secfile_lookup_enum_default(
        loading->file, DS_WAR, diplstate_type, "%s.current", buf));
    ds->max_state = static_cast<diplstate_type>(secfile_lookup_enum_default(
        loading->file, DS_WAR, diplstate_type, "%s.closest", buf));
    ds->first_contact_turn = secfile_lookup_int_default(
        loading->file, 0, "%s.first_contact_turn", buf);
    ds->turns_left =
        secfile_lookup_int_default(loading->file, -2, "%s.turns_left", buf);
    ds->has_reason_to_cancel = secfile_lookup_int_default(
        loading->file, 0, "%s.has_reason_to_cancel", buf);
    ds->contact_turns_left = secfile_lookup_int_default(
        loading->file, 0, "%s.contact_turns_left", buf);

    if (secfile_lookup_bool_default(loading->file, false, "%s.embassy",
                                    buf)) {
      BV_SET(plr->real_embassy, i);
    }
    /* 'gives_shared_vision' is loaded in sg_load_players() as all cities
     * must be known. */
  }
  players_iterate_end;

  // load ai data
  players_iterate(aplayer)
  {
    char buf[32];

    fc_snprintf(buf, sizeof(buf), "player%d.ai%d", plrno,
                player_index(aplayer));

    plr->ai_common.love[player_index(aplayer)] =
        secfile_lookup_int_default(loading->file, 1, "%s.love", buf);
    CALL_FUNC_EACH_AI(player_load_relations, plr, aplayer, loading->file,
                      plrno);
  }
  players_iterate_end;

  CALL_FUNC_EACH_AI(player_load, plr, loading->file, plrno);

  // Some sane defaults
  plr->ai_common.fuzzy = 0;
  plr->ai_common.expand = 100;
  plr->ai_common.science_cost = 100;

  level = secfile_lookup_str_default(loading->file, nullptr,
                                     "player%d.ai.level", plrno);
  if (level != nullptr) {
    plr->ai_common.skill_level = ai_level_by_name(level, fc_strcasecmp);

    /* In builds where level "Experimental" is not supported, convert it to
     * "Hard" */
    if (!ai_level_is_valid(plr->ai_common.skill_level)
        && !fc_strcasecmp(level, "Experimental")) {
      plr->ai_common.skill_level = AI_LEVEL_HARD;
    }
  } else {
    plr->ai_common.skill_level = ai_level_invalid();
  }

  if (!ai_level_is_valid(plr->ai_common.skill_level)) {
    plr->ai_common.skill_level = ai_level_convert(
        secfile_lookup_int_default(loading->file, game.info.skill_level,
                                   "player%d.ai.skill_level", plrno));
  }

  barb_str = secfile_lookup_str_default(loading->file, "None",
                                        "player%d.ai.barb_type", plrno);
  plr->ai_common.barbarian_type =
      barbarian_type_by_name(barb_str, fc_strcasecmp);

  if (!barbarian_type_is_valid(plr->ai_common.barbarian_type)) {
    log_sg("Player%d: Invalid barbarian type \"%s\". "
           "Changed to \"None\".",
           plrno, barb_str);
    plr->ai_common.barbarian_type = NOT_A_BARBARIAN;
  }

  if (is_barbarian(plr)) {
    server.nbarbarians++;
  }

  if (is_ai(plr)) {
    set_ai_level_directer(plr, plr->ai_common.skill_level);
    CALL_PLR_AI_FUNC(gained_control, plr, plr);
  }

  // Load nation style.
  {
    struct nation_style *style;

    str = secfile_lookup_str(loading->file, "player%d.style_by_name", plrno);

    sg_failure_ret(str != nullptr, "%s", secfile_error());
    style = style_by_rule_name(str);
    if (style == nullptr) {
      style = style_by_number(0);
      log_sg("Player%d: unsupported city_style_name \"%s\". "
             "Changed to \"%s\".",
             plrno, str, style_rule_name(style));
    }
    plr->style = style;
  }

  sg_failure_ret(secfile_lookup_int(loading->file, &plr->nturns_idle,
                                    "player%d.idle_turns", plrno),
                 "%s", secfile_error());
  kind = secfile_lookup_str(loading->file, "player%d.kind", plrno);
  plr->is_male = strcmp("male", kind) == 0;
  sg_failure_ret(secfile_lookup_bool(loading->file, &plr->is_alive,
                                     "player%d.is_alive", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->turns_alive,
                                    "player%d.turns_alive", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->last_war_action,
                                    "player%d.last_war", plrno),
                 "%s", secfile_error());
  plr->phase_done = secfile_lookup_bool_default(
      loading->file, false, "player%d.phase_done", plrno);
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.gold,
                                    "player%d.gold", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.tax,
                                    "player%d.rates.tax", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.science,
                                    "player%d.rates.science", plrno),
                 "%s", secfile_error());
  sg_failure_ret(secfile_lookup_int(loading->file, &plr->economic.luxury,
                                    "player%d.rates.luxury", plrno),
                 "%s", secfile_error());
  plr->server.bulbs_last_turn = secfile_lookup_int_default(
      loading->file, 0, "player%d.research.bulbs_last_turn", plrno);

  // Traits
  if (plr->nation) {
    for (i = 0; i < loading->trait.size; i++) {
      enum trait tr = trait_by_name(loading->trait.order[i], fc_strcasecmp);

      if (trait_is_valid(tr)) {
        int val;

        sg_failure_ret(secfile_lookup_int(loading->file, &val,
                                          "player%d.trait%d.val", plrno, i),
                       "%s", secfile_error());
        plr->ai_common.traits[tr].val = val;

        sg_failure_ret(secfile_lookup_int(loading->file, &val,
                                          "player%d.trait%d.mod", plrno, i),
                       "%s", secfile_error());
        plr->ai_common.traits[tr].mod = val;
      }
    }
  }

  // Achievements
  {
    int count;

    count = secfile_lookup_int_default(loading->file, -1,
                                       "player%d.achievement_count", plrno);

    if (count > 0) {
      for (i = 0; i < count; i++) {
        const char *name;
        struct achievement *pach;
        bool first;

        name = secfile_lookup_str(loading->file,
                                  "player%d.achievement%d.name", plrno, i);
        pach = achievement_by_rule_name(name);

        sg_failure_ret(pach != nullptr, "Unknown achievement \"%s\".", name);

        sg_failure_ret(secfile_lookup_bool(loading->file, &first,
                                           "player%d.achievement%d.first",
                                           plrno, i),
                       "achievement error: %s", secfile_error());

        sg_failure_ret(
            pach->first == nullptr || !first,
            "Multiple players listed as first to get achievement \"%s\".",
            name);

        BV_SET(pach->achievers, player_index(plr));

        if (first) {
          pach->first = plr;
        }
      }
    }
  }

  // Player score.
  plr->score.happy =
      secfile_lookup_int_default(loading->file, 0, "score%d.happy", plrno);
  plr->score.content =
      secfile_lookup_int_default(loading->file, 0, "score%d.content", plrno);
  plr->score.unhappy =
      secfile_lookup_int_default(loading->file, 0, "score%d.unhappy", plrno);
  plr->score.angry =
      secfile_lookup_int_default(loading->file, 0, "score%d.angry", plrno);

  /* Make sure that the score about specialists in current ruleset that
   * were not present at saving time are set to zero. */
  specialist_type_iterate(sp) { plr->score.specialists[sp] = 0; }
  specialist_type_iterate_end;

  for (i = 0; i < loading->specialist.size; i++) {
    plr->score.specialists[specialist_index(loading->specialist.order[i])] =
        secfile_lookup_int_default(loading->file, 0, "score%d.specialists%d",
                                   plrno, i);
  }

  plr->score.wonders =
      secfile_lookup_int_default(loading->file, 0, "score%d.wonders", plrno);
  plr->score.techs =
      secfile_lookup_int_default(loading->file, 0, "score%d.techs", plrno);
  plr->score.techout =
      secfile_lookup_int_default(loading->file, 0, "score%d.techout", plrno);
  plr->score.landarea = secfile_lookup_int_default(
      loading->file, 0, "score%d.landarea", plrno);
  plr->score.settledarea = secfile_lookup_int_default(
      loading->file, 0, "score%d.settledarea", plrno);
  plr->score.population = secfile_lookup_int_default(
      loading->file, 0, "score%d.population", plrno);
  plr->score.cities =
      secfile_lookup_int_default(loading->file, 0, "score%d.cities", plrno);
  plr->score.units =
      secfile_lookup_int_default(loading->file, 0, "score%d.units", plrno);
  plr->score.pollution = secfile_lookup_int_default(
      loading->file, 0, "score%d.pollution", plrno);
  plr->score.literacy = secfile_lookup_int_default(
      loading->file, 0, "score%d.literacy", plrno);
  plr->score.bnp =
      secfile_lookup_int_default(loading->file, 0, "score%d.bnp", plrno);
  plr->score.mfg =
      secfile_lookup_int_default(loading->file, 0, "score%d.mfg", plrno);
  plr->score.spaceship = secfile_lookup_int_default(
      loading->file, 0, "score%d.spaceship", plrno);
  plr->score.units_built = secfile_lookup_int_default(
      loading->file, 0, "score%d.units_built", plrno);
  plr->score.units_killed = secfile_lookup_int_default(
      loading->file, 0, "score%d.units_killed", plrno);
  plr->score.units_lost = secfile_lookup_int_default(
      loading->file, 0, "score%d.units_lost", plrno);
  plr->score.culture =
      secfile_lookup_int_default(loading->file, 0, "score%d.culture", plrno);
  plr->score.game =
      secfile_lookup_int_default(loading->file, 0, "score%d.total", plrno);

  // Load space ship data.
  {
    struct player_spaceship *ship = &plr->spaceship;
    char prefix[32];
    const char *st;
    int ei;

    fc_snprintf(prefix, sizeof(prefix), "player%d.spaceship", plrno);
    spaceship_init(ship);
    sg_failure_ret(
        secfile_lookup_int(loading->file, &ei, "%s.state", prefix), "%s",
        secfile_error());
    ship->state = static_cast<spaceship_state>(ei);

    if (ship->state != SSHIP_NONE) {
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->structurals,
                                        "%s.structurals", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->components,
                                        "%s.components", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->modules,
                                        "%s.modules", prefix),
                     "%s", secfile_error());
      sg_failure_ret(
          secfile_lookup_int(loading->file, &ship->fuel, "%s.fuel", prefix),
          "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->propulsion,
                                        "%s.propulsion", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->habitation,
                                        "%s.habitation", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->life_support,
                                        "%s.life_support", prefix),
                     "%s", secfile_error());
      sg_failure_ret(secfile_lookup_int(loading->file, &ship->solar_panels,
                                        "%s.solar_panels", prefix),
                     "%s", secfile_error());

      st = secfile_lookup_str(loading->file, "%s.structure", prefix);
      sg_failure_ret(
          st != nullptr, "%s",
          secfile_error()) for (i = 0; i < NUM_SS_STRUCTURALS && st[i]; i++)
      {
        sg_failure_ret(st[i] == '1' || st[i] == '0',
                       "Undefined value '%c' within '%s.structure'.", st[i],
                       prefix)

            if (!(st[i] == '0'))
        {
          BV_SET(ship->structure, i);
        }
      }
      if (ship->state >= SSHIP_LAUNCHED) {
        sg_failure_ret(secfile_lookup_int(loading->file, &ship->launch_year,
                                          "%s.launch_year", prefix),
                       "%s", secfile_error());
      }
      spaceship_calc_derived(ship);
    }
  }

  // Load lost wonder data.
  str = secfile_lookup_str(loading->file, "player%d.lost_wonders", plrno);
  // If not present, probably an old savegame; nothing to be done
  if (str != nullptr) {
    int k;
    sg_failure_ret(strlen(str) == loading->improvement.size,
                   "Invalid length for 'player%d.lost_wonders' "
                   "(%lu ~= %lu)",
                   plrno, (unsigned long) qstrlen(str),
                   (unsigned long) loading->improvement.size);
    for (k = 0; k < loading->improvement.size; k++) {
      sg_failure_ret(str[k] == '1' || str[k] == '0',
                     "Undefined value '%c' within "
                     "'player%d.lost_wonders'.",
                     plrno, str[k]);

      if (str[k] == '1') {
        struct impr_type *pimprove =
            improvement_by_rule_name(loading->improvement.order[k]);
        if (pimprove) {
          plr->wonders[improvement_index(pimprove)] = WONDER_LOST;
        }
      }
    }
  }

  plr->history = secfile_lookup_int_default(loading->file, 0,
                                            "player%d.history", plrno);
  plr->server.huts = secfile_lookup_int_default(loading->file, 0,
                                                "player%d.hut_count", plrno);
}

/**
   Main player data saving function.
 */
static void sg_save_player_main(struct savedata *saving, struct player *plr)
{
  int i, k, plrno = player_number(plr);
  struct player_spaceship *ship = &plr->spaceship;
  const char *flag_names[PLRF_COUNT];
  int set_count;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  set_count = 0;
  for (i = 0; i < PLRF_COUNT; i++) {
    if (player_has_flag(plr, static_cast<plr_flag_id>(i))) {
      flag_names[set_count++] =
          plr_flag_id_name(static_cast<plr_flag_id>(i));
    }
  }

  secfile_insert_str_vec(saving->file, flag_names, set_count,
                         "player%d.flags", plrno);

  secfile_insert_str(saving->file, ai_name(plr->ai), "player%d.ai_type",
                     plrno);
  secfile_insert_str(saving->file, player_name(plr), "player%d.name", plrno);
  secfile_insert_str(saving->file, plr->username, "player%d.username",
                     plrno);
  secfile_insert_bool(saving->file, plr->unassigned_user,
                      "player%d.unassigned_user", plrno);
  if (plr->rgb != nullptr) {
    rgbcolor_save(saving->file, plr->rgb, "player%d.color", plrno);
  } else {
    // Colorless players are ok in pregame
    if (game_was_started()) {
      log_sg("Game has started, yet player %d has no color defined.", plrno);
    }
  }
  secfile_insert_str(saving->file, plr->ranked_username,
                     "player%d.ranked_username", plrno);
  secfile_insert_bool(saving->file, plr->unassigned_ranked,
                      "player%d.unassigned_ranked", plrno);
  secfile_insert_str(saving->file, plr->server.orig_username,
                     "player%d.orig_username", plrno);
  secfile_insert_str(saving->file,
                     player_delegation_get(plr) ? player_delegation_get(plr)
                                                : "",
                     "player%d.delegation_username", plrno);
  secfile_insert_str(saving->file, nation_rule_name(nation_of_player(plr)),
                     "player%d.nation", plrno);
  secfile_insert_int(saving->file, plr->team ? team_index(plr->team) : -1,
                     "player%d.team_no", plrno);

  secfile_insert_str(saving->file,
                     government_rule_name(government_of_player(plr)),
                     "player%d.government_name", plrno);

  if (plr->target_government) {
    secfile_insert_str(saving->file,
                       government_rule_name(plr->target_government),
                       "player%d.target_government_name", plrno);
  }

  secfile_insert_str(saving->file, style_rule_name(plr->style),
                     "player%d.style_by_name", plrno);

  secfile_insert_int(saving->file, plr->nturns_idle, "player%d.idle_turns",
                     plrno);
  if (plr->is_male) {
    secfile_insert_str(saving->file, "male", "player%d.kind", plrno);
  } else {
    secfile_insert_str(saving->file, "female", "player%d.kind", plrno);
  }
  secfile_insert_bool(saving->file, plr->is_alive, "player%d.is_alive",
                      plrno);
  secfile_insert_int(saving->file, plr->turns_alive, "player%d.turns_alive",
                     plrno);
  secfile_insert_int(saving->file, plr->last_war_action, "player%d.last_war",
                     plrno);
  secfile_insert_bool(saving->file, plr->phase_done, "player%d.phase_done",
                      plrno);

  players_iterate(pplayer)
  {
    char buf[32];
    struct player_diplstate *ds = player_diplstate_get(plr, pplayer);

    i = player_index(pplayer);

    // save diplomatic state
    fc_snprintf(buf, sizeof(buf), "player%d.diplstate%d", plrno, i);

    secfile_insert_enum(saving->file, ds->type, diplstate_type, "%s.current",
                        buf);
    secfile_insert_enum(saving->file, ds->max_state, diplstate_type,
                        "%s.closest", buf);
    secfile_insert_int(saving->file, ds->first_contact_turn,
                       "%s.first_contact_turn", buf);
    secfile_insert_int(saving->file, ds->turns_left, "%s.turns_left", buf);
    secfile_insert_int(saving->file, ds->has_reason_to_cancel,
                       "%s.has_reason_to_cancel", buf);
    secfile_insert_int(saving->file, ds->contact_turns_left,
                       "%s.contact_turns_left", buf);
    secfile_insert_bool(saving->file, player_has_real_embassy(plr, pplayer),
                        "%s.embassy", buf);
    secfile_insert_bool(saving->file, gives_shared_vision(plr, pplayer),
                        "%s.gives_shared_vision", buf);
  }
  players_iterate_end;

  players_iterate(aplayer)
  {
    i = player_index(aplayer);
    // save ai data
    secfile_insert_int(saving->file, plr->ai_common.love[i],
                       "player%d.ai%d.love", plrno, i);
    CALL_FUNC_EACH_AI(player_save_relations, plr, aplayer, saving->file,
                      plrno);
  }
  players_iterate_end;

  CALL_FUNC_EACH_AI(player_save, plr, saving->file, plrno);

  // Multipliers (policies)
  i = multiplier_count();

  for (k = 0; k < i; k++) {
    secfile_insert_int(saving->file, plr->multipliers[k],
                       "player%d.multiplier%d.val", plrno, k);
    secfile_insert_int(saving->file, plr->multipliers_target[k],
                       "player%d.multiplier%d.target", plrno, k);
  }

  secfile_insert_str(saving->file, ai_level_name(plr->ai_common.skill_level),
                     "player%d.ai.level", plrno);
  secfile_insert_str(saving->file,
                     barbarian_type_name(plr->ai_common.barbarian_type),
                     "player%d.ai.barb_type", plrno);
  secfile_insert_int(saving->file, plr->economic.gold, "player%d.gold",
                     plrno);
  secfile_insert_int(saving->file, plr->economic.tax, "player%d.rates.tax",
                     plrno);
  secfile_insert_int(saving->file, plr->economic.science,
                     "player%d.rates.science", plrno);
  secfile_insert_int(saving->file, plr->economic.luxury,
                     "player%d.rates.luxury", plrno);
  secfile_insert_int(saving->file, plr->server.bulbs_last_turn,
                     "player%d.research.bulbs_last_turn", plrno);

  // Save traits
  {
    enum trait tr;
    int j;

    for (tr = trait_begin(), j = 0; tr != trait_end();
         tr = trait_next(tr), j++) {
      secfile_insert_int(saving->file, plr->ai_common.traits[tr].val,
                         "player%d.trait%d.val", plrno, j);
      secfile_insert_int(saving->file, plr->ai_common.traits[tr].mod,
                         "player%d.trait%d.mod", plrno, j);
    }
  }

  // Save achievements
  {
    int j = 0;

    achievements_iterate(pach)
    {
      if (achievement_player_has(pach, plr)) {
        secfile_insert_str(saving->file, achievement_rule_name(pach),
                           "player%d.achievement%d.name", plrno, j);
        if (pach->first == plr) {
          secfile_insert_bool(saving->file, true,
                              "player%d.achievement%d.first", plrno, j);
        } else {
          secfile_insert_bool(saving->file, false,
                              "player%d.achievement%d.first", plrno, j);
        }

        j++;
      }
    }
    achievements_iterate_end;

    secfile_insert_int(saving->file, j, "player%d.achievement_count", plrno);
  }

  secfile_insert_bool(saving->file, plr->server.got_first_city,
                      "player%d.got_first_city", plrno);
  secfile_insert_int(saving->file, plr->revolution_finishes,
                     "player%d.revolution_finishes", plrno);

  // Player score
  secfile_insert_int(saving->file, plr->score.happy, "score%d.happy", plrno);
  secfile_insert_int(saving->file, plr->score.content, "score%d.content",
                     plrno);
  secfile_insert_int(saving->file, plr->score.unhappy, "score%d.unhappy",
                     plrno);
  secfile_insert_int(saving->file, plr->score.angry, "score%d.angry", plrno);
  specialist_type_iterate(sp)
  {
    secfile_insert_int(saving->file, plr->score.specialists[sp],
                       "score%d.specialists%d", plrno, sp);
  }
  specialist_type_iterate_end;
  secfile_insert_int(saving->file, plr->score.wonders, "score%d.wonders",
                     plrno);
  secfile_insert_int(saving->file, plr->score.techs, "score%d.techs", plrno);
  secfile_insert_int(saving->file, plr->score.techout, "score%d.techout",
                     plrno);
  secfile_insert_int(saving->file, plr->score.landarea, "score%d.landarea",
                     plrno);
  secfile_insert_int(saving->file, plr->score.settledarea,
                     "score%d.settledarea", plrno);
  secfile_insert_int(saving->file, plr->score.population,
                     "score%d.population", plrno);
  secfile_insert_int(saving->file, plr->score.cities, "score%d.cities",
                     plrno);
  secfile_insert_int(saving->file, plr->score.units, "score%d.units", plrno);
  secfile_insert_int(saving->file, plr->score.pollution, "score%d.pollution",
                     plrno);
  secfile_insert_int(saving->file, plr->score.literacy, "score%d.literacy",
                     plrno);
  secfile_insert_int(saving->file, plr->score.bnp, "score%d.bnp", plrno);
  secfile_insert_int(saving->file, plr->score.mfg, "score%d.mfg", plrno);
  secfile_insert_int(saving->file, plr->score.spaceship, "score%d.spaceship",
                     plrno);
  secfile_insert_int(saving->file, plr->score.units_built,
                     "score%d.units_built", plrno);
  secfile_insert_int(saving->file, plr->score.units_killed,
                     "score%d.units_killed", plrno);
  secfile_insert_int(saving->file, plr->score.units_lost,
                     "score%d.units_lost", plrno);
  secfile_insert_int(saving->file, plr->score.culture, "score%d.culture",
                     plrno);
  secfile_insert_int(saving->file, plr->score.game, "score%d.total", plrno);

  // Save space ship status.
  secfile_insert_int(saving->file, ship->state, "player%d.spaceship.state",
                     plrno);
  if (ship->state != SSHIP_NONE) {
    char buf[32];
    char st[NUM_SS_STRUCTURALS + 1];
    int ssi;

    fc_snprintf(buf, sizeof(buf), "player%d.spaceship", plrno);

    secfile_insert_int(saving->file, ship->structurals, "%s.structurals",
                       buf);
    secfile_insert_int(saving->file, ship->components, "%s.components", buf);
    secfile_insert_int(saving->file, ship->modules, "%s.modules", buf);
    secfile_insert_int(saving->file, ship->fuel, "%s.fuel", buf);
    secfile_insert_int(saving->file, ship->propulsion, "%s.propulsion", buf);
    secfile_insert_int(saving->file, ship->habitation, "%s.habitation", buf);
    secfile_insert_int(saving->file, ship->life_support, "%s.life_support",
                       buf);
    secfile_insert_int(saving->file, ship->solar_panels, "%s.solar_panels",
                       buf);

    for (ssi = 0; ssi < NUM_SS_STRUCTURALS; ssi++) {
      st[ssi] = BV_ISSET(ship->structure, ssi) ? '1' : '0';
    }
    st[ssi] = '\0';
    secfile_insert_str(saving->file, st, "%s.structure", buf);
    if (ship->state >= SSHIP_LAUNCHED) {
      secfile_insert_int(saving->file, ship->launch_year, "%s.launch_year",
                         buf);
    }
  }

  // Save lost wonders info.
  {
    char lost[B_LAST + 1];

    improvement_iterate(pimprove)
    {
      if (is_wonder(pimprove) && wonder_is_lost(plr, pimprove)) {
        lost[improvement_index(pimprove)] = '1';
      } else {
        lost[improvement_index(pimprove)] = '0';
      }
    }
    improvement_iterate_end;
    lost[improvement_count()] = '\0';
    secfile_insert_str(saving->file, lost, "player%d.lost_wonders", plrno);
  }

  secfile_insert_int(saving->file, plr->history, "player%d.history", plrno);
  secfile_insert_int(saving->file, plr->server.huts, "player%d.hut_count",
                     plrno);

  secfile_insert_bool(saving->file, plr->server.border_vision,
                      "player%d.border_vision", plrno);
}

/**
   Load city data
 */
static void sg_load_player_cities(struct loaddata *loading,
                                  struct player *plr)
{
  int ncities, i, plrno = player_number(plr);
  bool tasks_handled;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  sg_failure_ret(
      secfile_lookup_int(loading->file, &ncities, "player%d.ncities", plrno),
      "%s", secfile_error());

  if (!plr->is_alive && ncities > 0) {
    log_sg("'player%d.ncities' = %d for dead player!", plrno, ncities);
    ncities = 0;
  }

  if (!plr->server.got_first_city && ncities > 0) {
    // Probably barbarians in an old savegame; fix up
    plr->server.got_first_city = true;
  }

  // Load all cities of the player.
  for (i = 0; i < ncities; i++) {
    char buf[32];
    struct city *pcity;

    fc_snprintf(buf, sizeof(buf), "player%d.c%d", plrno, i);

    // Create a dummy city.
    pcity = create_city_virtual(plr, nullptr, buf);
    adv_city_alloc(pcity);
    if (!sg_load_player_city(loading, plr, pcity, buf)) {
      adv_city_free(pcity);
      destroy_city_virtual(pcity);
      sg_failure_ret(false, "Error loading city %d of player %d.", i, plrno);
    }

    identity_number_reserve(pcity->id);
    idex_register_city(&wld, pcity);

    /* Load the information about the nationality of citizens. This is done
     * here because the city sanity check called by citizens_update()
     * requires that the city is registered. */
    sg_load_player_city_citizens(loading, plr, pcity, buf);

    // After everything is loaded, but before vision.
    map_claim_ownership(city_tile(pcity), plr, city_tile(pcity), true);

    // adding the city contribution to fog-of-war
    pcity->server.vision = vision_new(plr, city_tile(pcity));
    vision_reveal_tiles(pcity->server.vision,
                        game.server.vision_reveal_tiles);
    city_refresh_vision(pcity);

    city_list_append(plr->cities, pcity);
  }

  tasks_handled = false;
  for (i = 0; !tasks_handled; i++) {
    int city_id;
    struct city *pcity = nullptr;

    city_id = secfile_lookup_int_default(loading->file, -1,
                                         "player%d.task%d.city", plrno, i);

    if (city_id != -1) {
      pcity = player_city_by_number(plr, city_id);
    }

    if (pcity != nullptr) {
      const char *str;
      int nat_x, nat_y;
      struct worker_task *ptask = new worker_task;

      nat_x = secfile_lookup_int_default(loading->file, -1,
                                         "player%d.task%d.x", plrno, i);
      nat_y = secfile_lookup_int_default(loading->file, -1,
                                         "player%d.task%d.y", plrno, i);

      ptask->ptile = native_pos_to_tile(&(wld.map), nat_x, nat_y);

      str = secfile_lookup_str(loading->file, "player%d.task%d.activity",
                               plrno, i);
      ptask->act = unit_activity_by_name(str, fc_strcasecmp);

      sg_failure_ret(unit_activity_is_valid(ptask->act),
                     "Unknown workertask activity %s", str);

      str = secfile_lookup_str(loading->file, "player%d.task%d.target",
                               plrno, i);

      if (strcmp("-", str)) {
        ptask->tgt = extra_type_by_rule_name(str);

        sg_failure_ret(ptask->tgt != nullptr, "Unknown workertask target %s",
                       str);
      } else {
        ptask->tgt = nullptr;

        if (ptask->act == ACTIVITY_IRRIGATE) {
          ptask->act = ACTIVITY_CULTIVATE;
        } else if (ptask->act == ACTIVITY_MINE) {
          ptask->act = ACTIVITY_MINE;
        }
      }

      ptask->want = secfile_lookup_int_default(
          loading->file, 1, "player%d.task%d.want", plrno, i);

      worker_task_list_append(pcity->task_reqs, ptask);
    } else {
      tasks_handled = true;
    }
  }
}

/**
   Load data for one city. sg_save_player_city() is not defined.
 */
static bool sg_load_player_city(struct loaddata *loading, struct player *plr,
                                struct city *pcity, const char *citystr)
{
  struct player *past;
  const char *kind, *name, *str;
  int id, i, repair, sp_count = 0, workers = 0, value;
  int nat_x, nat_y;
  citizens size;
  const char *stylename;
  int partner = 1;

  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_x, "%s.x", citystr),
                  false, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_y, "%s.y", citystr),
                  false, "%s", secfile_error());
  pcity->tile = native_pos_to_tile(&(wld.map), nat_x, nat_y);
  sg_warn_ret_val(nullptr != pcity->tile, false,
                  "%s has invalid center tile (%d, %d)", citystr, nat_x,
                  nat_y);
  sg_warn_ret_val(nullptr == tile_city(pcity->tile), false,
                  "%s duplicates city (%d, %d)", citystr, nat_x, nat_y);

  // Instead of dying, use 'citystr' string for damaged name.
  sz_strlcpy(pcity->name, secfile_lookup_str_default(loading->file, citystr,
                                                     "%s.name", citystr));

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &pcity->id, "%s.id", citystr), false,
      "%s", secfile_error());

  id = secfile_lookup_int_default(loading->file, player_number(plr),
                                  "%s.original", citystr);
  past = player_by_number(id);
  if (nullptr != past) {
    pcity->original = past;
  }

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &value, "%s.size", citystr), false,
      "%s", secfile_error());
  size = static_cast<citizens>(value); // set the correct type
  sg_warn_ret_val(value == (int) size, false,
                  "Invalid city size: %d, set to %d", value, size);
  city_size_set(pcity, size);

  for (i = 0; i < loading->specialist.size; i++) {
    sg_warn_ret_val(
        secfile_lookup_int(loading->file, &value, "%s.nspe%d", citystr, i),
        false, "%s", secfile_error());
    pcity->specialists[specialist_index(loading->specialist.order[i])] =
        static_cast<citizens>(value);
    sp_count += value;
  }

  for (i = 0; partner != 0; i++) {
    partner = secfile_lookup_int_default(loading->file, 0, "%s.traderoute%d",
                                         citystr, i);

    if (partner != 0) {
      struct trade_route *proute = new trade_route();
      const char *dir;
      const char *good_str;

      /* Append to routes list immediately, so the pointer can be found for
       * freeing even if we abort */
      trade_route_list_append(pcity->routes, proute);

      proute->partner = partner;
      dir = secfile_lookup_str(loading->file, "%s.route_direction%d",
                               citystr, i);
      sg_warn_ret_val(dir != nullptr, false,
                      "No traderoute direction found for %s", citystr);
      proute->dir = route_direction_by_name(dir, fc_strcasecmp);
      sg_warn_ret_val(route_direction_is_valid(proute->dir), false,
                      "Illegal route direction %s", dir);
      good_str =
          secfile_lookup_str(loading->file, "%s.route_good%d", citystr, i);
      sg_warn_ret_val(dir != nullptr, false, "No good found for %s",
                      citystr);
      proute->goods = goods_by_rule_name(good_str);
      sg_warn_ret_val(proute->goods != nullptr, false, "Illegal good %s",
                      good_str);
    }
  }

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->food_stock,
                                     "%s.food_stock", citystr),
                  false, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->shield_stock,
                                     "%s.shield_stock", citystr),
                  false, "%s", secfile_error());
  pcity->history =
      secfile_lookup_int_default(loading->file, 0, "%s.history", citystr);

  pcity->airlift =
      secfile_lookup_int_default(loading->file, 0, "%s.airlift", citystr);
  pcity->was_happy = secfile_lookup_bool_default(loading->file, false,
                                                 "%s.was_happy", citystr);

  pcity->turn_plague = secfile_lookup_int_default(loading->file, 0,
                                                  "%s.turn_plague", citystr);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->anarchy,
                                     "%s.anarchy", citystr),
                  false, "%s", secfile_error());
  pcity->rapture =
      secfile_lookup_int_default(loading->file, 0, "%s.rapture", citystr);
  pcity->steal =
      secfile_lookup_int_default(loading->file, 0, "%s.steal", citystr);

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->turn_founded,
                                     "%s.turn_founded", citystr),
                  false, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_bool(loading->file, &pcity->did_buy,
                                      "%s.did_buy", citystr),
                  false, "%s", secfile_error());
  // May not be present in older saves
  pcity->bought_shields = secfile_lookup_int_default(
      loading->file, 0, "%s.bought_shields", citystr);
  sg_warn_ret_val(secfile_lookup_bool(loading->file, &pcity->did_sell,
                                      "%s.did_sell", citystr),
                  false, "%s", secfile_error());

  sg_warn_ret_val(secfile_lookup_int(loading->file, &pcity->turn_last_built,
                                     "%s.turn_last_built", citystr),
                  false, "%s", secfile_error());

  kind = secfile_lookup_str(loading->file, "%s.currently_building_kind",
                            citystr);
  name = secfile_lookup_str(loading->file, "%s.currently_building_name",
                            citystr);
  pcity->production = universal_by_rule_name(kind, name);
  sg_warn_ret_val(pcity->production.kind != universals_n_invalid(), false,
                  "%s.currently_building: unknown \"%s\" \"%s\".", citystr,
                  kind, name);

  kind = secfile_lookup_str(loading->file, "%s.changed_from_kind", citystr);
  name = secfile_lookup_str(loading->file, "%s.changed_from_name", citystr);
  pcity->changed_from = universal_by_rule_name(kind, name);
  sg_warn_ret_val(pcity->changed_from.kind != universals_n_invalid(), false,
                  "%s.changed_from: unknown \"%s\" \"%s\".", citystr, kind,
                  name);

  pcity->before_change_shields =
      secfile_lookup_int_default(loading->file, pcity->shield_stock,
                                 "%s.before_change_shields", citystr);
  pcity->caravan_shields = secfile_lookup_int_default(
      loading->file, 0, "%s.caravan_shields", citystr);
  pcity->disbanded_shields = secfile_lookup_int_default(
      loading->file, 0, "%s.disbanded_shields", citystr);
  pcity->last_turns_shield_surplus = secfile_lookup_int_default(
      loading->file, 0, "%s.last_turns_shield_surplus", citystr);

  stylename = secfile_lookup_str_default(loading->file, nullptr, "%s.style",
                                         citystr);
  if (stylename != nullptr) {
    pcity->style = city_style_by_rule_name(stylename);
  } else {
    pcity->style = 0;
  }
  if (pcity->style < 0) {
    pcity->style = city_style(pcity);
  }

  pcity->server.synced = false; // must re-sync with clients

  // Initialise list of city improvements.
  for (i = 0; i < ARRAY_SIZE(pcity->built); i++) {
    pcity->built[i].turn = I_NEVER;
  }

  // Load city improvements.
  str = secfile_lookup_str(loading->file, "%s.improvements", citystr);
  sg_warn_ret_val(str != nullptr, false, "%s", secfile_error());
  sg_warn_ret_val(strlen(str) == loading->improvement.size, false,
                  "Invalid length of '%s.improvements' (%lu ~= %lu).",
                  citystr, (unsigned long) qstrlen(str),
                  (unsigned long) loading->improvement.size);
  for (i = 0; i < loading->improvement.size; i++) {
    sg_warn_ret_val(str[i] == '1' || str[i] == '0', false,
                    "Undefined value '%c' within '%s.improvements'.", str[i],
                    citystr)

        if (str[i] == '1')
    {
      struct impr_type *pimprove =
          improvement_by_rule_name(loading->improvement.order[i]);
      if (pimprove) {
        city_add_improvement(pcity, pimprove);
      }
    }
  }

  sg_failure_ret_val(loading->worked_tiles != nullptr, false,
                     "No worked tiles map defined.");

  city_freeze_workers(pcity);

  /* load new savegame with variable (squared) city radius and worked
   * tiles map */

  int radius_sq = secfile_lookup_int_default(loading->file, -1,
                                             "%s.city_radius_sq", citystr);
  city_map_radius_sq_set(pcity, radius_sq);

  city_tile_iterate(radius_sq, city_tile(pcity), ptile)
  {
    if (loading->worked_tiles[ptile->index] == pcity->id) {
      tile_set_worked(ptile, pcity);
      workers++;

#ifdef FREECIV_DEBUG
      /* set this tile to unused; a check for not resetted tiles is
       * included in game_load_internal() */
      loading->worked_tiles[ptile->index] = -1;
#endif // FREECIV_DEBUG
    }
  }
  city_tile_iterate_end;

  if (tile_worked(city_tile(pcity)) != pcity) {
    struct city *pwork = tile_worked(city_tile(pcity));

    if (nullptr != pwork) {
      log_sg("[%s] city center of '%s' (%d,%d) [%d] is worked by '%s' "
             "(%d,%d) [%d]; repairing ",
             citystr, city_name_get(pcity), TILE_XY(city_tile(pcity)),
             city_size_get(pcity), city_name_get(pwork),
             TILE_XY(city_tile(pwork)), city_size_get(pwork));

      tile_set_worked(city_tile(pcity), nullptr); // remove tile from pwork
      pwork->specialists[DEFAULT_SPECIALIST]++;
      auto_arrange_workers(pwork);
    } else {
      log_sg("[%s] city center of '%s' (%d,%d) [%d] is empty; repairing ",
             citystr, city_name_get(pcity), TILE_XY(city_tile(pcity)),
             city_size_get(pcity));
    }

    // repair pcity
    tile_set_worked(city_tile(pcity), pcity);
    city_repair_size(pcity, -1);
  }

  repair = city_size_get(pcity) - sp_count - (workers - FREE_WORKED_TILES);
  if (0 != repair) {
    log_sg("[%s] size mismatch for '%s' (%d,%d): size [%d] != "
           "(workers [%d] - free worked tiles [%d]) + specialists [%d]",
           citystr, city_name_get(pcity), TILE_XY(city_tile(pcity)),
           city_size_get(pcity), workers, FREE_WORKED_TILES, sp_count);

    // repair pcity
    city_repair_size(pcity, repair);
  }

  // worklist_init() done in create_city_virtual()
  worklist_load(loading->file, &pcity->worklist, "%s", citystr);

  // Load city options.
  BV_CLR_ALL(pcity->city_options);
  for (i = 0; i < CITYO_LAST; i++) {
    if (secfile_lookup_bool_default(loading->file, false, "%s.option%d",
                                    citystr, i)) {
      BV_SET(pcity->city_options, i);
    }
  }

  // Load the city rally point.
  {
    int len = secfile_lookup_int_default(loading->file, 0,
                                         "%s.rally_point_length", citystr);
    int unconverted;

    pcity->rally_point.length = len;
    if (len > 0) {
      const char *rally_orders, *rally_dirs, *rally_activities;

      pcity->rally_point.orders = new unit_order[len];
      pcity->rally_point.persistent = secfile_lookup_bool_default(
          loading->file, false, "%s.rally_point_persistent", citystr);
      pcity->rally_point.vigilant = secfile_lookup_bool_default(
          loading->file, false, "%s.rally_point_vigilant", citystr);

      rally_orders = secfile_lookup_str_default(
          loading->file, "", "%s.rally_point_orders", citystr);
      rally_dirs = secfile_lookup_str_default(
          loading->file, "", "%s.rally_point_dirs", citystr);
      rally_activities = secfile_lookup_str_default(
          loading->file, "", "%s.rally_point_activities", citystr);

      for (i = 0; i < len; i++) {
        struct unit_order *order = &pcity->rally_point.orders[i];

        if (rally_orders[i] == '\0' || rally_dirs[i] == '\0'
            || rally_activities[i] == '\0') {
          log_sg("Invalid rally point.");
          delete[] pcity->rally_point.orders;
          pcity->rally_point.orders = nullptr;
          pcity->rally_point.length = 0;
          break;
        }
        order->order = char2order(rally_orders[i]);
        order->dir = char2dir(rally_dirs[i]);
        order->activity = char2activity(rally_activities[i]);

        unconverted = secfile_lookup_int_default(
            loading->file, ACTION_NONE, "%s.rally_point_action_vec,%d",
            citystr, i);

        if (unconverted >= 0 && unconverted < loading->action.size) {
          // Look up what action id the unconverted number represents.
          order->action = loading->action.order[unconverted];
        } else {
          if (order->order == ORDER_PERFORM_ACTION) {
            log_sg("Invalid action id in order for city rally point %d",
                   pcity->id);
          }

          order->action = ACTION_NONE;
        }

        order->target = secfile_lookup_int_default(
            loading->file, NO_TARGET, "%s.rally_point_tgt_vec,%d", citystr,
            i);
        order->sub_target = secfile_lookup_int_default(
            loading->file, -1, "%s.rally_point_sub_tgt_vec,%d", citystr, i);
      }
    } else {
      pcity->rally_point.orders = nullptr;

      (void) secfile_entry_lookup(loading->file, "%s.rally_point_persistent",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.rally_point_vigilant",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.rally_point_orders",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.rally_point_dirs",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.rally_point_activities",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.rally_point_action_vec",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.rally_point_tgt_vec",
                                  citystr);
      (void) secfile_entry_lookup(loading->file,
                                  "%s.rally_point_sub_tgt_vec", citystr);
    }
  }

  // Load the city manager parameters.
  {
    bool enabled = secfile_lookup_bool_default(loading->file, false,
                                               "%s.cma_enabled", citystr);
    if (enabled) {
      struct cm_parameter *param = new cm_parameter();

      for (i = 0; i < O_LAST; i++) {
        param->minimal_surplus[i] = secfile_lookup_int_default(
            loading->file, 0, "%s.cma_minimal_surplus,%d", citystr, i);
        param->factor[i] = secfile_lookup_int_default(
            loading->file, 0, "%s.factor,%d", citystr, i);
      }
      param->max_growth = secfile_lookup_bool_default(
          loading->file, false, "%s.max_growth", citystr);
      param->require_happy = secfile_lookup_bool_default(
          loading->file, false, "%s.require_happy", citystr);
      param->allow_disorder = secfile_lookup_bool_default(
          loading->file, false, "%s.allow_disorder", citystr);
      param->allow_specialists = secfile_lookup_bool_default(
          loading->file, false, "%s.allow_specialists", citystr);
      param->happy_factor = secfile_lookup_int_default(
          loading->file, 0, "%s.happy_factor", citystr);
      pcity->cm_parameter = param;
    } else {
      pcity->cm_parameter = nullptr;
      for (i = 0; i < O_LAST; i++) {
        (void) secfile_entry_lookup(loading->file,
                                    "%s.cma_minimal_surplus,%d", citystr, i);
        (void) secfile_entry_lookup(loading->file, "%s.cma_factor,%d",
                                    citystr, i);
      }
      (void) secfile_entry_lookup(loading->file, "%s.cma_max_growth",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.cma_require_happy",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.cma_allow_disorder",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.cma_allow_specialists",
                                  citystr);
      (void) secfile_entry_lookup(loading->file, "%s.cma_factor", citystr);
      (void) secfile_entry_lookup(loading->file, "%s.happy_factor", citystr);
    }
  }
  CALL_FUNC_EACH_AI(city_load, loading->file, pcity, citystr);

  return true;
}

/**
   Load nationality data for one city.
 */
static void sg_load_player_city_citizens(struct loaddata *loading,
                                         struct player *plr,
                                         struct city *pcity,
                                         const char *citystr)
{
  if (game.info.citizen_nationality) {
    citizens size;

    citizens_init(pcity);
    player_slots_iterate(pslot)
    {
      int nationality;

      nationality =
          secfile_lookup_int_default(loading->file, -1, "%s.citizen%d",
                                     citystr, player_slot_index(pslot));
      if (nationality > 0 && !player_slot_is_used(pslot)) {
        log_sg("Citizens of an invalid nation for %s (player slot %d)!",
               city_name_get(pcity), player_slot_index(pslot));
        continue;
      }

      if (nationality != -1 && player_slot_is_used(pslot)) {
        sg_warn(nationality >= 0 && nationality <= MAX_CITY_SIZE,
                "Invalid value for citizens of player %d in %s: %d.",
                player_slot_index(pslot), city_name_get(pcity), nationality);
        citizens_nation_set(pcity, pslot, nationality);
      }
    }
    player_slots_iterate_end;
    // Sanity check.
    size = citizens_count(pcity);
    if (size != city_size_get(pcity)) {
      if (size != 0) {
        /* size == 0 can be result from the fact that ruleset had no
         * nationality enabled at saving time, so no citizens at all
         * were saved. But something more serious must be going on if
         * citizens have been saved partially - if some of them are there. */
        log_sg("City size and number of citizens does not match in %s "
               "(%d != %d)! Repairing ...",
               city_name_get(pcity), city_size_get(pcity), size);
      }
      citizens_update(pcity, nullptr);
    }
  }
}

/**
   Save cities data
 */
static void sg_save_player_cities(struct savedata *saving,
                                  struct player *plr)
{
  int wlist_max_length = 0, rally_point_max_length = 0;
  int i = 0;
  int plrno = player_number(plr);
  bool nations[MAX_NUM_PLAYER_SLOTS];

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  secfile_insert_int(saving->file, city_list_size(plr->cities),
                     "player%d.ncities", plrno);

  if (game.info.citizen_nationality) {
    // Initialise the nation list for the citizens information.
    player_slots_iterate(pslot)
    {
      nations[player_slot_index(pslot)] = false;
    }
    player_slots_iterate_end;
  }

  /* First determine length of longest worklist, rally point order, and the
   * nationalities we have. */
  city_list_iterate(plr->cities, pcity)
  {
    // Check the sanity of the city.
    city_refresh(pcity);
    sanity_check_city(pcity);

    if (pcity->worklist.length > wlist_max_length) {
      wlist_max_length = pcity->worklist.length;
    }

    if (pcity->rally_point.length > rally_point_max_length) {
      rally_point_max_length = pcity->rally_point.length;
    }

    if (game.info.citizen_nationality) {
      /* Find all nations of the citizens,*/
      players_iterate(pplayer)
      {
        if (!nations[player_index(pplayer)]
            && citizens_nation_get(pcity, pplayer->slot) != 0) {
          nations[player_index(pplayer)] = true;
        }
      }
      players_iterate_end;
    }
  }
  city_list_iterate_end;

  city_list_iterate(plr->cities, pcity)
  {
    struct tile *pcenter = city_tile(pcity);
    char impr_buf[B_LAST + 1];
    char buf[32];
    int j, nat_x, nat_y;

    fc_snprintf(buf, sizeof(buf), "player%d.c%d", plrno, i);

    index_to_native_pos(&nat_x, &nat_y, tile_index(pcenter));
    secfile_insert_int(saving->file, nat_y, "%s.y", buf);
    secfile_insert_int(saving->file, nat_x, "%s.x", buf);

    secfile_insert_int(saving->file, pcity->id, "%s.id", buf);

    secfile_insert_int(saving->file, player_number(pcity->original),
                       "%s.original", buf);
    secfile_insert_int(saving->file, city_size_get(pcity), "%s.size", buf);

    j = 0;
    specialist_type_iterate(sp)
    {
      secfile_insert_int(saving->file, pcity->specialists[sp], "%s.nspe%d",
                         buf, j++);
    }
    specialist_type_iterate_end;

    j = 0;
    trade_routes_iterate(pcity, proute)
    {
      secfile_insert_int(saving->file, proute->partner, "%s.traderoute%d",
                         buf, j);
      secfile_insert_str(saving->file, route_direction_name(proute->dir),
                         "%s.route_direction%d", buf, j);
      secfile_insert_str(saving->file, goods_rule_name(proute->goods),
                         "%s.route_good%d", buf, j);
      j++;
    }
    trade_routes_iterate_end;

    // Save dummy values to keep tabular format happy
    for (; j < MAX_TRADE_ROUTES; j++) {
      secfile_insert_int(saving->file, 0, "%s.traderoute%d", buf, j);
      secfile_insert_str(saving->file,
                         route_direction_name(RDIR_BIDIRECTIONAL),
                         "%s.route_direction%d", buf, j);
      secfile_insert_str(saving->file, goods_rule_name(goods_by_number(0)),
                         "%s.route_good%d", buf, j);
    }

    secfile_insert_int(saving->file, pcity->food_stock, "%s.food_stock",
                       buf);
    secfile_insert_int(saving->file, pcity->shield_stock, "%s.shield_stock",
                       buf);
    secfile_insert_int(saving->file, pcity->history, "%s.history", buf);

    secfile_insert_int(saving->file, pcity->airlift, "%s.airlift", buf);
    secfile_insert_bool(saving->file, pcity->was_happy, "%s.was_happy", buf);
    secfile_insert_int(saving->file, pcity->turn_plague, "%s.turn_plague",
                       buf);

    secfile_insert_int(saving->file, pcity->anarchy, "%s.anarchy", buf);
    secfile_insert_int(saving->file, pcity->rapture, "%s.rapture", buf);
    secfile_insert_int(saving->file, pcity->steal, "%s.steal", buf);
    secfile_insert_int(saving->file, pcity->turn_founded, "%s.turn_founded",
                       buf);
    secfile_insert_bool(saving->file, pcity->did_buy, "%s.did_buy", buf);
    secfile_insert_bool(saving->file, pcity->did_sell, "%s.did_sell", buf);
    secfile_insert_int(saving->file, pcity->turn_last_built,
                       "%s.turn_last_built", buf);

    // for visual debugging, variable length strings together here
    secfile_insert_str(saving->file, city_name_get(pcity), "%s.name", buf);

    secfile_insert_str(saving->file,
                       universal_type_rule_name(&pcity->production),
                       "%s.currently_building_kind", buf);
    secfile_insert_str(saving->file, universal_rule_name(&pcity->production),
                       "%s.currently_building_name", buf);

    secfile_insert_str(saving->file,
                       universal_type_rule_name(&pcity->changed_from),
                       "%s.changed_from_kind", buf);
    secfile_insert_str(saving->file,
                       universal_rule_name(&pcity->changed_from),
                       "%s.changed_from_name", buf);

    secfile_insert_int(saving->file, pcity->before_change_shields,
                       "%s.before_change_shields", buf);
    secfile_insert_int(saving->file, pcity->bought_shields,
                       "%s.bought_shields", buf);
    secfile_insert_int(saving->file, pcity->caravan_shields,
                       "%s.caravan_shields", buf);
    secfile_insert_int(saving->file, pcity->disbanded_shields,
                       "%s.disbanded_shields", buf);
    secfile_insert_int(saving->file, pcity->last_turns_shield_surplus,
                       "%s.last_turns_shield_surplus", buf);

    secfile_insert_str(saving->file, city_style_rule_name(pcity->style),
                       "%s.style", buf);

    /* Save the squared city radius and all tiles within the corresponing
     * city map. */
    secfile_insert_int(saving->file, pcity->city_radius_sq,
                       "player%d.c%d.city_radius_sq", plrno, i);
    /* The tiles worked by the city are saved using the main map.
     * See also sg_save_map_worked(). */

    /* Save improvement list as bytevector. Note that improvement order
     * is saved in savefile.improvement_order. */
    improvement_iterate(pimprove)
    {
      impr_buf[improvement_index(pimprove)] =
          (pcity->built[improvement_index(pimprove)].turn <= I_NEVER) ? '0'
                                                                      : '1';
    }
    improvement_iterate_end;
    impr_buf[improvement_count()] = '\0';
    sg_failure_ret(
        qstrlen(impr_buf) < sizeof(impr_buf),
        "Invalid size of the improvement vector (%s.improvements: "
        "%lu < %lu).",
        buf, (long unsigned int) qstrlen(impr_buf),
        (long unsigned int) sizeof(impr_buf));
    secfile_insert_str(saving->file, impr_buf, "%s.improvements", buf);

    worklist_save(saving->file, &pcity->worklist, wlist_max_length, "%s",
                  buf);

    for (j = 0; j < CITYO_LAST; j++) {
      secfile_insert_bool(saving->file, BV_ISSET(pcity->city_options, j),
                          "%s.option%d", buf, j);
    }

    CALL_FUNC_EACH_AI(city_save, saving->file, pcity, buf);

    if (game.info.citizen_nationality) {
      /* Save nationality of the citizens,*/
      players_iterate(pplayer)
      {
        if (nations[player_index(pplayer)]) {
          secfile_insert_int(saving->file,
                             citizens_nation_get(pcity, pplayer->slot),
                             "%s.citizen%d", buf, player_index(pplayer));
        }
      }
      players_iterate_end;
    }

    secfile_insert_int(saving->file, pcity->rally_point.length,
                       "%s.rally_point_length", buf);
    if (pcity->rally_point.length) {
      int len = pcity->rally_point.length;
      char orders[len + 1], dirs[len + 1], activities[len + 1];
      int actions[len];
      int targets[len];
      int sub_targets[len];

      secfile_insert_bool(saving->file, pcity->rally_point.persistent,
                          "%s.rally_point_persistent", buf);
      secfile_insert_bool(saving->file, pcity->rally_point.vigilant,
                          "%s.rally_point_vigilant", buf);

      for (j = 0; j < len; j++) {
        orders[j] = order2char(pcity->rally_point.orders[j].order);
        dirs[j] = '?';
        activities[j] = '?';
        targets[j] = NO_TARGET;
        sub_targets[j] = NO_TARGET;
        actions[j] = -1;
        switch (pcity->rally_point.orders[j].order) {
        case ORDER_MOVE:
        case ORDER_ACTION_MOVE:
          dirs[j] = dir2char(pcity->rally_point.orders[j].dir);
          break;
        case ORDER_ACTIVITY:
          sub_targets[j] = pcity->rally_point.orders[j].sub_target;
          activities[j] =
              activity2char(pcity->rally_point.orders[j].activity);
          break;
        case ORDER_PERFORM_ACTION:
          actions[j] = pcity->rally_point.orders[j].action;
          targets[j] = pcity->rally_point.orders[j].target;
          sub_targets[j] = pcity->rally_point.orders[j].sub_target;
          break;
        case ORDER_FULL_MP:
        case ORDER_LAST:
          break;
        }
      }
      orders[len] = dirs[len] = activities[len] = '\0';

      secfile_insert_str(saving->file, orders, "%s.rally_point_orders", buf);
      secfile_insert_str(saving->file, dirs, "%s.rally_point_dirs", buf);
      secfile_insert_str(saving->file, activities,
                         "%s.rally_point_activities", buf);

      secfile_insert_int_vec(saving->file, actions, len,
                             "%s.rally_point_action_vec", buf);
      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */
      for (j = len; j < rally_point_max_length; j++) {
        secfile_insert_int(saving->file, -1, "%s.rally_point_action_vec,%d",
                           buf, j);
      }

      secfile_insert_int_vec(saving->file, targets, len,
                             "%s.rally_point_tgt_vec", buf);
      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */
      for (j = len; j < rally_point_max_length; j++) {
        secfile_insert_int(saving->file, NO_TARGET,
                           "%s.rally_point_tgt_vec,%d", buf, j);
      }

      secfile_insert_int_vec(saving->file, sub_targets, len,
                             "%s.rally_point_sub_tgt_vec", buf);
      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */
      for (j = len; j < rally_point_max_length; j++) {
        secfile_insert_int(saving->file, -1, "%s.rally_point_sub_tgt_vec,%d",
                           buf, j);
      }
    } else {
      /* Put all the same fields into the savegame - otherwise the
       * registry code can't correctly use a tabular format and the
       * savegame will be bigger. */
      secfile_insert_bool(saving->file, false, "%s.rally_point_persistent",
                          buf);
      secfile_insert_bool(saving->file, false, "%s.rally_point_vigilant",
                          buf);
      secfile_insert_str(saving->file, "-", "%s.rally_point_orders", buf);
      secfile_insert_str(saving->file, "-", "%s.rally_point_dirs", buf);
      secfile_insert_str(saving->file, "-", "%s.rally_point_activities",
                         buf);

      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */

      // The start of a vector has no number.
      secfile_insert_int(saving->file, -1, "%s.rally_point_action_vec", buf);
      for (j = 1; j < rally_point_max_length; j++) {
        secfile_insert_int(saving->file, -1, "%s.rally_point_action_vec,%d",
                           buf, j);
      }

      // The start of a vector has no number.
      secfile_insert_int(saving->file, NO_TARGET, "%s.rally_point_tgt_vec",
                         buf);
      for (j = 1; j < rally_point_max_length; j++) {
        secfile_insert_int(saving->file, NO_TARGET,
                           "%s.rally_point_tgt_vec,%d", buf, j);
      }

      // The start of a vector has no number.
      secfile_insert_int(saving->file, -1, "%s.rally_point_sub_tgt_vec",
                         buf);
      for (j = 1; j < rally_point_max_length; j++) {
        secfile_insert_int(saving->file, -1, "%s.rally_point_sub_tgt_vec,%d",
                           buf, j);
      }
    }

    secfile_insert_bool(saving->file, pcity->cm_parameter != nullptr,
                        "%s.cma_enabled", buf);
    if (pcity->cm_parameter) {
      secfile_insert_int_vec(saving->file,
                             pcity->cm_parameter->minimal_surplus, O_LAST,
                             "%s.cma_minimal_surplus", buf);
      secfile_insert_int_vec(saving->file, pcity->cm_parameter->factor,
                             O_LAST, "%s.cma_factor", buf);
      secfile_insert_bool(saving->file, pcity->cm_parameter->max_growth,
                          "%s.max_growth", buf);
      secfile_insert_bool(saving->file, pcity->cm_parameter->require_happy,
                          "%s.require_happy", buf);
      secfile_insert_bool(saving->file, pcity->cm_parameter->allow_disorder,
                          "%s.allow_disorder", buf);
      secfile_insert_bool(saving->file,
                          pcity->cm_parameter->allow_specialists,
                          "%s.allow_specialists", buf);
      secfile_insert_int(saving->file, pcity->cm_parameter->happy_factor,
                         "%s.happy_factor", buf);
    } else {
      int zeros[O_LAST];

      memset(zeros, 0, sizeof(zeros));
      secfile_insert_int_vec(saving->file, zeros, O_LAST,
                             "%s.cma_minimal_surplus", buf);
      secfile_insert_int_vec(saving->file, zeros, O_LAST, "%s.cma_factor",
                             buf);
      secfile_insert_bool(saving->file, false, "%s.max_growth", buf);
      secfile_insert_bool(saving->file, false, "%s.require_happy", buf);
      secfile_insert_bool(saving->file, false, "%s.allow_disorder", buf);
      secfile_insert_bool(saving->file, false, "%s.allow_specialists", buf);
      secfile_insert_int(saving->file, 0, "%s.happy_factor", buf);
    }

    i++;
  }
  city_list_iterate_end;

  i = 0;
  city_list_iterate(plr->cities, pcity)
  {
    worker_task_list_iterate(pcity->task_reqs, ptask)
    {
      int nat_x, nat_y;

      index_to_native_pos(&nat_x, &nat_y, tile_index(ptask->ptile));
      secfile_insert_int(saving->file, pcity->id, "player%d.task%d.city",
                         plrno, i);
      secfile_insert_int(saving->file, nat_y, "player%d.task%d.y", plrno, i);
      secfile_insert_int(saving->file, nat_x, "player%d.task%d.x", plrno, i);
      secfile_insert_str(saving->file, unit_activity_name(ptask->act),
                         "player%d.task%d.activity", plrno, i);
      if (ptask->tgt != nullptr) {
        secfile_insert_str(saving->file, extra_rule_name(ptask->tgt),
                           "player%d.task%d.target", plrno, i);
      } else {
        secfile_insert_str(saving->file, "-", "player%d.task%d.target",
                           plrno, i);
      }
      secfile_insert_int(saving->file, ptask->want, "player%d.task%d.want",
                         plrno, i);

      i++;
    }
    worker_task_list_iterate_end;
  }
  city_list_iterate_end;
}

/**
   Load unit data
 */
static void sg_load_player_units(struct loaddata *loading,
                                 struct player *plr)
{
  int nunits, i, plrno = player_number(plr);

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  sg_failure_ret(
      secfile_lookup_int(loading->file, &nunits, "player%d.nunits", plrno),
      "%s", secfile_error());
  if (!plr->is_alive && nunits > 0) {
    log_sg("'player%d.nunits' = %d for dead player!", plrno, nunits);
    nunits = 0; // Some old savegames may be buggy.
  }

  for (i = 0; i < nunits; i++) {
    struct unit *punit;
    struct city *pcity;
    const char *name;
    char buf[32];
    struct unit_type *type;
    struct tile *ptile;

    fc_snprintf(buf, sizeof(buf), "player%d.u%d", plrno, i);

    name = secfile_lookup_str(loading->file, "%s.type_by_name", buf);
    type = unit_type_by_rule_name(name);
    sg_failure_ret(type != nullptr, "%s: unknown unit type \"%s\".", buf,
                   name);

    // Create a dummy unit.
    punit = unit_virtual_create(plr, nullptr, type, 0);
    if (!sg_load_player_unit(loading, plr, punit, buf)) {
      unit_virtual_destroy(punit);
      sg_failure_ret(false, "Error loading unit %d of player %d.", i, plrno);
    }

    identity_number_reserve(punit->id);
    idex_register_unit(&wld, punit);

    if ((pcity = game_city_by_number(punit->homecity))) {
      unit_list_prepend(pcity->units_supported, punit);
    } else if (punit->homecity > IDENTITY_NUMBER_ZERO) {
      log_sg("%s: bad home city %d.", buf, punit->homecity);
      punit->homecity = IDENTITY_NUMBER_ZERO;
    }

    ptile = unit_tile(punit);

    // allocate the unit's contribution to fog of war
    punit->server.vision = vision_new(unit_owner(punit), ptile);
    unit_refresh_vision(punit);
    /* NOTE: There used to be some map_set_known calls here.  These were
     * unneeded since unfogging the tile when the unit sees it will
     * automatically reveal that tile. */

    unit_list_append(plr->units, punit);
    unit_list_prepend(unit_tile(punit)->units, punit);

    // Claim ownership of fortress?
    if ((extra_owner(ptile) == nullptr
         || pplayers_at_war(extra_owner(ptile), plr))
        && tile_has_claimable_base(ptile, unit_type_get(punit))) {
      tile_claim_bases(ptile, plr);
    }
  }
}

/**
   Load one unit. sg_save_player_unit() is not defined.
 */
static bool sg_load_player_unit(struct loaddata *loading, struct player *plr,
                                struct unit *punit, const char *unitstr)
{
  int j;
  enum unit_activity activity;
  int nat_x, nat_y;
  struct extra_type *pextra = nullptr;
  struct tile *ptile;
  int extra_id;
  int ei;
  const char *facing_str;
  enum tile_special_type cfspe;
  int natnbr;
  int unconverted;
  const char *str;

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &punit->id, "%s.id", unitstr), false,
      "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_x, "%s.x", unitstr),
                  false, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_y, "%s.y", unitstr),
                  false, "%s", secfile_error());

  ptile = native_pos_to_tile(&(wld.map), nat_x, nat_y);
  sg_warn_ret_val(nullptr != ptile, false, "%s invalid tile (%d, %d)",
                  unitstr, nat_x, nat_y);
  unit_tile_set(punit, ptile);

  facing_str =
      secfile_lookup_str_default(loading->file, "x", "%s.facing", unitstr);
  if (facing_str[0] != 'x') {
    /* We don't touch punit->facing if savegame does not contain that
     * information. Initial orientation set by unit_virtual_create()
     * is as good as any. */
    enum direction8 facing = char2dir(facing_str[0]);

    if (direction8_is_valid(facing)) {
      punit->facing = facing;
    } else {
      qCritical("Illegal unit orientation '%s'", facing_str);
    }
  }

  /* If savegame has unit nationality, it doesn't hurt to
   * internally set it even if nationality rules are disabled. */
  natnbr = secfile_lookup_int_default(loading->file, player_number(plr),
                                      "%s.nationality", unitstr);

  punit->nationality = player_by_number(natnbr);
  if (punit->nationality == nullptr) {
    punit->nationality = plr;
  }

  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->homecity,
                                     "%s.homecity", unitstr),
                  false, "%s", secfile_error());

  if (auto name = secfile_lookup_str(loading->file, "%s.name", unitstr)) {
    // Support earlier saves
    punit->name = QString::fromUtf8(name);
  }

  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->moves_left,
                                     "%s.moves", unitstr),
                  false, "%s", secfile_error());
  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &punit->fuel, "%s.fuel", unitstr),
      false, "%s", secfile_error());
  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &ei, "%s.activity", unitstr), false,
      "%s", secfile_error());
  activity =
      unit_activity_by_name(loading->activities.order[ei], fc_strcasecmp);

  punit->server.birth_turn = secfile_lookup_int_default(
      loading->file, game.info.turn, "%s.born", unitstr);

  if (activity == ACTIVITY_PATROL_UNUSED) {
    /* Previously ACTIVITY_PATROL and ACTIVITY_GOTO were used for
     * client-side goto. Now client-side goto is handled by setting
     * a special flag, and units with orders generally have ACTIVITY_IDLE.
     * Old orders are lost. Old client-side goto units will still have
     * ACTIVITY_GOTO and will goto the correct position via server goto.
     * Old client-side patrol units lose their patrol routes and are put
     * into idle mode. */
    activity = ACTIVITY_IDLE;
  }

  extra_id = secfile_lookup_int_default(loading->file, -2, "%s.activity_tgt",
                                        unitstr);

  if (extra_id != -2) {
    if (extra_id >= 0 && extra_id < loading->extra.size) {
      pextra = loading->extra.order[extra_id];
      set_unit_activity_targeted(punit, activity, pextra);
    } else if (activity == ACTIVITY_IRRIGATE) {
      struct extra_type *tgt = next_extra_for_tile(
          unit_tile(punit), EC_IRRIGATION, unit_owner(punit), punit);
      if (tgt != nullptr) {
        set_unit_activity_targeted(punit, ACTIVITY_IRRIGATE, tgt);
      } else {
        set_unit_activity(punit, ACTIVITY_CULTIVATE);
      }
    } else if (activity == ACTIVITY_MINE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit), EC_MINE,
                                                   unit_owner(punit), punit);
      if (tgt != nullptr) {
        set_unit_activity_targeted(punit, ACTIVITY_MINE, tgt);
      } else {
        set_unit_activity(punit, ACTIVITY_PLANT);
      }
    } else {
      set_unit_activity(punit, activity);
    }
  } else {
    set_unit_activity_targeted(punit, activity, nullptr);
  } // activity_tgt == nullptr

  sg_warn_ret_val(secfile_lookup_int(loading->file, &punit->activity_count,
                                     "%s.activity_count", unitstr),
                  false, "%s", secfile_error());

  punit->changed_from =
      static_cast<unit_activity>(secfile_lookup_int_default(
          loading->file, ACTIVITY_IDLE, "%s.changed_from", unitstr));

  extra_id = secfile_lookup_int_default(loading->file, -2,
                                        "%s.changed_from_tgt", unitstr);

  if (extra_id != -2) {
    if (extra_id >= 0 && extra_id < loading->extra.size) {
      punit->changed_from_target = loading->extra.order[extra_id];
    } else {
      punit->changed_from_target = nullptr;
    }
  } else {
    // extra_id == -2 -> changed_from_tgt not set

    cfspe = static_cast<tile_special_type>(secfile_lookup_int_default(
        loading->file, S_LAST, "%s.changed_from_target", unitstr));

    if (cfspe != S_LAST) {
      punit->changed_from_target = special_extra_get(cfspe);
    } else {
      punit->changed_from_target = nullptr;
    }

    if (punit->changed_from == ACTIVITY_IRRIGATE) {
      struct extra_type *tgt = next_extra_for_tile(
          unit_tile(punit), EC_IRRIGATION, unit_owner(punit), punit);
      if (tgt != nullptr) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = nullptr;
      }
    } else if (punit->changed_from == ACTIVITY_MINE) {
      struct extra_type *tgt = next_extra_for_tile(unit_tile(punit), EC_MINE,
                                                   unit_owner(punit), punit);
      if (tgt != nullptr) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = nullptr;
      }
    } else if (punit->changed_from == ACTIVITY_POLLUTION) {
      struct extra_type *tgt = prev_extra_in_tile(
          unit_tile(punit), ERM_CLEANPOLLUTION, unit_owner(punit), punit);
      if (tgt != nullptr) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = nullptr;
      }
    } else if (punit->changed_from == ACTIVITY_FALLOUT) {
      struct extra_type *tgt = prev_extra_in_tile(
          unit_tile(punit), ERM_CLEANFALLOUT, unit_owner(punit), punit);
      if (tgt != nullptr) {
        punit->changed_from_target = tgt;
      } else {
        punit->changed_from_target = nullptr;
      }
    }
  }

  punit->changed_from_count = secfile_lookup_int_default(
      loading->file, 0, "%s.changed_from_count", unitstr);

  /* Special case: for a long time, we accidentally incremented
   * activity_count while a unit was sentried, so it could increase
   * without bound (bug #20641) and be saved in old savefiles.
   * We zero it to prevent potential trouble overflowing the range
   * in network packets, etc. */
  if (activity == ACTIVITY_SENTRY) {
    punit->activity_count = 0;
  }
  if (punit->changed_from == ACTIVITY_SENTRY) {
    punit->changed_from_count = 0;
  }

  punit->veteran =
      secfile_lookup_int_default(loading->file, 0, "%s.veteran", unitstr);
  {
    // Protect against change in veteran system in ruleset
    const int levels = utype_veteran_levels(unit_type_get(punit));

    if (punit->veteran >= levels) {
      fc_assert(levels >= 1);
      punit->veteran = levels - 1;
    }
  }
  punit->done_moving = secfile_lookup_bool_default(
      loading->file, (punit->moves_left == 0), "%s.done_moving", unitstr);
  punit->battlegroup = secfile_lookup_int_default(
      loading->file, BATTLEGROUP_NONE, "%s.battlegroup", unitstr);

  if (secfile_lookup_bool_default(loading->file, false, "%s.go", unitstr)) {
    int gnat_x, gnat_y;

    sg_warn_ret_val(
        secfile_lookup_int(loading->file, &gnat_x, "%s.goto_x", unitstr),
        false, "%s", secfile_error());
    sg_warn_ret_val(
        secfile_lookup_int(loading->file, &gnat_y, "%s.goto_y", unitstr),
        false, "%s", secfile_error());

    punit->goto_tile = native_pos_to_tile(&(wld.map), gnat_x, gnat_y);
  } else {
    punit->goto_tile = nullptr;

    /* These variables are not used but needed for saving the unit table.
     * Load them to prevent unused variables errors. */
    (void) secfile_entry_lookup(loading->file, "%s.goto_x", unitstr);
    (void) secfile_entry_lookup(loading->file, "%s.goto_y", unitstr);
  }

  // Load AI data of the unit.
  CALL_FUNC_EACH_AI(unit_load, loading->file, punit, unitstr);

  unconverted = secfile_lookup_int_default(loading->file, 0,
                                           "%s.server_side_agent", unitstr);
  if (unconverted >= 0 && unconverted < loading->ssa.size) {
    // Look up what server side agent the unconverted number represents.
    punit->ssa_controller = loading->ssa.order[unconverted];
  } else {
    log_sg("Invalid server side agent %d for unit %d", unconverted,
           punit->id);

    punit->ssa_controller = SSA_NONE;
  }

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &punit->hp, "%s.hp", unitstr), false,
      "%s", secfile_error());

  punit->server.ord_map =
      secfile_lookup_int_default(loading->file, 0, "%s.ord_map", unitstr);
  punit->server.ord_city =
      secfile_lookup_int_default(loading->file, 0, "%s.ord_city", unitstr);
  punit->moved =
      secfile_lookup_bool_default(loading->file, false, "%s.moved", unitstr);
  punit->paradropped = secfile_lookup_bool_default(
      loading->file, false, "%s.paradropped", unitstr);
  str =
      secfile_lookup_str_default(loading->file, "", "%s.carrying", unitstr);
  if (str[0] != '\0') {
    punit->carrying = goods_by_rule_name(str);
  }

  /* The transport status (punit->transported_by) is loaded in
   * sg_player_units_transport(). */

  /* Initialize upkeep values: these are hopefully initialized
   * elsewhere before use (specifically, in city_support(); but
   * fixme: check whether always correctly initialized?).
   * Below is mainly for units which don't have homecity --
   * otherwise these don't get initialized (and AI calculations
   * etc may use junk values). */
  output_type_iterate(o)
  {
    punit->upkeep[o] = utype_upkeep_cost(unit_type_get(punit), plr,
                                         static_cast<Output_type_id>(o));
  }
  output_type_iterate_end;

  sg_warn_ret_val(secfile_lookup_int(loading->file, &unconverted,
                                     "%s.action_decision", unitstr),
                  false, "%s", secfile_error());

  if (unconverted >= 0 && unconverted < loading->act_dec.size) {
    /* Look up what action decision want the unconverted number
     * represents. */
    punit->action_decision_want = loading->act_dec.order[unconverted];
  } else {
    log_sg("Invalid action decision want for unit %d", punit->id);

    punit->action_decision_want = ACT_DEC_NOTHING;
  }

  if (punit->action_decision_want != ACT_DEC_NOTHING) {
    // Load the tile to act against.
    int adwt_x, adwt_y;

    if (secfile_lookup_int(loading->file, &adwt_x,
                           "%s.action_decision_tile_x", unitstr)
        && secfile_lookup_int(loading->file, &adwt_y,
                              "%s.action_decision_tile_y", unitstr)) {
      punit->action_decision_tile =
          native_pos_to_tile(&(wld.map), adwt_x, adwt_y);
    } else {
      punit->action_decision_want = ACT_DEC_NOTHING;
      punit->action_decision_tile = nullptr;
      log_sg("Bad action_decision_tile for unit %d", punit->id);
    }
  } else {
    (void) secfile_entry_lookup(loading->file, "%s.action_decision_tile_x",
                                unitstr);
    (void) secfile_entry_lookup(loading->file, "%s.action_decision_tile_y",
                                unitstr);
    punit->action_decision_tile = nullptr;
  }

  punit->stay =
      secfile_lookup_bool_default(loading->file, false, "%s.stay", unitstr);

  // load the unit orders
  {
    int len = secfile_lookup_int_default(loading->file, 0,
                                         "%s.orders_length", unitstr);
    if (len > 0) {
      const char *orders_unitstr, *dir_unitstr, *act_unitstr;

      punit->orders.list = new unit_order[len];
      punit->orders.length = len;
      punit->orders.index = secfile_lookup_int_default(
          loading->file, 0, "%s.orders_index", unitstr);
      punit->orders.repeat = secfile_lookup_bool_default(
          loading->file, false, "%s.orders_repeat", unitstr);
      punit->orders.vigilant = secfile_lookup_bool_default(
          loading->file, false, "%s.orders_vigilant", unitstr);

      orders_unitstr = secfile_lookup_str_default(loading->file, "",
                                                  "%s.orders_list", unitstr);
      dir_unitstr = secfile_lookup_str_default(loading->file, "",
                                               "%s.dir_list", unitstr);
      act_unitstr = secfile_lookup_str_default(loading->file, "",
                                               "%s.activity_list", unitstr);

      punit->has_orders = true;
      for (j = 0; j < len; j++) {
        struct unit_order *order = &punit->orders.list[j];
        bool action_wants_extra = false;
        int order_sub_tgt;

        if (orders_unitstr[j] == '\0' || dir_unitstr[j] == '\0'
            || act_unitstr[j] == '\0') {
          log_sg("Invalid unit orders.");
          free_unit_orders(punit);
          break;
        }
        order->order = char2order(orders_unitstr[j]);
        order->dir = char2dir(dir_unitstr[j]);
        order->activity = char2activity(act_unitstr[j]);

        unconverted = secfile_lookup_int_default(
            loading->file, ACTION_NONE, "%s.action_vec,%d", unitstr, j);

        if (unconverted >= 0 && unconverted < loading->action.size) {
          // Look up what action id the unconverted number represents.
          order->action = loading->action.order[unconverted];
        } else {
          if (order->order == ORDER_PERFORM_ACTION) {
            log_sg("Invalid action id in order for unit %d", punit->id);
          }

          order->action = ACTION_NONE;
        }

        if (order->order == ORDER_LAST
            || (order->order == ORDER_MOVE
                && !direction8_is_valid(order->dir))
            || (order->order == ORDER_ACTION_MOVE
                && !direction8_is_valid(order->dir))
            || (order->order == ORDER_PERFORM_ACTION
                && !action_id_exists(order->action))
            || (order->order == ORDER_ACTIVITY
                && order->activity == ACTIVITY_LAST)) {
          // An invalid order. Just drop the orders for this unit.
          delete[] punit->orders.list;
          punit->orders.list = nullptr;
          punit->has_orders = false;
          break;
        }

        order->target = secfile_lookup_int_default(
            loading->file, NO_TARGET, "%s.tgt_vec,%d", unitstr, j);
        order_sub_tgt = secfile_lookup_int_default(
            loading->file, -1, "%s.sub_tgt_vec,%d", unitstr, j);

        if (order->order == ORDER_PERFORM_ACTION) {
          // Validate sub target
          switch (action_id_get_sub_target_kind(order->action)) {
          case ASTK_BUILDING:
            // Sub target is a building.
            if (!improvement_by_number(order_sub_tgt)) {
              // Sub target is invalid.
              log_sg(
                  "Cannot find building %d for %s to %s", order_sub_tgt,
                  unit_rule_name(punit),
                  qUtf8Printable(action_id_name_translation(order->action)));
              order->sub_target = B_LAST;
            } else {
              order->sub_target = order_sub_tgt;
            }
            break;
          case ASTK_TECH:
            // Sub target is a technology.
            if (order_sub_tgt == A_NONE
                || (!valid_advance_by_number(order_sub_tgt)
                    && order_sub_tgt != A_FUTURE)) {
              // Target tech is invalid.
              log_sg("Cannot find tech %d for %s to steal", order_sub_tgt,
                     unit_rule_name(punit));
              order->sub_target = A_NONE;
            } else {
              order->sub_target = order_sub_tgt;
            }
            break;
          case ASTK_EXTRA:
          case ASTK_EXTRA_NOT_THERE:
            // These take an extra.
            action_wants_extra = true;
            break;
          case ASTK_NONE:
            // None of these can take a sub target.
            fc_assert_msg(order_sub_tgt == -1,
                          "Specified sub target for action %d unsupported.",
                          order->action);
            order->sub_target = NO_TARGET;
            break;
          case ASTK_COUNT:
            fc_assert_msg(order_sub_tgt == -1, "Bad action action %d.",
                          order->action);
            order->sub_target = NO_TARGET;
            break;
          }
        }
        if (order->order == ORDER_ACTIVITY || action_wants_extra) {
          enum unit_activity act;

          if (order_sub_tgt < 0 || order_sub_tgt >= loading->extra.size) {
            if (order_sub_tgt != EXTRA_NONE) {
              log_sg("Cannot find extra %d for %s to build", order_sub_tgt,
                     unit_rule_name(punit));
            }

            order->sub_target = EXTRA_NONE;
          } else {
            order->sub_target = order_sub_tgt;
          }

          // An action or an activity may require an extra target.
          if (action_wants_extra) {
            act = action_id_get_activity(order->action);
          } else {
            act = order->activity;
          }

          if (unit_activity_is_valid(act)
              && unit_activity_needs_target_from_client(act)
              && order->sub_target == EXTRA_NONE) {
            // Missing required action extra target.
            delete[] punit->orders.list;
            punit->orders.list = nullptr;
            punit->has_orders = false;
            break;
          }
        } else if (order->order != ORDER_PERFORM_ACTION) {
          if (order_sub_tgt != -1) {
            log_sg(
                "Unexpected sub_target %d (expected %d) for order type %d",
                order_sub_tgt, -1, order->order);
          }
          order->sub_target = NO_TARGET;
        }
      }
    } else {
      punit->has_orders = false;
      punit->orders.list = nullptr;

      (void) secfile_entry_lookup(loading->file, "%s.orders_index", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.orders_repeat",
                                  unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.orders_vigilant",
                                  unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.orders_list", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.dir_list", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.activity_list",
                                  unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.tgt_vec", unitstr);
      (void) secfile_entry_lookup(loading->file, "%s.sub_tgt_vec", unitstr);
    }
  }

  return true;
}

/**
   Load the transport status of all units. This is seperated from the other
   code as all units must be known.
 */
static void sg_load_player_units_transport(struct loaddata *loading,
                                           struct player *plr)
{
  int nunits, i, plrno = player_number(plr);

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  /* Recheck the number of units for the player. This is a copied from
   * sg_load_player_units(). */
  sg_failure_ret(
      secfile_lookup_int(loading->file, &nunits, "player%d.nunits", plrno),
      "%s", secfile_error());
  if (!plr->is_alive && nunits > 0) {
    log_sg("'player%d.nunits' = %d for dead player!", plrno, nunits);
    nunits = 0; // Some old savegames may be buggy.
  }

  for (i = 0; i < nunits; i++) {
    int id_unit, id_trans;
    struct unit *punit, *ptrans;

    id_unit = secfile_lookup_int_default(loading->file, -1,
                                         "player%d.u%d.id", plrno, i);
    punit = player_unit_by_number(plr, id_unit);
    fc_assert_action(punit != nullptr, continue);

    id_trans = secfile_lookup_int_default(
        loading->file, -1, "player%d.u%d.transported_by", plrno, i);
    if (id_trans == -1) {
      // Not transported.
      continue;
    }

    ptrans = game_unit_by_number(id_trans);
    fc_assert_action(id_trans == -1 || ptrans != nullptr, continue);

    if (ptrans) {
      bool load_success = unit_transport_load(punit, ptrans, true);

      fc_assert_action(load_success == true, continue);
    }
  }
}

/**
   Save unit data
 */
static void sg_save_player_units(struct savedata *saving, struct player *plr)
{
  int i = 0;
  int longest_order = 0;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  secfile_insert_int(saving->file, unit_list_size(plr->units),
                     "player%d.nunits", player_number(plr));

  /* Find the longest unit order so different order length won't break
   * storing units in the tabular format. */
  unit_list_iterate(plr->units, punit)
  {
    if (punit->has_orders) {
      if (longest_order < punit->orders.length) {
        longest_order = punit->orders.length;
      }
    }
  }
  unit_list_iterate_end;

  unit_list_iterate(plr->units, punit)
  {
    char buf[32];
    char dirbuf[2] = " ";
    int nat_x, nat_y;
    int last_order, j;

    fc_snprintf(buf, sizeof(buf), "player%d.u%d", player_number(plr), i);
    dirbuf[0] = dir2char(punit->facing);
    secfile_insert_int(saving->file, punit->id, "%s.id", buf);

    index_to_native_pos(&nat_x, &nat_y, tile_index(unit_tile(punit)));
    secfile_insert_int(saving->file, nat_x, "%s.x", buf);
    secfile_insert_int(saving->file, nat_y, "%s.y", buf);

    secfile_insert_str(saving->file, dirbuf, "%s.facing", buf);
    if (game.info.citizen_nationality) {
      secfile_insert_int(saving->file,
                         player_number(unit_nationality(punit)),
                         "%s.nationality", buf);
    }
    secfile_insert_int(saving->file, punit->veteran, "%s.veteran", buf);
    secfile_insert_int(saving->file, punit->hp, "%s.hp", buf);
    secfile_insert_int(saving->file, punit->homecity, "%s.homecity", buf);
    secfile_insert_str(saving->file, punit->name.toUtf8(), "%s.name", buf);

    secfile_insert_str(saving->file, unit_rule_name(punit),
                       "%s.type_by_name", buf);

    secfile_insert_int(saving->file, punit->activity, "%s.activity", buf);
    secfile_insert_int(saving->file, punit->activity_count,
                       "%s.activity_count", buf);
    if (punit->activity_target == nullptr) {
      secfile_insert_int(saving->file, -1, "%s.activity_tgt", buf);
    } else {
      secfile_insert_int(saving->file, extra_index(punit->activity_target),
                         "%s.activity_tgt", buf);
    }

    secfile_insert_int(saving->file, punit->changed_from, "%s.changed_from",
                       buf);
    secfile_insert_int(saving->file, punit->changed_from_count,
                       "%s.changed_from_count", buf);
    if (punit->changed_from_target == nullptr) {
      secfile_insert_int(saving->file, -1, "%s.changed_from_tgt", buf);
    } else {
      secfile_insert_int(saving->file,
                         extra_index(punit->changed_from_target),
                         "%s.changed_from_tgt", buf);
    }

    secfile_insert_bool(saving->file, punit->done_moving, "%s.done_moving",
                        buf);
    secfile_insert_int(saving->file, punit->moves_left, "%s.moves", buf);
    secfile_insert_int(saving->file, punit->fuel, "%s.fuel", buf);
    secfile_insert_int(saving->file, punit->server.birth_turn, "%s.born",
                       buf);
    secfile_insert_int(saving->file, punit->battlegroup, "%s.battlegroup",
                       buf);

    if (punit->goto_tile) {
      index_to_native_pos(&nat_x, &nat_y, tile_index(punit->goto_tile));
      secfile_insert_bool(saving->file, true, "%s.go", buf);
      secfile_insert_int(saving->file, nat_x, "%s.goto_x", buf);
      secfile_insert_int(saving->file, nat_y, "%s.goto_y", buf);
    } else {
      secfile_insert_bool(saving->file, false, "%s.go", buf);
      // Set this values to allow saving it as table.
      secfile_insert_int(saving->file, 0, "%s.goto_x", buf);
      secfile_insert_int(saving->file, 0, "%s.goto_y", buf);
    }

    secfile_insert_int(saving->file, punit->ssa_controller,
                       "%s.server_side_agent", buf);

    // Save AI data of the unit.
    CALL_FUNC_EACH_AI(unit_save, saving->file, punit, buf);

    secfile_insert_int(saving->file, punit->server.ord_map, "%s.ord_map",
                       buf);
    secfile_insert_int(saving->file, punit->server.ord_city, "%s.ord_city",
                       buf);
    secfile_insert_bool(saving->file, punit->moved, "%s.moved", buf);
    secfile_insert_bool(saving->file, punit->paradropped, "%s.paradropped",
                        buf);
    secfile_insert_int(
        saving->file,
        unit_transport_get(punit) ? unit_transport_get(punit)->id : -1,
        "%s.transported_by", buf);
    if (punit->carrying != nullptr) {
      secfile_insert_str(saving->file, goods_rule_name(punit->carrying),
                         "%s.carrying", buf);
    } else {
      secfile_insert_str(saving->file, "", "%s.carrying", buf);
    }

    secfile_insert_int(saving->file, punit->action_decision_want,
                       "%s.action_decision", buf);

    /* Stored as tile rather than direction to make sure the target tile is
     * sane. */
    if (punit->action_decision_tile) {
      index_to_native_pos(&nat_x, &nat_y,
                          tile_index(punit->action_decision_tile));
      secfile_insert_int(saving->file, nat_x, "%s.action_decision_tile_x",
                         buf);
      secfile_insert_int(saving->file, nat_y, "%s.action_decision_tile_y",
                         buf);
    } else {
      // Dummy values to get tabular format.
      secfile_insert_int(saving->file, -1, "%s.action_decision_tile_x", buf);
      secfile_insert_int(saving->file, -1, "%s.action_decision_tile_y", buf);
    }

    secfile_insert_bool(saving->file, punit->stay, "%s.stay", buf);

    if (punit->has_orders) {
      int len = punit->orders.length;
      char orders_buf[len + 1], dir_buf[len + 1];
      char act_buf[len + 1];
      int action_buf[len];
      int tgt_vec[len];
      int sub_tgt_vec[len];

      last_order = len;

      secfile_insert_int(saving->file, len, "%s.orders_length", buf);
      secfile_insert_int(saving->file, punit->orders.index,
                         "%s.orders_index", buf);
      secfile_insert_bool(saving->file, punit->orders.repeat,
                          "%s.orders_repeat", buf);
      secfile_insert_bool(saving->file, punit->orders.vigilant,
                          "%s.orders_vigilant", buf);

      for (j = 0; j < len; j++) {
        orders_buf[j] = order2char(punit->orders.list[j].order);
        dir_buf[j] = '?';
        act_buf[j] = '?';
        tgt_vec[j] = NO_TARGET;
        sub_tgt_vec[j] = -1;
        action_buf[j] = -1;
        switch (punit->orders.list[j].order) {
        case ORDER_MOVE:
        case ORDER_ACTION_MOVE:
          dir_buf[j] = dir2char(punit->orders.list[j].dir);
          break;
        case ORDER_ACTIVITY:
          sub_tgt_vec[j] = punit->orders.list[j].sub_target;
          act_buf[j] = activity2char(punit->orders.list[j].activity);
          break;
        case ORDER_PERFORM_ACTION:
          action_buf[j] = punit->orders.list[j].action;
          tgt_vec[j] = punit->orders.list[j].target;
          sub_tgt_vec[j] = punit->orders.list[j].sub_target;
          break;
        case ORDER_FULL_MP:
        case ORDER_LAST:
          break;
        }
      }
      orders_buf[len] = dir_buf[len] = act_buf[len] = '\0';

      secfile_insert_str(saving->file, orders_buf, "%s.orders_list", buf);
      secfile_insert_str(saving->file, dir_buf, "%s.dir_list", buf);
      secfile_insert_str(saving->file, act_buf, "%s.activity_list", buf);

      secfile_insert_int_vec(saving->file, action_buf, len, "%s.action_vec",
                             buf);
      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */
      for (j = last_order; j < longest_order; j++) {
        secfile_insert_int(saving->file, -1, "%s.action_vec,%d", buf, j);
      }

      secfile_insert_int_vec(saving->file, tgt_vec, len, "%s.tgt_vec", buf);
      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */
      for (j = last_order; j < longest_order; j++) {
        secfile_insert_int(saving->file, NO_TARGET, "%s.tgt_vec,%d", buf, j);
      }

      secfile_insert_int_vec(saving->file, sub_tgt_vec, len,
                             "%s.sub_tgt_vec", buf);
      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */
      for (j = last_order; j < longest_order; j++) {
        secfile_insert_int(saving->file, -1, "%s.sub_tgt_vec,%d", buf, j);
      }
    } else {
      /* Put all the same fields into the savegame - otherwise the
       * registry code can't correctly use a tabular format and the
       * savegame will be bigger. */
      secfile_insert_int(saving->file, 0, "%s.orders_length", buf);
      secfile_insert_int(saving->file, 0, "%s.orders_index", buf);
      secfile_insert_bool(saving->file, false, "%s.orders_repeat", buf);
      secfile_insert_bool(saving->file, false, "%s.orders_vigilant", buf);
      secfile_insert_str(saving->file, "-", "%s.orders_list", buf);
      secfile_insert_str(saving->file, "-", "%s.dir_list", buf);
      secfile_insert_str(saving->file, "-", "%s.activity_list", buf);

      /* Fill in dummy values for order targets so the registry will save
       * the unit table in a tabular format. */

      // The start of a vector has no number.
      secfile_insert_int(saving->file, -1, "%s.action_vec", buf);
      for (j = 1; j < longest_order; j++) {
        secfile_insert_int(saving->file, -1, "%s.action_vec,%d", buf, j);
      }

      // The start of a vector has no number.
      secfile_insert_int(saving->file, NO_TARGET, "%s.tgt_vec", buf);
      for (j = 1; j < longest_order; j++) {
        secfile_insert_int(saving->file, NO_TARGET, "%s.tgt_vec,%d", buf, j);
      }

      // The start of a vector has no number.
      secfile_insert_int(saving->file, -1, "%s.sub_tgt_vec", buf);
      for (j = 1; j < longest_order; j++) {
        secfile_insert_int(saving->file, -1, "%s.sub_tgt_vec,%d", buf, j);
      }
    }

    i++;
  }
  unit_list_iterate_end;
}

/**
   Load player (client) attributes data
 */
static void sg_load_player_attributes(struct loaddata *loading,
                                      struct player *plr)
{
  int plrno = player_number(plr);

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Toss any existing attribute_block (should not exist)
  plr->attribute_block.clear();

  // This is a big heap of opaque data for the client, check everything!
  auto length = secfile_lookup_int_default(
      loading->file, 0, "player%d.attribute_v2_block_length", plrno);

  if (length < 0) {
    log_sg("player%d.attribute_v2_block_length=%d too small", plrno, length);
  } else if (length >= MAX_ATTRIBUTE_BLOCK) {
    log_sg("player%d.attribute_v2_block_length=%d too big (max %d)", plrno,
           length, MAX_ATTRIBUTE_BLOCK);
  } else if (length > 0) {
    int part_nr, parts;
    size_t actual_length;
    int quoted_length;
    char *quoted;

    sg_failure_ret(secfile_lookup_int(
                       loading->file, &quoted_length,
                       "player%d.attribute_v2_block_length_quoted", plrno),
                   "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file, &parts,
                                      "player%d.attribute_v2_block_parts",
                                      plrno),
                   "%s", secfile_error());

    quoted = new char[quoted_length + 1];
    quoted[0] = '\0';
    plr->attribute_block.resize(length);
    for (part_nr = 0; part_nr < parts; part_nr++) {
      const char *current = secfile_lookup_str(
          loading->file, "player%d.attribute_v2_block_data.part%d", plrno,
          part_nr);
      if (!current) {
        log_sg("attribute_v2_block_parts=%d actual=%d", parts, part_nr);
        break;
      }
      log_debug("attribute_v2_block_length_quoted=%lu have=%lu part=%lu",
                (unsigned long) quoted_length,
                (unsigned long) qstrlen(quoted),
                (unsigned long) qstrlen(current));
      fc_assert(strlen(quoted) + qstrlen(current) <= quoted_length);
      strcat(quoted, current);
    }
    fc_assert_msg(quoted_length == qstrlen(quoted),
                  "attribute_v2_block_length_quoted=%lu actual=%lu",
                  (unsigned long) quoted_length,
                  (unsigned long) qstrlen(quoted));

    actual_length = unquote_block(quoted, plr->attribute_block.data(),
                                  plr->attribute_block.size());
    fc_assert(actual_length == plr->attribute_block.size());
    delete[] quoted;
  }
}

/**
   Save player (client) attributes data.
 */
static void sg_save_player_attributes(struct savedata *saving,
                                      struct player *plr)
{
  int plrno = player_number(plr);

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  /* This is a big heap of opaque data from the client.  Although the binary
   * format is not user editable, keep the lines short enough for debugging,
   * and hope that data compression will keep the file a reasonable size.
   * Note that the "quoted" format is a multiple of 3.
   */
#define PART_SIZE (3 * 256)
#define PART_ADJUST (3)
  if (!plr->attribute_block.isEmpty()) {
    char part[PART_SIZE + PART_ADJUST];
    int parts;
    int current_part_nr;
    char *quoted = quote_block(plr->attribute_block.constData(),
                               plr->attribute_block.size());
    char *quoted_at = strchr(quoted, ':');
    size_t bytes_left = qstrlen(quoted);
    size_t bytes_at_colon = 1 + (quoted_at - quoted);
    size_t bytes_adjust = bytes_at_colon % PART_ADJUST;

    secfile_insert_int(saving->file, plr->attribute_block.size(),
                       "player%d.attribute_v2_block_length", plrno);
    secfile_insert_int(saving->file, bytes_left,
                       "player%d.attribute_v2_block_length_quoted", plrno);

    /* Try to wring some compression efficiencies out of the "quoted" format.
     * The first line has a variable length decimal, mis-aligning triples.
     */
    if ((bytes_left - bytes_adjust) > PART_SIZE) {
      // first line can be longer
      parts = 1 + (bytes_left - bytes_adjust - 1) / PART_SIZE;
    } else {
      parts = 1;
    }

    secfile_insert_int(saving->file, parts,
                       "player%d.attribute_v2_block_parts", plrno);

    if (parts > 1) {
      size_t size_of_current_part = PART_SIZE + bytes_adjust;

      // first line can be longer
      memcpy(part, quoted, size_of_current_part);
      part[size_of_current_part] = '\0';
      secfile_insert_str(saving->file, part,
                         "player%d.attribute_v2_block_data.part%d", plrno,
                         0);
      bytes_left -= size_of_current_part;
      quoted_at = &quoted[size_of_current_part];
      current_part_nr = 1;
    } else {
      quoted_at = quoted;
      current_part_nr = 0;
    }

    for (; current_part_nr < parts; current_part_nr++) {
      size_t size_of_current_part = MIN(bytes_left, PART_SIZE);

      memcpy(part, quoted_at, size_of_current_part);
      part[size_of_current_part] = '\0';
      secfile_insert_str(saving->file, part,
                         "player%d.attribute_v2_block_data.part%d", plrno,
                         current_part_nr);
      bytes_left -= size_of_current_part;
      quoted_at = &quoted_at[size_of_current_part];
    }
    fc_assert(bytes_left == 0);
    delete[] quoted;
  }
#undef PART_ADJUST
#undef PART_SIZE
}

/**
   Load vision data
 */
static void sg_load_player_vision(struct loaddata *loading,
                                  struct player *plr)
{
  int plrno = player_number(plr);
  int total_ncities = secfile_lookup_int_default(loading->file, -1,
                                                 "player%d.dc_total", plrno);
  int i;
  bool someone_alive = false;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (game.server.revealmap & REVEAL_MAP_DEAD) {
    player_list_iterate(team_members(plr->team), pteam_member)
    {
      if (pteam_member->is_alive) {
        someone_alive = true;
        break;
      }
    }
    player_list_iterate_end;

    if (!someone_alive) {
      // Reveal all for completely dead teams.
      map_know_and_see_all(plr);
    }
  }

  if (-1 == total_ncities || !game.info.fogofwar
      || !secfile_lookup_bool_default(loading->file, true,
                                      "game.save_private_map")) {
    /* We have:
     * - a dead player;
     * - fogged cities are not saved for any reason;
     * - a savegame with fog of war turned off;
     * - or game.save_private_map is not set to FALSE in the scenario /
     * savegame. The players private knowledge is set to be what he could
     * see without fog of war. */
    whole_map_iterate(&(wld.map), ptile)
    {
      if (map_is_known(ptile, plr)) {
        struct city *pcity = tile_city(ptile);

        update_player_tile_last_seen(plr, ptile);
        update_player_tile_knowledge(plr, ptile);

        if (nullptr != pcity) {
          update_dumb_city(plr, pcity);
        }
      }
    }
    whole_map_iterate_end;

    // Nothing more to do;
    return;
  }

  // Load player map (terrain).
  LOAD_MAP_CHAR(ch, ptile,
                map_get_player_tile(ptile, plr)->terrain = char2terrain(ch),
                loading->file, "player%d.map_t%04d", plrno);

  // Load player map (extras).
  halfbyte_iterate_extras(j, loading->extra.size)
  {
    LOAD_MAP_CHAR(ch, ptile,
                  sg_extras_set(&map_get_player_tile(ptile, plr)->extras, ch,
                                loading->extra.order + 4 * j),
                  loading->file, "player%d.map_e%02d_%04d", plrno, j);
  }
  halfbyte_iterate_extras_end;

  whole_map_iterate(&(wld.map), ptile)
  {
    struct player_tile *plrtile = map_get_player_tile(ptile, plr);

    extra_type_by_cause_iterate(EC_RESOURCE, pres)
    {
      if (BV_ISSET(plrtile->extras, extra_number(pres))
          && terrain_has_resource(plrtile->terrain, pres)) {
        plrtile->resource = pres;
      }
    }
    extra_type_by_cause_iterate_end;
  }
  whole_map_iterate_end;

  if (game.server.foggedborders) {
    // Load player map (border).
    int x, y;

    for (y = 0; y < wld.map.ysize; y++) {
      const char *buffer = secfile_lookup_str(
          loading->file, "player%d.map_owner%04d", plrno, y);
      const char *buffer2 = secfile_lookup_str(
          loading->file, "player%d.extras_owner%04d", plrno, y);
      const char *ptr = buffer;
      const char *ptr2 = buffer2;

      sg_failure_ret(nullptr != buffer,
                     "Savegame corrupt - map line %d not found.", y);
      for (x = 0; x < wld.map.xsize; x++) {
        char token[TOKEN_SIZE];
        char token2[TOKEN_SIZE];
        int number;
        struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);
        char n[] = ",";
        scanin(const_cast<char **>(&ptr), n, token, sizeof(token));
        sg_failure_ret('\0' != token[0],
                       "Savegame corrupt - map size not correct.");
        if (strcmp(token, "-") == 0) {
          map_get_player_tile(ptile, plr)->owner = nullptr;
        } else {
          sg_failure_ret(str_to_int(token, &number),
                         "Savegame corrupt - got tile owner=%s in (%d, %d).",
                         token, x, y);
          map_get_player_tile(ptile, plr)->owner = player_by_number(number);
        }
        scanin(const_cast<char **>(&ptr2), n, token2, sizeof(token2));
        sg_failure_ret('\0' != token2[0],
                       "Savegame corrupt - map size not correct.");
        if (strcmp(token2, "-") == 0) {
          map_get_player_tile(ptile, plr)->extras_owner = nullptr;
        } else {
          sg_failure_ret(
              str_to_int(token2, &number),
              "Savegame corrupt - got extras owner=%s in (%d, %d).", token,
              x, y);
          map_get_player_tile(ptile, plr)->extras_owner =
              player_by_number(number);
        }
      }
    }
  }

  // Load player map (update time).
  for (i = 0; i < 4; i++) {
    // put 4-bit segments of 16-bit "updated" field
    if (i == 0) {
      LOAD_MAP_CHAR(ch, ptile,
                    map_get_player_tile(ptile, plr)->last_updated =
                        ascii_hex2bin(ch, i),
                    loading->file, "player%d.map_u%02d_%04d", plrno, i);
    } else {
      LOAD_MAP_CHAR(ch, ptile,
                    map_get_player_tile(ptile, plr)->last_updated |=
                    ascii_hex2bin(ch, i),
                    loading->file, "player%d.map_u%02d_%04d", plrno, i);
    }
  }

  // Load player map known cities.
  for (i = 0; i < total_ncities; i++) {
    struct vision_site *pdcity;
    char buf[32];
    fc_snprintf(buf, sizeof(buf), "player%d.dc%d", plrno, i);

    pdcity = vision_site_new(0, nullptr, nullptr);
    if (sg_load_player_vision_city(loading, plr, pdcity, buf)) {
      change_playertile_site(map_get_player_tile(pdcity->location, plr),
                             pdcity);
      identity_number_reserve(pdcity->identity);
    } else {
      // Error loading the data.
      log_sg("Skipping seen city %d for player %d.", i, plrno);
      if (pdcity != nullptr) {
        vision_site_destroy(pdcity);
      }
    }
  }

  // Repair inconsistent player maps.
  whole_map_iterate(&(wld.map), ptile)
  {
    if (map_is_known_and_seen(ptile, plr, V_MAIN)) {
      struct city *pcity = tile_city(ptile);

      update_player_tile_knowledge(plr, ptile);
      reality_check_city(plr, ptile);

      if (nullptr != pcity) {
        update_dumb_city(plr, pcity);
      }
    } else if (!game.server.foggedborders && map_is_known(ptile, plr)) {
      // Non fogged borders aren't loaded. See hrm Bug #879084
      struct player_tile *plrtile = map_get_player_tile(ptile, plr);

      plrtile->owner = tile_owner(ptile);
    }
  }
  whole_map_iterate_end;
}

/**
   Load data for one seen city. sg_save_player_vision_city() is not defined.
 */
static bool sg_load_player_vision_city(struct loaddata *loading,
                                       struct player *plr,
                                       struct vision_site *pdcity,
                                       const char *citystr)
{
  const char *str;
  int i, id, size;
  citizens city_size;
  int nat_x, nat_y;
  const char *stylename;
  enum capital_type cap;

  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_x, "%s.x", citystr),
                  false, "%s", secfile_error());
  sg_warn_ret_val(secfile_lookup_int(loading->file, &nat_y, "%s.y", citystr),
                  false, "%s", secfile_error());
  pdcity->location = native_pos_to_tile(&(wld.map), nat_x, nat_y);
  sg_warn_ret_val(nullptr != pdcity->location, false,
                  "%s invalid tile (%d,%d)", citystr, nat_x, nat_y);

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &id, "%s.owner", citystr), false,
      "%s", secfile_error());
  pdcity->owner = player_by_number(id);
  sg_warn_ret_val(nullptr != pdcity->owner, false,
                  "%s has invalid owner (%d); skipping.", citystr, id);

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &pdcity->identity, "%s.id", citystr),
      false, "%s", secfile_error());
  sg_warn_ret_val(IDENTITY_NUMBER_ZERO < pdcity->identity, false,
                  "%s has invalid id (%d); skipping.", citystr, id);

  sg_warn_ret_val(
      secfile_lookup_int(loading->file, &size, "%s.size", citystr), false,
      "%s", secfile_error());
  city_size = static_cast<citizens>(size); // set the correct type
  sg_warn_ret_val(size == (int) city_size, false,
                  "Invalid city size: %d; set to %d.", size, city_size);
  vision_site_size_set(pdcity, city_size);

  // Initialise list of improvements
  BV_CLR_ALL(pdcity->improvements);
  str = secfile_lookup_str(loading->file, "%s.improvements", citystr);
  sg_warn_ret_val(str != nullptr, false, "%s", secfile_error());
  sg_warn_ret_val(strlen(str) == loading->improvement.size, false,
                  "Invalid length of '%s.improvements' (%lu ~= %lu).",
                  citystr, (unsigned long) qstrlen(str),
                  (unsigned long) loading->improvement.size);
  for (i = 0; i < loading->improvement.size; i++) {
    sg_warn_ret_val(str[i] == '1' || str[i] == '0', false,
                    "Undefined value '%c' within '%s.improvements'.", str[i],
                    citystr)

        if (str[i] == '1')
    {
      struct impr_type *pimprove =
          improvement_by_rule_name(loading->improvement.order[i]);
      if (pimprove) {
        BV_SET(pdcity->improvements, improvement_index(pimprove));
      }
    }
  }

  // Use the section as backup name.
  sz_strlcpy(pdcity->name, secfile_lookup_str_default(loading->file, citystr,
                                                      "%s.name", citystr));

  pdcity->occupied = secfile_lookup_bool_default(loading->file, false,
                                                 "%s.occupied", citystr);
  pdcity->walls =
      secfile_lookup_bool_default(loading->file, false, "%s.walls", citystr);
  pdcity->happy =
      secfile_lookup_bool_default(loading->file, false, "%s.happy", citystr);
  pdcity->unhappy = secfile_lookup_bool_default(loading->file, false,
                                                "%s.unhappy", citystr);
  stylename = secfile_lookup_str_default(loading->file, nullptr, "%s.style",
                                         citystr);
  if (stylename != nullptr) {
    pdcity->style = city_style_by_rule_name(stylename);
  } else {
    pdcity->style = 0;
  }
  if (pdcity->style < 0) {
    pdcity->style = 0;
  }

  pdcity->city_image = secfile_lookup_int_default(loading->file, -100,
                                                  "%s.city_image", citystr);

  cap =
      capital_type_by_name(secfile_lookup_str_default(loading->file, nullptr,
                                                      "%s.capital", citystr),
                           fc_strcasecmp);

  if (capital_type_is_valid(cap)) {
    pdcity->capital = cap;
  } else {
    pdcity->capital = CAPITAL_NOT;
  }

  return true;
}

/**
   Save vision data
 */
static void sg_save_player_vision(struct savedata *saving,
                                  struct player *plr)
{
  int i, plrno = player_number(plr);

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (!game.info.fogofwar || !game.server.save_options.save_private_map) {
    // The player can see all, there's no reason to save the private map.
    return;
  }

  // Save the map (terrain).
  SAVE_MAP_CHAR(ptile,
                terrain2char(map_get_player_tile(ptile, plr)->terrain),
                saving->file, "player%d.map_t%04d", plrno);

  if (game.server.foggedborders) {
    // Save the map (borders).
    int x, y;

    for (y = 0; y < wld.map.ysize; y++) {
      char line[wld.map.xsize * TOKEN_SIZE];

      line[0] = '\0';
      for (x = 0; x < wld.map.xsize; x++) {
        char token[TOKEN_SIZE];
        struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);
        struct player_tile *plrtile = map_get_player_tile(ptile, plr);

        if (plrtile == nullptr || plrtile->owner == nullptr) {
          qstrcpy(token, "-");
        } else {
          fc_snprintf(token, sizeof(token), "%d",
                      player_number(plrtile->owner));
        }
        strcat(line, token);
        if (x < wld.map.xsize) {
          strcat(line, ",");
        }
      }
      secfile_insert_str(saving->file, line, "player%d.map_owner%04d", plrno,
                         y);
    }

    for (y = 0; y < wld.map.ysize; y++) {
      char line[wld.map.xsize * TOKEN_SIZE];

      line[0] = '\0';
      for (x = 0; x < wld.map.xsize; x++) {
        char token[TOKEN_SIZE];
        struct tile *ptile = native_pos_to_tile(&(wld.map), x, y);
        struct player_tile *plrtile = map_get_player_tile(ptile, plr);

        if (plrtile == nullptr || plrtile->extras_owner == nullptr) {
          qstrcpy(token, "-");
        } else {
          fc_snprintf(token, sizeof(token), "%d",
                      player_number(plrtile->extras_owner));
        }
        strcat(line, token);
        if (x < wld.map.xsize) {
          strcat(line, ",");
        }
      }
      secfile_insert_str(saving->file, line, "player%d.extras_owner%04d",
                         plrno, y);
    }
  }

  // Save the map (extras).
  halfbyte_iterate_extras(j, game.control.num_extra_types)
  {
    int mod[4];
    int l;

    for (l = 0; l < 4; l++) {
      if (4 * j + 1 > game.control.num_extra_types) {
        mod[l] = -1;
      } else {
        mod[l] = 4 * j + l;
      }
    }

    SAVE_MAP_CHAR(ptile,
                  sg_extras_get(map_get_player_tile(ptile, plr)->extras,
                                map_get_player_tile(ptile, plr)->resource,
                                mod),
                  saving->file, "player%d.map_e%02d_%04d", plrno, j);
  }
  halfbyte_iterate_extras_end;

  // Save the map (update time).
  for (i = 0; i < 4; i++) {
    // put 4-bit segments of 16-bit "updated" field
    SAVE_MAP_CHAR(
        ptile,
        bin2ascii_hex(map_get_player_tile(ptile, plr)->last_updated, i),
        saving->file, "player%d.map_u%02d_%04d", plrno, i);
  }

  // Save known cities.
  i = 0;
  whole_map_iterate(&(wld.map), ptile)
  {
    struct vision_site *pdcity = map_get_player_city(ptile, plr);
    char impr_buf[B_LAST + 1];
    char buf[32];

    fc_snprintf(buf, sizeof(buf), "player%d.dc%d", plrno, i);

    if (nullptr != pdcity && plr != vision_site_owner(pdcity)) {
      int nat_x, nat_y;

      index_to_native_pos(&nat_x, &nat_y, tile_index(ptile));
      secfile_insert_int(saving->file, nat_y, "%s.y", buf);
      secfile_insert_int(saving->file, nat_x, "%s.x", buf);

      secfile_insert_int(saving->file, pdcity->identity, "%s.id", buf);
      secfile_insert_int(saving->file,
                         player_number(vision_site_owner(pdcity)),
                         "%s.owner", buf);

      secfile_insert_int(saving->file, vision_site_size_get(pdcity),
                         "%s.size", buf);
      secfile_insert_bool(saving->file, pdcity->occupied, "%s.occupied",
                          buf);
      secfile_insert_bool(saving->file, pdcity->walls, "%s.walls", buf);
      secfile_insert_bool(saving->file, pdcity->happy, "%s.happy", buf);
      secfile_insert_bool(saving->file, pdcity->unhappy, "%s.unhappy", buf);
      secfile_insert_str(saving->file, city_style_rule_name(pdcity->style),
                         "%s.style", buf);
      secfile_insert_int(saving->file, pdcity->city_image, "%s.city_image",
                         buf);
      secfile_insert_str(saving->file, capital_type_name(pdcity->capital),
                         "%s.capital", buf);

      /* Save improvement list as bitvector. Note that improvement order
       * is saved in savefile.improvement.order. */
      improvement_iterate(pimprove)
      {
        impr_buf[improvement_index(pimprove)] =
            BV_ISSET(pdcity->improvements, improvement_index(pimprove))
                ? '1'
                : '0';
      }
      improvement_iterate_end;
      impr_buf[improvement_count()] = '\0';
      sg_failure_ret(
          qstrlen(impr_buf) < sizeof(impr_buf),
          "Invalid size of the improvement vector (%s.improvements: "
          "%lu < %lu).",
          buf, (long unsigned int) qstrlen(impr_buf),
          (long unsigned int) sizeof(impr_buf));
      secfile_insert_str(saving->file, impr_buf, "%s.improvements", buf);
      secfile_insert_str(saving->file, pdcity->name, "%s.name", buf);

      i++;
    }
  }
  whole_map_iterate_end;

  secfile_insert_int(saving->file, i, "player%d.dc_total", plrno);
}

/* =======================================================================
 * Load / save the researches.
 * ======================================================================= */

/**
   Load '[research]'.
 */
static void sg_load_researches(struct loaddata *loading)
{
  struct research *presearch;
  int count;
  int number;
  const char *str;
  int i, j;
  int *vlist_research;
  size_t count_res;

  vlist_research = nullptr;
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Initialize all researches.
  for (auto &pinitres : research_array) {
    if (research_is_valid(pinitres)) {
      init_tech(&pinitres, false);
    }
  };

  // May be unsaved (e.g. scenario case).
  count = secfile_lookup_int_default(loading->file, 0, "research.count");
  for (i = 0; i < count; i++) {
    sg_failure_ret(
        secfile_lookup_int(loading->file, &number, "research.r%d.number", i),
        "%s", secfile_error());
    presearch = research_by_number(number);
    sg_failure_ret(presearch != nullptr,
                   "Invalid research number %d in 'research.r%d.number'",
                   number, i);

    presearch->tech_goal =
        technology_load(loading->file, "research.r%d.goal", i);
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->techs_researched,
                                      "research.r%d.techs", i),
                   "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file, &presearch->future_tech,
                                      "research.r%d.futuretech", i),
                   "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->bulbs_researched,
                                      "research.r%d.bulbs", i),
                   "%s", secfile_error());
    sg_failure_ret(secfile_lookup_int(loading->file,
                                      &presearch->bulbs_researching_saved,
                                      "research.r%d.bulbs_before", i),
                   "%s", secfile_error());
    presearch->researching_saved =
        technology_load(loading->file, "research.r%d.saved", i);
    presearch->researching =
        technology_load(loading->file, "research.r%d.now", i);
    sg_failure_ret(secfile_lookup_bool(loading->file, &presearch->got_tech,
                                       "research.r%d.got_tech", i),
                   "%s", secfile_error());

    str = secfile_lookup_str(loading->file, "research.r%d.done", i);
    sg_failure_ret(str != nullptr, "%s", secfile_error());
    sg_failure_ret(strlen(str) == loading->technology.size,
                   "Invalid length of 'research.r%d.done' (%lu ~= %lu).", i,
                   (unsigned long) qstrlen(str),
                   (unsigned long) loading->technology.size);
    for (j = 0; j < loading->technology.size; j++) {
      sg_failure_ret(str[j] == '1' || str[j] == '0',
                     "Undefined value '%c' within 'research.r%d.done'.",
                     str[j], i);

      if (str[j] == '1') {
        struct advance *padvance =
            advance_by_rule_name(loading->technology.order[j]);

        if (padvance) {
          research_invention_set(presearch, advance_number(padvance),
                                 TECH_KNOWN);
        }
      }
    }

    if (game.server.multiresearch) {
      vlist_research = secfile_lookup_int_vec(loading->file, &count_res,
                                              "research.r%d.vbs", i);
      fc_assert_ret(vlist_research);
      advance_index_iterate(A_FIRST, o)
      {
        presearch->inventions[o].bulbs_researched_saved = vlist_research[o];
      }
      advance_index_iterate_end;
      delete[] vlist_research;
      vlist_research = nullptr;
    }
  }

  /* In case of tech_leakage, we can update research only after all the
   * researches have been loaded */
  for (auto &pupres : research_array) {
    if (research_is_valid(pupres)) {
      research_update(&pupres);
    }
  };
}

/**
   Save '[research]'.
 */
static void sg_save_researches(struct savedata *saving)
{
  char invs[A_LAST];
  int i = 0;
  int *vlist_research;

  vlist_research = nullptr;
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (saving->save_players) {
    for (const auto &presearch : research_array) {
      if (research_is_valid(presearch)) {
        secfile_insert_int(saving->file, research_number(&presearch),
                           "research.r%d.number", i);
        technology_save(saving->file, "research.r%d.goal", i,
                        presearch.tech_goal);
        secfile_insert_int(saving->file, presearch.techs_researched,
                           "research.r%d.techs", i);
        secfile_insert_int(saving->file, presearch.future_tech,
                           "research.r%d.futuretech", i);
        secfile_insert_int(saving->file, presearch.bulbs_researching_saved,
                           "research.r%d.bulbs_before", i);
        if (game.server.multiresearch) {
          vlist_research = new int[game.control.num_tech_types]();
          advance_index_iterate(A_FIRST, j)
          {
            vlist_research[j] =
                presearch.inventions[j].bulbs_researched_saved;
          }
          advance_index_iterate_end;
          secfile_insert_int_vec(saving->file, vlist_research,
                                 game.control.num_tech_types,
                                 "research.r%d.vbs", i);
          delete[] vlist_research;
          vlist_research = nullptr;
        }
        technology_save(saving->file, "research.r%d.saved", i,
                        presearch.researching_saved);
        secfile_insert_int(saving->file, presearch.bulbs_researched,
                           "research.r%d.bulbs", i);
        technology_save(saving->file, "research.r%d.now", i,
                        presearch.researching);
        secfile_insert_bool(saving->file, presearch.got_tech,
                            "research.r%d.got_tech", i);
        /* Save technology lists as bytevector. Note that technology order is
         * saved in savefile.technology.order */
        advance_index_iterate(A_NONE, tech_id)
        {
          invs[tech_id] =
              (research_invention_state(&presearch, tech_id) == TECH_KNOWN
                   ? '1'
                   : '0');
        }
        advance_index_iterate_end;
        invs[game.control.num_tech_types] = '\0';
        secfile_insert_str(saving->file, invs, "research.r%d.done", i);
        i++;
      }
    };
    secfile_insert_int(saving->file, i, "research.count");
  }
}

/* =======================================================================
 * Load / save the event cache. Should be the last thing to do.
 * ======================================================================= */

/**
   Load '[event_cache]'.
 */
static void sg_load_event_cache(struct loaddata *loading)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  event_cache_load(loading->file, "event_cache");
}

/**
   Save '[event_cache]'.
 */
static void sg_save_event_cache(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (saving->scenario) {
    // Do _not_ save events in a scenario.
    return;
  }

  event_cache_save(saving->file, "event_cache");
}

/* =======================================================================
 * Load / save the open treaties
 * ======================================================================= */

/**
   Load '[treaty_xxx]'.
 */
static void sg_load_treaties(struct loaddata *loading)
{
  int tidx;
  const char *plr0;
  struct treaty_list *treaties = get_all_treaties();

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  for (tidx = 0; (plr0 = secfile_lookup_str_default(loading->file, nullptr,
                                                    "treaty%d.plr0", tidx))
                 != nullptr;
       tidx++) {
    const char *plr1;
    const char *ct;
    int cidx;
    struct player *p0, *p1;

    plr1 = secfile_lookup_str(loading->file, "treaty%d.plr1", tidx);

    p0 = player_by_name(plr0);
    p1 = player_by_name(plr1);

    if (p0 == nullptr || p1 == nullptr) {
      qCritical("Treaty between unknown players %s and %s", plr0, plr1);
    } else {
      struct Treaty *ptreaty = new Treaty;

      init_treaty(ptreaty, p0, p1);
      treaty_list_prepend(treaties, ptreaty);

      for (cidx = 0; (ct = secfile_lookup_str_default(
                          loading->file, nullptr, "treaty%d.clause%d.type",
                          tidx, cidx))
                     != nullptr;
           cidx++) {
        enum clause_type type = clause_type_by_name(ct, fc_strcasecmp);
        const char *plrx;

        if (!clause_type_is_valid(type)) {
          qCritical("Invalid clause type \"%s\"", ct);
        } else {
          struct player *pgiver = nullptr;

          plrx = secfile_lookup_str(loading->file, "treaty%d.clause%d.from",
                                    tidx, cidx);

          if (!fc_strcasecmp(plrx, plr0)) {
            pgiver = p0;
          } else if (!fc_strcasecmp(plrx, plr1)) {
            pgiver = p1;
          } else {
            qCritical("Clause giver %s is not participant of the treaty"
                      "between %s and %s",
                      plrx, plr0, plr1);
          }

          if (pgiver != nullptr) {
            int value;

            value = secfile_lookup_int_default(
                loading->file, 0, "treaty%d.clause%d.value", tidx, cidx);

            add_clause(ptreaty, pgiver, type, value);
          }
        }

        /* These must be after clauses have been added so that acceptance
         * does not get cleared by what seems like changes to the treaty. */
        ptreaty->accept0 = secfile_lookup_bool_default(
            loading->file, false, "treaty%d.accept0", tidx);
        ptreaty->accept1 = secfile_lookup_bool_default(
            loading->file, false, "treaty%d.accept1", tidx);
      }
    }
  }
}

/**
   Save '[treaty_xxx]'.
 */
static void sg_save_treaties(struct savedata *saving)
{
  struct treaty_list *treaties = get_all_treaties();
  int tidx = 0;

  treaty_list_iterate(treaties, ptr)
  {
    char tpath[512];
    int cidx = 0;

    fc_snprintf(tpath, sizeof(tpath), "treaty%d", tidx++);

    secfile_insert_str(saving->file, player_name(ptr->plr0), "%s.plr0",
                       tpath);
    secfile_insert_str(saving->file, player_name(ptr->plr1), "%s.plr1",
                       tpath);
    secfile_insert_bool(saving->file, ptr->accept0, "%s.accept0", tpath);
    secfile_insert_bool(saving->file, ptr->accept1, "%s.accept1", tpath);

    clause_list_iterate(ptr->clauses, pclaus)
    {
      char cpath[512];

      fc_snprintf(cpath, sizeof(cpath), "%s.clause%d", tpath, cidx++);

      secfile_insert_str(saving->file, clause_type_name(pclaus->type),
                         "%s.type", cpath);
      secfile_insert_str(saving->file, player_name(pclaus->from), "%s.from",
                         cpath);
      secfile_insert_int(saving->file, pclaus->value, "%s.value", cpath);
    }
    clause_list_iterate_end;
  }
  treaty_list_iterate_end;
}

/* =======================================================================
 * Load / save the history report
 * ======================================================================= */

/**
   Load '[history]'.
 */
static void sg_load_history(struct loaddata *loading)
{
  struct history_report *hist = history_report_get();
  int turn;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  turn = secfile_lookup_int_default(loading->file, -2, "history.turn");

  if (turn + 1 >= game.info.turn) {
    const char *str;

    hist->turn = turn;
    str = secfile_lookup_str(loading->file, "history.title");
    sg_failure_ret(str != nullptr, "%s", secfile_error());
    sz_strlcpy(hist->title, str);
    str = secfile_lookup_str(loading->file, "history.body");
    sg_failure_ret(str != nullptr, "%s", secfile_error());
    sz_strlcpy(hist->body, str);
  }
}

/**
   Save '[history]'.
 */
static void sg_save_history(struct savedata *saving)
{
  struct history_report *hist = history_report_get();

  secfile_insert_int(saving->file, hist->turn, "history.turn");

  if (hist->turn + 1 >= game.info.turn) {
    secfile_insert_str(saving->file, hist->title, "history.title");
    secfile_insert_str(saving->file, hist->body, "history.body");
  }
}

/* =======================================================================
 * Load / save the mapimg definitions.
 * ======================================================================= */

/**
   Load '[mapimg]'.
 */
static void sg_load_mapimg(struct loaddata *loading)
{
  int mapdef_count, i;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  // Clear all defined map images.
  while (mapimg_count() > 0) {
    mapimg_delete(0);
  }

  mapdef_count =
      secfile_lookup_int_default(loading->file, 0, "mapimg.count");
  qDebug("Saved map image definitions: %d.", mapdef_count);

  if (0 >= mapdef_count) {
    return;
  }

  for (i = 0; i < mapdef_count; i++) {
    const char *p;

    p = secfile_lookup_str(loading->file, "mapimg.mapdef%d", i);
    if (nullptr == p) {
      qDebug("[Mapimg %4d] Missing definition.", i);
      continue;
    }

    if (!mapimg_define(p, false)) {
      qCritical("Invalid map image definition %4d: %s.", i, p);
    }

    qDebug("Mapimg %4d loaded.", i);
  }
}

/**
   Save '[mapimg]'.
 */
static void sg_save_mapimg(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  secfile_insert_int(saving->file, mapimg_count(), "mapimg.count");
  if (mapimg_count() > 0) {
    int i;

    for (i = 0; i < mapimg_count(); i++) {
      char buf[MAX_LEN_MAPDEF];

      mapimg_id2str(i, buf, sizeof(buf));
      secfile_insert_str(saving->file, buf, "mapimg.mapdef%d", i);
    }
  }
}

/* =======================================================================
 * Sanity checks for loading / saving a game.
 * ======================================================================= */

/**
   Sanity check for loaded game.
 */
static void sg_load_sanitycheck(struct loaddata *loading)
{
  int players;

  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();

  if (game.info.is_new_game) {
    // Nothing to do for new games (or not started scenarios).
    return;
  }

  /* Old savegames may have maxplayers lower than current player count,
   * fix. */
  players = normal_player_count();
  if (game.server.max_players < players) {
    qDebug("Max players lower than current players, fixing");
    game.server.max_players = players;
  }

  // Fix ferrying sanity
  players_iterate(pplayer)
  {
    unit_list_iterate_safe(pplayer->units, punit)
    {
      if (!unit_transport_get(punit)
          && !can_unit_exist_at_tile(&(wld.map), punit, unit_tile(punit))) {
        log_sg("Removing %s unferried %s in %s at (%d, %d)",
               nation_rule_name(nation_of_player(pplayer)),
               unit_rule_name(punit),
               terrain_rule_name(unit_tile(punit)->terrain),
               TILE_XY(unit_tile(punit)));
        bounce_unit(punit, true);
      }
    }
    unit_list_iterate_safe_end;
  }
  players_iterate_end;

  /* Fix stacking issues.  We don't rely on the savegame preserving
   * alliance invariants (old savegames often did not) so if there are any
   * unallied units on the same tile we just bounce them. */
  players_iterate(pplayer)
  {
    players_iterate(aplayer) { resolve_unit_stacks(pplayer, aplayer, true); }
    players_iterate_end;
  }
  players_iterate_end;

  /* Recalculate the potential buildings for each city. Has caused some
   * problems with game random state.
   * This also changes the game state if you save the game directly after
   * loading it and compare the results. */
  players_iterate(pplayer)
  {
    bool saved_ai_control = is_ai(pplayer);

    // Recalculate for all players.
    set_as_human(pplayer);

    // Building advisor needs data phase open in order to work
    adv_data_phase_init(pplayer, false);
    building_advisor(pplayer);
    // Close data phase again so it can be opened again when game starts.
    adv_data_phase_done(pplayer);

    if (saved_ai_control) {
      set_as_ai(pplayer);
    }
  }
  players_iterate_end;

  // Check worked tiles map
#ifdef FREECIV_DEBUG
  if (loading->worked_tiles != nullptr) {
    // check the entire map for unused worked tiles
    whole_map_iterate(&(wld.map), ptile)
    {
      if (loading->worked_tiles[ptile->index] != -1) {
        qCritical("[city id: %d] Unused worked tile at (%d, %d).",
                  loading->worked_tiles[ptile->index], TILE_XY(ptile));
      }
    }
    whole_map_iterate_end;
  }
#endif // FREECIV_DEBUG

  // Check researching technologies and goals.
  for (auto &presearch : research_array) {
    if (research_is_valid(presearch)) {
      if (presearch.researching != A_UNSET
          && !is_future_tech(presearch.researching)
          && (valid_advance_by_number(presearch.researching) == nullptr
              || (research_invention_state(&presearch, presearch.researching)
                  != TECH_PREREQS_KNOWN))) {
        log_sg(_("%s had invalid researching technology."),
               research_name_translation(&presearch));
        presearch.researching = A_UNSET;
      }
      if (presearch.tech_goal != A_UNSET
          && !is_future_tech(presearch.tech_goal)
          && (valid_advance_by_number(presearch.tech_goal) == nullptr
              || !research_invention_reachable(&presearch,
                                               presearch.tech_goal)
              || (research_invention_state(&presearch, presearch.tech_goal)
                  == TECH_KNOWN))) {
        log_sg(_("%s had invalid technology goal."),
               research_name_translation(&presearch));
        presearch.tech_goal = A_UNSET;
      }
    }
  };

  // Check if some player has more than one of some UTYF_UNIQUE unit type
  players_iterate(pplayer)
  {
    int unique_count[U_LAST];

    memset(unique_count, 0, sizeof(unique_count));

    unit_list_iterate(pplayer->units, punit)
    {
      unique_count[utype_index(unit_type_get(punit))]++;
    }
    unit_list_iterate_end;

    unit_type_iterate(ut)
    {
      if (unique_count[utype_index(ut)] > 1
          && utype_has_flag(ut, UTYF_UNIQUE)) {
        log_sg(_("%s has multiple units of type %s though it should be "
                 "possible "
                 "to have only one."),
               player_name(pplayer), utype_name_translation(ut));
      }
    }
    unit_type_iterate_end;
  }
  players_iterate_end;

  players_iterate(pplayer)
  {
    unit_list_iterate_safe(pplayer->units, punit)
    {
      if (!unit_order_list_is_sane(punit->orders.length,
                                   punit->orders.list)) {
        log_sg("Invalid unit orders for unit %d.", punit->id);
        free_unit_orders(punit);
      }
    }
    unit_list_iterate_safe_end;
  }
  players_iterate_end;

  if (0 == qstrlen(server.game_identifier)
      || !is_base64url(server.game_identifier)) {
    // This uses fc_rand(), so random state has to be initialized before.
    randomize_base64url_string(server.game_identifier,
                               sizeof(server.game_identifier));
  }

  /* Restore game random state, just in case various initialization code
   * inexplicably altered the previously existing state. */
  if (!game.info.is_new_game) {
    fc_rand_set_state(loading->rstate);
  }

  // At the end do the default sanity checks.
  sanity_check();
}

/**
   Sanity check for saved game.
 */
static void sg_save_sanitycheck(struct savedata *saving)
{
  // Check status and return if not OK (sg_success != TRUE).
  sg_check_ret();
}
