/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
 */

#include <fc_config.h>

// utility
#include "astring.h"
#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "registry.h"
#include "shared.h"

// common
#include "map.h"
#include "nation.h"

// server
#include "gamehand.h"
#include "maphand.h"
#include "meta.h"
#include "notify.h"
#include "plrhand.h"
#include "report.h"
#include "rssanity.h"
#include "settings.h"
#include "srv_main.h"
#include "stdinhand.h"

/* The following classes determine what can be changed when. Actually, some
 * of them have the same "changeability", but different types are separated
 * here in case they have other uses. Also, SSET_GAME_INIT/SSET_RULES
 * separate the two sections of server settings sent to the client. See the
 * settings[] array and setting_is_changeable() for what these correspond to
 * and explanations.
 */
enum sset_class {
  SSET_MAP_SIZE,
  SSET_MAP_GEN,
  SSET_MAP_ADD,
  SSET_PLAYERS,
  SSET_GAME_INIT,
  SSET_RULES,
  SSET_RULES_SCENARIO,
  SSET_RULES_FLEXIBLE,
  SSET_META
};

typedef bool (*bool_validate_func_t)(bool value, struct connection *pconn,
                                     char *reject_msg,
                                     size_t reject_msg_len);
typedef bool (*int_validate_func_t)(int value, struct connection *pconn,
                                    char *reject_msg, size_t reject_msg_len);
typedef bool (*string_validate_func_t)(const char *value,
                                       struct connection *pconn,
                                       char *reject_msg,
                                       size_t reject_msg_len);
typedef bool (*enum_validate_func_t)(int value, struct connection *pconn,
                                     char *reject_msg,
                                     size_t reject_msg_len);
typedef bool (*bitwise_validate_func_t)(unsigned value,
                                        struct connection *pconn,
                                        char *reject_msg,
                                        size_t reject_msg_len);

typedef void (*action_callback_func_t)(const struct setting *pset);
typedef const char *(*help_callback_func_t)(const struct setting *pset);
typedef const struct sset_val_name *(*val_name_func_t)(int value);

struct setting {
  const char *name;
  enum sset_class sclass;

  // What access level viewing and setting the setting requires.
  enum cmdlevel access_level_read;
  enum cmdlevel access_level_write;

  /* Should be less than 42 chars (?), or shorter if the values may have more
   * than about 4 digits. Don't put "." on the end.
   */
  const char *short_help;

  /* May be empty string, if short_help is sufficient. Need not include
   * embedded newlines (but may, for formatting); lines will be wrapped (and
   * indented) automatically. Should have punctuation etc, and should end
   * with a "."
   */
  const char *extra_help;

  // help function
  const help_callback_func_t help_func;

  enum sset_type stype;
  enum sset_category scategory;
  enum sset_level slevel;

  /* About the *_validate functions: If the function is non-nullptr, it is
   * called with the new value, and returns whether the change is legal. The
   * char * is an error message in the case of reject.
   */

  union {
    /*** bool part ***/
    struct {
      bool *const pvalue;
      const bool default_value;
      const bool_validate_func_t validate;
      const val_name_func_t name;
      bool game_value;
    } boolean;
    /*** int part ***/
    struct {
      int *const pvalue;
      const int default_value;
      const int min_value;
      const int max_value;
      const int_validate_func_t validate;
      int game_value;
    } integer;
    /*** string part ***/
    struct {
      char *const value;
      const char *const default_value;
      const size_t value_size;
      const string_validate_func_t validate;
      char *game_value;
    } string;
    /*** enumerator part ***/
    struct {
      void *const pvalue;
      const int store_size;
      const int default_value;
      const enum_validate_func_t validate;
      const val_name_func_t name;
      int game_value;
    } enumerator;
    /*** bitwise part ***/
    struct {
      unsigned *const pvalue;
      const unsigned default_value;
      const bitwise_validate_func_t validate;
      const val_name_func_t name;
      unsigned game_value;
    } bitwise;
  };

  // action function
  const action_callback_func_t action;

  // ruleset lock for game settings
  bool locked;

  // It's not "default", even if value is the same as default
  enum setting_default_level setdef;
};

static struct {
  bool init;
  struct setting_list *level[OLEVELS_NUM];
} setting_sorted = {.init = false};

static bool setting_ruleset_one(struct section_file *file, const char *name,
                                const char *path);
static void setting_game_set(struct setting *pset, bool init);
static void setting_game_free(struct setting *pset);
static void setting_game_restore(struct setting *pset);

static void settings_list_init();
static void settings_list_free();
int settings_list_cmp(const struct setting *const *pset1,
                      const struct setting *const *pset2);

#define settings_snprintf(_buf, _buf_len, format, ...)                      \
  if (_buf != nullptr) {                                                    \
    fc_snprintf(_buf, _buf_len, format, ##__VA_ARGS__);                     \
  }

static bool set_enum_value(struct setting *pset, int val);

/*
  Enumerator name accessors.

  Important note about compatibility:
  1) you cannot modify the support name of an existant value. However, in a
     developpement, you can modify it if it wasn't included in any stable
     branch before.
  2) Take care of modifiying the pretty name of an existant value: make sure
  to modify the help texts which are using it.
 */

#define NAME_CASE(_val, _support, _pretty)                                  \
  case _val: {                                                              \
    static const struct sset_val_name name = {_support, _pretty};           \
    return &name;                                                           \
  }

/**
   Caravan bonus style setting names accessor.
 */
static const struct sset_val_name *caravanbonusstyle_name(int caravanbonus)
{
  switch (caravanbonus) {
    NAME_CASE(CBS_CLASSIC, "CLASSIC", N_("Classic Freeciv"));
    NAME_CASE(CBS_LOGARITHMIC, "LOGARITHMIC", N_("Log^2 N style"));
    NAME_CASE(CBS_LINEAR, "LINEAR", N_("Linear max trade"));
    NAME_CASE(CBS_DISTANCE, "DISTANCE", N_("Pure distance"));
  }
  return nullptr;
}

/**
   Map size definition setting names accessor. This setting has an
   hard-coded depedence in "server/meta.c".
 */
static const struct sset_val_name *mapsize_name(int mapsize)
{
  switch (mapsize) {
    NAME_CASE(MAPSIZE_FULLSIZE, "FULLSIZE", N_("Number of tiles"));
    NAME_CASE(MAPSIZE_PLAYER, "PLAYER", N_("Tiles per player"));
    NAME_CASE(MAPSIZE_XYSIZE, "XYSIZE", N_("Width and height"));
  }
  return nullptr;
}

/**
   Topology setting names accessor.
 */
static const struct sset_val_name *topology_name(int topology_bit)
{
  switch (1 << topology_bit) {
    NAME_CASE(TF_WRAPX, "WRAPX", N_("Wrap East-West"));
    NAME_CASE(TF_WRAPY, "WRAPY", N_("Wrap North-South"));
    NAME_CASE(TF_ISO, "ISO", N_("Isometric"));
    NAME_CASE(TF_HEX, "HEX", N_("Hexagonal"));
  }
  return nullptr;
}

/**
   Trade revenue style setting names accessor.
 */
static const struct sset_val_name *traderevenuestyle_name(int revenue_style)
{
  switch (revenue_style) {
    NAME_CASE(TRS_CLASSIC, "CLASSIC", N_("Classic Freeciv"));
    NAME_CASE(TRS_SIMPLE, "SIMPLE", N_("Proportional to tile trade"));
  }
  return nullptr;
}

/**
   Generator setting names accessor.
 */
static const struct sset_val_name *generator_name(int generator)
{
  switch (generator) {
    NAME_CASE(MAPGEN_SCENARIO, "SCENARIO", N_("Scenario map"));
    NAME_CASE(MAPGEN_RANDOM, "RANDOM", N_("Fully random height"));
    NAME_CASE(MAPGEN_FRACTAL, "FRACTAL", N_("Pseudo-fractal height"));
    NAME_CASE(MAPGEN_ISLAND, "ISLAND", N_("Island-based"));
    NAME_CASE(MAPGEN_FAIR, "FAIR", N_("Fair islands"));
    NAME_CASE(MAPGEN_FRACTURE, "FRACTURE", N_("Fracture map"));
  }
  return nullptr;
}

/**
   Start position setting names accessor.
 */
static const struct sset_val_name *startpos_name(int startpos)
{
  switch (startpos) {
    NAME_CASE(MAPSTARTPOS_DEFAULT, "DEFAULT", N_("Generator's choice"));
    NAME_CASE(MAPSTARTPOS_SINGLE, "SINGLE", N_("One player per continent"));
    NAME_CASE(MAPSTARTPOS_2or3, "2or3",
              N_("Two or three players per continent"));
    NAME_CASE(MAPSTARTPOS_ALL, "ALL",
              N_("All players on a single continent"));
    NAME_CASE(MAPSTARTPOS_VARIABLE, "VARIABLE",
              N_("Depending on size of continents"));
  }
  return nullptr;
}

/**
   Team placement setting names accessor.
 */
static const struct sset_val_name *teamplacement_name(int team_placement)
{
  switch (team_placement) {
    NAME_CASE(TEAM_PLACEMENT_DISABLED, "DISABLED", N_("Disabled"));
    NAME_CASE(TEAM_PLACEMENT_CLOSEST, "CLOSEST", N_("As close as possible"));
    NAME_CASE(TEAM_PLACEMENT_CONTINENT, "CONTINENT",
              N_("On the same continent"));
    NAME_CASE(TEAM_PLACEMENT_HORIZONTAL, "HORIZONTAL",
              N_("Horizontal placement"));
    NAME_CASE(TEAM_PLACEMENT_VERTICAL, "VERTICAL", N_("Vertical placement"));
  }
  return nullptr;
}

/**
   Persistentready setting names accessor.
 */
static const struct sset_val_name *persistentready_name(int persistent_ready)
{
  switch (persistent_ready) {
    NAME_CASE(PERSISTENTR_DISABLED, "DISABLED", N_("Disabled"));
    NAME_CASE(PERSISTENTR_CONNECTED, "CONNECTED",
              N_("As long as connected"));
  }

  return nullptr;
}

/**
   Victory conditions setting names accessor.
 */
static const struct sset_val_name *victory_conditions_name(int condition_bit)
{
  switch (condition_bit) {
    NAME_CASE(VC_SPACERACE, "SPACERACE", N_("Spacerace"));
    NAME_CASE(VC_ALLIED, "ALLIED", N_("Allied victory"));
    NAME_CASE(VC_CULTURE, "CULTURE", N_("Culture victory"));
  };

  return nullptr;
}

/**
   Autosaves setting names accessor.
 */
static const struct sset_val_name *autosaves_name(int autosaves_bit)
{
  switch (autosaves_bit) {
    NAME_CASE(AS_TURN, "TURN", N_("New turn"));
    NAME_CASE(AS_GAME_OVER, "GAMEOVER", N_("Game over"));
    NAME_CASE(AS_QUITIDLE, "QUITIDLE", N_("No player connections"));
    NAME_CASE(AS_INTERRUPT, "INTERRUPT", N_("Server interrupted"));
    NAME_CASE(AS_TIMER, "TIMER", N_("Timer"));
  };

  return nullptr;
}

/**
   Borders setting names accessor.
 */
static const struct sset_val_name *borders_name(int borders)
{
  switch (borders) {
    NAME_CASE(BORDERS_DISABLED, "DISABLED", N_("Disabled"));
    NAME_CASE(BORDERS_ENABLED, "ENABLED", N_("Enabled"));
    NAME_CASE(BORDERS_SEE_INSIDE, "SEE_INSIDE",
              N_("See everything inside borders"));
    NAME_CASE(BORDERS_EXPAND, "EXPAND",
              N_("Borders expand to unknown, revealing tiles"));
  }
  return nullptr;
}

/**
   Trait distribution setting names accessor.
 */
static const struct sset_val_name *trait_dist_name(int trait_dist)
{
  switch (trait_dist) {
    NAME_CASE(TDM_FIXED, "FIXED", N_("Fixed"));
    NAME_CASE(TDM_EVEN, "EVEN", N_("Even"));
  }
  return nullptr;
}

/**
   Player colors configuration setting names accessor.
 */
static const struct sset_val_name *plrcol_name(int plrcol)
{
  switch (plrcol) {
    NAME_CASE(PLRCOL_PLR_ORDER, "PLR_ORDER", N_("Per-player, in order"));
    NAME_CASE(PLRCOL_PLR_RANDOM, "PLR_RANDOM", N_("Per-player, random"));
    NAME_CASE(PLRCOL_PLR_SET, "PLR_SET", N_("Set manually"));
    NAME_CASE(PLRCOL_TEAM_ORDER, "TEAM_ORDER", N_("Per-team, in order"));
    NAME_CASE(PLRCOL_NATION_ORDER, "NATION_ORDER",
              N_("Per-nation, in order"));
  }
  return nullptr;
}

/**
   Happyborders setting names accessor.
 */
static const struct sset_val_name *happyborders_name(int happyborders)
{
  switch (happyborders) {
    NAME_CASE(HB_DISABLED, "DISABLED", N_("Borders are not helping"));
    NAME_CASE(HB_NATIONAL, "NATIONAL", N_("Happy within own borders"));
    NAME_CASE(HB_ALLIANCE, "ALLIED", N_("Happy within allied borders"));
  }
  return nullptr;
}

/**
   Diplomacy setting names accessor.
 */
static const struct sset_val_name *diplomacy_name(int diplomacy)
{
  switch (diplomacy) {
    NAME_CASE(DIPLO_FOR_ALL, "ALL", N_("Enabled for everyone"));
    NAME_CASE(DIPLO_FOR_HUMANS, "HUMAN",
              N_("Only allowed between human players"));
    NAME_CASE(DIPLO_FOR_AIS, "AI", N_("Only allowed between AI players"));
    NAME_CASE(DIPLO_NO_AIS, "NOAI", N_("Only allowed when human involved"));
    NAME_CASE(DIPLO_NO_MIXED, "NOMIXED",
              N_("Only allowed between two humans, or two AI players"));
    NAME_CASE(DIPLO_FOR_TEAMS, "TEAM", N_("Restricted to teams"));
    NAME_CASE(DIPLO_DISABLED, "DISABLED", N_("Disabled for everyone"));
  }
  return nullptr;
}

/**
   City names setting names accessor.
 */
static const struct sset_val_name *citynames_name(int citynames)
{
  switch (citynames) {
    NAME_CASE(CNM_NO_RESTRICTIONS, "NO_RESTRICTIONS", N_("No restrictions"));
    NAME_CASE(CNM_PLAYER_UNIQUE, "PLAYER_UNIQUE", N_("Unique to a player"));
    NAME_CASE(CNM_GLOBAL_UNIQUE, "GLOBAL_UNIQUE", N_("Globally unique"));
    NAME_CASE(CNM_NO_STEALING, "NO_STEALING", N_("No city name stealing"));
  }
  return nullptr;
}

/**
   Barbarian setting names accessor.
 */
static const struct sset_val_name *barbarians_name(int barbarians)
{
  switch (barbarians) {
    NAME_CASE(BARBS_DISABLED, "DISABLED", N_("No barbarians"));
    NAME_CASE(BARBS_HUTS_ONLY, "HUTS_ONLY", N_("Only in huts"));
    NAME_CASE(BARBS_NORMAL, "NORMAL", N_("Normal rate of appearance"));
    NAME_CASE(BARBS_FREQUENT, "FREQUENT", N_("Frequent barbarian uprising"));
    NAME_CASE(BARBS_HORDES, "HORDES", N_("Raging hordes"));
  }
  return nullptr;
}

/**
   Revolution length type setting names accessor.
 */
static const struct sset_val_name *revolentype_name(int revolentype)
{
  switch (revolentype) {
    NAME_CASE(REVOLEN_FIXED, "FIXED", N_("Fixed to 'revolen' turns"));
    NAME_CASE(REVOLEN_RANDOM, "RANDOM", N_("Randomly 1-'revolen' turns"));
    NAME_CASE(REVOLEN_QUICKENING, "QUICKENING",
              N_("First time 'revolen', then always quicker"));
    NAME_CASE(REVOLEN_RANDQUICK, "RANDQUICK",
              N_("Random, max always quicker"));
  }
  return nullptr;
}

/**
   Revealmap setting names accessor.
 */
static const struct sset_val_name *revealmap_name(int bit)
{
  switch (1 << bit) {
    NAME_CASE(REVEAL_MAP_START, "START", N_("Reveal map at game start"));
    NAME_CASE(REVEAL_MAP_DEAD, "DEAD", N_("Unfog map for dead players"));
  }
  return nullptr;
}

/**
   Airlifting style setting names accessor.
 */
static const struct sset_val_name *airliftingstyle_name(int bit)
{
  switch (1 << bit) {
    NAME_CASE(AIRLIFTING_ALLIED_SRC, "FROM_ALLIES",
              N_("Allows units to be airlifted from allied cities"));
    NAME_CASE(AIRLIFTING_ALLIED_DEST, "TO_ALLIES",
              N_("Allows units to be airlifted to allied cities"));
    NAME_CASE(AIRLIFTING_UNLIMITED_SRC, "SRC_UNLIMITED",
              N_("Unlimited units from source city"));
    NAME_CASE(AIRLIFTING_UNLIMITED_DEST, "DEST_UNLIMITED",
              N_("Unlimited units to destination city"));
  }
  return nullptr;
}

/**
   Phase mode names accessor.
 */
static const struct sset_val_name *phasemode_name(int phasemode)
{
  switch (phasemode) {
    NAME_CASE(PMT_CONCURRENT, "ALL", N_("All players move concurrently"));
    NAME_CASE(PMT_PLAYERS_ALTERNATE, "PLAYER",
              N_("All players alternate movement"));
    NAME_CASE(PMT_TEAMS_ALTERNATE, "TEAM", N_("Team alternate movement"));
  }
  return nullptr;
}

/**
   Scorelog level names accessor.
 */
static const struct sset_val_name *
scoreloglevel_name(enum scoreQtMsgType sl_level)
{
  switch (sl_level) {
    NAME_CASE(SL_ALL, "ALL", N_("All players"));
    NAME_CASE(SL_HUMANS, "HUMANS", N_("Human players only"));
  }
  return nullptr;
}

/**
   Savegame compress type names accessor.
 */
static const struct sset_val_name *
compresstype_name(enum compress_type compresstype)
{
  switch (compresstype) {
    NAME_CASE(COMPRESS_PLAIN, "PLAIN", N_("No compression"));
    NAME_CASE(COMPRESS_ZLIB, "LIBZ", N_("Using zlib (gzip format)"));
#ifdef FREECIV_HAVE_BZ2
    NAME_CASE(COMPRESS_BZIP2, "BZIP2", N_("Using bzip2 (deprecated)"));
#endif
#ifdef FREECIV_HAVE_LZMA
    NAME_CASE(COMPRESS_XZ, "XZ", N_("Using xz"));
#endif
#ifdef FREECIV_HAVE_ZSTD
    NAME_CASE(COMPRESS_ZSTD, "ZSTD", N_("Using Zstandard"));
#endif
  }
  return nullptr;
}

/**
   Names accessor for boolean settings (disable/enable).
 */
static const struct sset_val_name *bool_name(int enable)
{
  switch (enable) {
    NAME_CASE(false, "DISABLED", N_("disabled"));
    NAME_CASE(true, "ENABLED", N_("enabled"));
  }
  return nullptr;
}

/**
 * Unitwaittime style names accessor.
 */
static const struct sset_val_name *unitwaittime_name(int bit)
{
  switch (1 << bit) {
    NAME_CASE(UWT_ACTIVITIES, "ACTIVITIES",
              N_("Wait time applies to activity completion at turn change"));
  }
  return NULL;
}

#undef NAME_CASE

/*
 * Help callback functions.
 */

/**
   Help about phasemode setting
 */
static const char *phasemode_help(const struct setting *pset)
{
  static char pmhelp[512];

  // Translated here
  fc_snprintf(
      pmhelp, sizeof(pmhelp),
      _("This setting controls whether players may make moves at the same "
        "time during a turn. Change in setting takes effect next turn. "
        "Currently, at least to the end of this turn, mode is \"%s\"."),
      phasemode_name(game.info.phase_mode)->pretty);

  return pmhelp;
}

/**
   Help about huts setting
 */
static const char *huts_help(const struct setting *pset)
{
  if (wld.map.server.huts_absolute >= 0) {
    static char hutshelp[512];

    // Translated here
    fc_snprintf(hutshelp, sizeof(hutshelp),
                _("%s\n"
                  "Currently this setting is being overridden by an old "
                  "scenario or savegame, which has set the absolute number "
                  "of huts to %d. Explicitly set this setting again to make "
                  "it take effect instead."),
                _(pset->extra_help), wld.map.server.huts_absolute);

    return hutshelp;
  }

  return pset->extra_help;
}

/*
 * Action callback functions.
 */

/**
   (De)initialze the score log.
 */
static void scorelog_action(const struct setting *pset)
{
  if (*pset->boolean.pvalue) {
    log_civ_score_init();
  } else {
    log_civ_score_free();
  }
}

/**
   Create the selected number of AI's.
 */
static void aifill_action(const struct setting *pset)
{
  const char *msg = aifill(*pset->integer.pvalue);
  if (msg) {
    qInfo(_("Warning: aifill not met: %s."), msg);
    notify_conn(nullptr, nullptr, E_SETTING, ftc_server,
                _("Warning: aifill not met: %s."), msg);
  }
}

/**
   Restrict to the selected nation set.
 */
static void nationset_action(const struct setting *pset)
{
  // If any player's existing selection is invalid, abort it
  players_iterate(pplayer)
  {
    if (pplayer->nation != nullptr) {
      if (!nation_is_in_current_set(pplayer->nation)) {
        (void) player_set_nation(pplayer, NO_NATION_SELECTED);
        send_player_info_c(pplayer, game.est_connections);
      }
    }
  }
  players_iterate_end;
  count_playable_nations();
  (void) aifill(game.info.aifill);

  /* There might now be too many players for the available nations. Rather
   * than getting rid of some players arbitrarily, we let the situation
   * persist for all already-connected players; the server will simply refuse
   * to start until someone reduces the number of players. This policy also
   * avoids annoyance if nationset is accidentally and transiently set to an
   * unintended value. (However, new connections will start out detached.) */
  if (normal_player_count() > server.playable_nations) {
    notify_conn(nullptr, nullptr, E_SETTING, ftc_server, "%s",
                _("Warning: not enough nations in this nation set for all "
                  "current players."));
  }

  send_nation_availability(game.est_connections, true);
}

/**
   Clear any user-set player colors in modes other than PLRCOL_PLR_SET.
 */
static void plrcol_action(const struct setting *pset)
{
  if (!game_was_started()) {
    if (read_enum_value(pset) != PLRCOL_PLR_SET) {
      players_iterate(pplayer) { server_player_set_color(pplayer, nullptr); }
      players_iterate_end;
    }
    // Update clients with new color scheme.
    send_player_info_c(nullptr, nullptr);
  }
}

/**
   Toggle player AI status.
 */
static void autotoggle_action(const struct setting *pset)
{
  if (*pset->boolean.pvalue) {
    players_iterate(pplayer)
    {
      if (is_human(pplayer) && !pplayer->is_connected) {
        toggle_ai_player_direct(nullptr, pplayer);
        send_player_info_c(pplayer, game.est_connections);
      }
    }
    players_iterate_end;
  }
}

/**
   Enact a change in the 'timeout' server setting immediately, if the game
   is afoot.
 */
static void timeout_action(const struct setting *pset)
{
  if (S_S_RUNNING == server_state()) {
    int timeout = *pset->integer.pvalue;

    if (game.info.turn != 1 || game.info.first_timeout == -1) {
      // This may cause the current turn to end immediately.
      game.tinfo.seconds_to_phasedone = timeout;
    }
    send_game_info(nullptr);
  }
}

/**
   Enact a change in the 'first_timeout' server setting immediately, if the
 game is afoot.
 */
static void first_timeout_action(const struct setting *pset)
{
  if (S_S_RUNNING == server_state()) {
    int timeout = *pset->integer.pvalue;

    if (game.info.turn == 1) {
      // This may cause the current turn to end immediately.
      if (timeout != -1) {
        game.tinfo.seconds_to_phasedone = timeout;
      } else {
        game.tinfo.seconds_to_phasedone = game.info.timeout;
      }
    }
    send_game_info(nullptr);
  }
}

/**
   Clean out absolute number of huts when relative setting set.
 */
static void huts_action(const struct setting *pset)
{
  wld.map.server.huts_absolute = -1;
}

/**
   Topology setting changed.
 */
static void topology_action(const struct setting *pset)
{
  struct packet_set_topology packet;

  packet.topology_id = *pset->integer.pvalue;

  conn_list_iterate(game.est_connections, pconn)
  {
    send_packet_set_topology(pconn, &packet);
  }
  conn_list_iterate_end;
}

/**
   Update metaserver message string from changed user meta server message
   string.
 */
static void metamessage_action(const struct setting *pset)
{
  /* Set the metaserver message based on the new meta server user message. An
   * empty user metaserver message results in an automatic meta message. A
   * non empty user meta message results in the user meta message. */
  set_user_meta_message_string(pset->string.value);

  if (is_metaserver_open()) {
    // Update the meta server.
    send_server_info_to_metaserver(META_INFO);
  }
}

/*
 * Validation callback functions.
 */

/**
   Verify the selected savename definition.
 */
static bool savename_validate(const char *value, struct connection *caller,
                              char *reject_msg, size_t reject_msg_len)
{
  char buf[MAX_LEN_PATH];

  generate_save_name(value, buf, sizeof(buf), nullptr);

  if (!is_safe_filename(buf)) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("Invalid save name definition: '%s' (resolves to '%s')."), value,
        buf);
    return false;
  }

  return true;
}

/**
   Verify the value of the generator option (notably the MAPGEN_SCENARIO
   case).
 */
static bool generator_validate(int value, struct connection *caller,
                               char *reject_msg, size_t reject_msg_len)
{
  if (map_is_empty()) {
    if (MAPGEN_SCENARIO == value
        && (nullptr != caller || !game.scenario.is_scenario)) {
      settings_snprintf(reject_msg, reject_msg_len,
                        _("You cannot disable the map generator."));
      return false;
    }
    return true;
  } else {
    if (MAPGEN_SCENARIO != value) {
      settings_snprintf(
          reject_msg, reject_msg_len,
          _("You cannot require a map generator when a map is loaded."));
      return false;
    }
  }
  return true;
}

/**
   Verify the name for the score log file.
 */
static bool scorefile_validate(const char *value, struct connection *caller,
                               char *reject_msg, size_t reject_msg_len)
{
  if (!is_safe_filename(value)) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Invalid score name definition: '%s'."), value);
    return false;
  }

  return true;
}

/**
   Verify that a given demography string is valid. See
   game.demography.
 */
static bool demography_callback(const char *value, struct connection *caller,
                                char *reject_msg, size_t reject_msg_len)
{
  int error;

  if (is_valid_demography(value, &error)) {
    return true;
  } else {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Demography string validation failed at character: "
                        "'%c'. Try \"/help demography\"."),
                      value[error]);
    return false;
  }
}

/**
   Autosaves setting callback
 */
static bool autosaves_callback(unsigned value, struct connection *caller,
                               char *reject_msg, size_t reject_msg_len)
{
  if (S_S_RUNNING == server_state()) {
    if ((value & (1 << AS_TIMER))
        && !(game.server.autosaves & (1 << AS_TIMER))) {
      game.server.save_timer =
          timer_renew(game.server.save_timer, TIMER_USER, TIMER_ACTIVE);
      timer_start(game.server.save_timer);
    } else if (!(value & (1 << AS_TIMER))
               && (game.server.autosaves & (1 << AS_TIMER))) {
      timer_stop(game.server.save_timer);
      timer_destroy(game.server.save_timer);
      game.server.save_timer = nullptr;
    }
  }

  return true;
}

/**
   Verify that a given allowtake string is valid.  See
   game.allow_take.
 */
static bool allowtake_callback(const char *value, struct connection *caller,
                               char *reject_msg, size_t reject_msg_len)
{
  int len = qstrlen(value), i;
  bool havecharacter_state = false;

  /* We check each character individually to see if it's valid.  This does
   * not check for duplicate entries.
   * We also track the state of the machine.  havecharacter_state is true if
   * the preceeding character was a primary label, e.g. NHhAadb.  It is false
   * if the preceeding character was a modifier or if this is the first
   * character. */

  for (i = 0; i < len; i++) {
    // Check to see if the character is a primary label.
    if (strchr("HhAadbOo", value[i])) {
      havecharacter_state = true;
      continue;
    }

    /* If we've already passed a primary label, check to see if the character
     * is a modifier. */
    if (havecharacter_state && strchr("1234", value[i])) {
      havecharacter_state = false;
      continue;
    }

    // Looks like the character was invalid.
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Allowed take string validation failed at "
                        "character: '%c'. Try \"/help allowtake\"."),
                      value[i]);
    return false;
  }

  // All characters were valid.
  return true;
}

/**
   Verify that a given startunits string is valid. See
   game.server.start_units.
 */
static bool startunits_callback(const char *value, struct connection *caller,
                                char *reject_msg, size_t reject_msg_len)
{
  int len = qstrlen(value), i;
  Unit_Class_id first_role;
  bool firstnative = false;

  // We check each character individually to see if it's valid.
  for (i = 0; i < len; i++) {
    if (strchr("cwxksfdDaA", value[i])) {
      continue;
    }

    // Looks like the character was invalid.
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Starting units string validation failed at "
                        "character '%c'. Try \"/help startunits\"."),
                      value[i]);
    return false;
  }

  // Check the first character to make sure it can use a startpos.
  first_role = uclass_index(
      utype_class(get_role_unit(crole_to_role_id(value[0]), 0)));
  terrain_type_iterate(pterrain)
  {
    if (terrain_has_flag(pterrain, TER_STARTER)
        && BV_ISSET(pterrain->native_to, first_role)) {
      firstnative = true;
      break;
    }
  }
  terrain_type_iterate_end;

  if (!firstnative) {
    // Loading would cause an infinite loop hunting for a valid startpos.
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The first starting unit must be native to at least one "
          "\"Starter\" terrain. Try \"/help startunits\"."));
    return false;
  }

  // Everything seems fine.
  return true;
}

/**
   Verify that a given endturn is valid.
 */
static bool endturn_callback(int value, struct connection *caller,
                             char *reject_msg, size_t reject_msg_len)
{
  if (value < game.info.turn) {
    // Tried to set endturn earlier than current turn
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Cannot set endturn earlier than current turn."));
    return false;
  }
  return true;
}

/**
   Verify that a given maxplayers is valid.
 */
static bool maxplayers_callback(int value, struct connection *caller,
                                char *reject_msg, size_t reject_msg_len)
{
  if (value < player_count()) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Number of players (%d) is higher than requested "
                        "value (%d). Keeping old value."),
                      player_count(), value);
    return false;
  }
  /* If any start positions are defined by a scenario, we can only
   * accommodate as many players as we have start positions. */
  if (0 < map_startpos_count() && value > map_startpos_count()) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("Requested value (%d) is greater than number of available start "
          "positions (%d). Keeping old value."),
        value, map_startpos_count());
    return false;
  }

  return true;
}

/**
   Validate the 'nationset' server setting.
 */
static bool nationset_callback(const char *value, struct connection *caller,
                               char *reject_msg, size_t reject_msg_len)
{
  if (strlen(value) == 0) {
    return true;
  } else if (nation_set_by_rule_name(value)) {
    return true;
  } else {
    settings_snprintf(reject_msg, reject_msg_len,
                      // TRANS: do not translate 'list nationsets'
                      _("Unknown nation set \"%s\". See '%slist nationsets' "
                        "for possible values."),
                      value, caller ? "/" : "");
    return false;
  }
}

/**
   Validate the 'timeout' server setting.
 */
static bool timeout_callback(int value, struct connection *caller,
                             char *reject_msg, size_t reject_msg_len)
{
  // Disallow low timeout values for non-hack connections.
  if (caller && caller->access_level < ALLOW_HACK && value < 30
      && value != 0) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("You are not allowed to set timeout values less "
                        "than 30 seconds."));
    return false;
  }

  if (value == -1 && game.server.unitwaittime != 0) {
    // autogame only with 'unitwaittime' = 0
    settings_snprintf(reject_msg, reject_msg_len,
                      // TRANS: Do not translate setting names in ''.
                      _("For autogames ('timeout' = -1) 'unitwaittime' "
                        "should be deactivated (= 0)."));
    return false;
  }

  if (value > 0 && value < game.server.unitwaittime * 3 / 2) {
    /* for normal games 'timeout' should be at least 3/2 times the value of
     * 'unitwaittime' */
    settings_snprintf(
        reject_msg, reject_msg_len,
        // TRANS: Do not translate setting names in ''.
        _("'timeout' can not be lower than 3/2 of the 'unitwaittime' "
          "setting (= %d). Please change 'unitwaittime' first."),
        game.server.unitwaittime);
    return false;
  }

  return true;
}

/**
   Validate the 'first_timeout' server setting.
 */
static bool first_timeout_callback(int value, struct connection *caller,
                                   char *reject_msg, size_t reject_msg_len)
{
  // Disallow low timeout values for non-hack connections.
  if (caller && caller->access_level < ALLOW_HACK && value < 30
      && value != 0) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("You are not allowed to set timeout values less "
                        "than 30 seconds."));
    return false;
  }

  return true;
}

/**
   Check 'timeout' setting if 'unitwaittime' is changed.
 */
static bool unitwaittime_callback(int value, struct connection *caller,
                                  char *reject_msg, size_t reject_msg_len)
{
  if (game.info.timeout == -1 && value != 0) {
    settings_snprintf(reject_msg, reject_msg_len,
                      // TRANS: Do not translate setting names in ''.
                      _("For autogames ('timeout' = -1) 'unitwaittime' "
                        "should be deactivated (= 0)."));
    return false;
  }

  if (game.info.timeout > 0 && value > game.info.timeout * 2 / 3) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        // TRANS: Do not translate setting names in ''.
        _("'unitwaittime' has to be lower than 2/3 of the 'timeout' setting "
          "(= %d). Please change 'timeout' first."),
        game.info.timeout);
    return false;
  }

  return true;
}

/**
   Mapsize setting validation callback.
 */
static bool mapsize_callback(int value, struct connection *caller,
                             char *reject_msg, size_t reject_msg_len)
{
  if (value == MAPSIZE_XYSIZE && MAP_IS_ISOMETRIC
      && wld.map.ysize % 2 != 0) {
    /* An isometric map needs a even ysize. It is calculated automatically
     * for all settings but mapsize=XYSIZE. */
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("For an isometric or hexagonal map the ysize must be even."));
    return false;
  }

  return true;
}

/**
   xsize setting validation callback.
 */
static bool xsize_callback(int value, struct connection *caller,
                           char *reject_msg, size_t reject_msg_len)
{
  int size = value * wld.map.ysize;

  if (size < MAP_MIN_SIZE * 1000) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The map size (%d * %d = %d) must be larger than %d tiles."),
        value, wld.map.ysize, size, MAP_MIN_SIZE * 1000);
    return false;
  } else if (size > MAP_MAX_SIZE * 1000) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The map size (%d * %d = %d) must be lower than %d tiles."), value,
        wld.map.ysize, size, MAP_MAX_SIZE * 1000);
    return false;
  }

  return true;
}

/**
   ysize setting validation callback.
 */
static bool ysize_callback(int value, struct connection *caller,
                           char *reject_msg, size_t reject_msg_len)
{
  int size = wld.map.xsize * value;

  if (size < MAP_MIN_SIZE * 1000) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The map size (%d * %d = %d) must be larger than %d tiles."),
        wld.map.xsize, value, size, MAP_MIN_SIZE * 1000);
    return false;
  } else if (size > MAP_MAX_SIZE * 1000) {
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The map size (%d * %d = %d) must be lower than %d tiles."),
        wld.map.xsize, value, size, MAP_MAX_SIZE * 1000);
    return false;
  } else if (wld.map.server.mapsize == MAPSIZE_XYSIZE && MAP_IS_ISOMETRIC
             && value % 2 != 0) {
    /* An isometric map needs a even ysize. It is calculated automatically
     * for all settings but mapsize=XYSIZE. */
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("For an isometric or hexagonal map the ysize must be even."));
    return false;
  }

  return true;
}

/**
   Topology setting validation callback.
 */
static bool topology_callback(unsigned value, struct connection *caller,
                              char *reject_msg, size_t reject_msg_len)
{
  if (wld.map.server.mapsize == MAPSIZE_XYSIZE
      && ((value & (TF_ISO)) != 0 || (value & (TF_HEX)) != 0)
      && wld.map.ysize % 2 != 0) {
    /* An isometric map needs a even ysize. It is calculated automatically
     * for all settings but mapsize=XYSIZE. */
    settings_snprintf(
        reject_msg, reject_msg_len,
        _("For an isometric or hexagonal map the ysize must be even."));
    return false;
  }

  return true;
}

/**
   Warn about deprecated compresstype selection.
 */
static bool compresstype_callback(int value, struct connection *caller,
                                  char *reject_msg, size_t reject_msg_len)
{
#ifdef FREECIV_HAVE_BZ2
  if (value == COMPRESS_BZIP2) {
    qWarning(
        _("Bzip2 is deprecated as compresstype. Consider other options."));
  }
#endif // FREECIV_HAVE_BZ2

  return true;
}

/**
   Validate that the player color mode can be used.
 */
static bool plrcol_validate(int value, struct connection *caller,
                            char *reject_msg, size_t reject_msg_len)
{
  plrcolor_mode mode = plrcolor_mode(value);
  if (mode == PLRCOL_NATION_ORDER) {
    for (const auto &pnation : nations) {
      if (nation_color(&pnation)) {
        // At least one nation has a color. Allow this mode.
        return true;
      }
    }
    settings_snprintf(reject_msg, reject_msg_len,
                      _("No nations in the currently loaded ruleset have "
                        "associated colors."));
    return false;
  }
  return true;
}

#define GEN_BOOL(name, value, sclass, scateg, slevel, al_read, al_write,    \
                 short_help, extra_help, func_validate, func_action,        \
                 _default)                                                  \
  {                                                                         \
    name, sclass, al_read, al_write, short_help, extra_help, nullptr,       \
        SST_BOOL, scateg, slevel,                                           \
        {.boolean = {&value, _default, func_validate, bool_name, false}},   \
        func_action, false                                                  \
  }

#define GEN_INT(name, value, sclass, scateg, slevel, al_read, al_write,     \
                short_help, extra_help, func_help, func_validate,           \
                func_action, _min, _max, _default)                          \
  {                                                                         \
    name, sclass, al_read, al_write, short_help, extra_help, func_help,     \
        SST_INT, scateg, slevel,                                            \
        {.integer = {(int *) &value, _default, _min, _max, func_validate,   \
                     0}},                                                   \
        func_action, false                                                  \
  }

#define GEN_STRING(name, value, sclass, scateg, slevel, al_read, al_write,  \
                   short_help, extra_help, func_validate, func_action,      \
                   _default)                                                \
  {                                                                         \
    name, sclass, al_read, al_write, short_help, extra_help, nullptr,       \
        SST_STRING, scateg, slevel,                                         \
        {.string = {value, _default, sizeof(value), func_validate,          \
                    (char *) ""}},                                          \
        func_action, false                                                  \
  }

#define GEN_ENUM(name, value, sclass, scateg, slevel, al_read, al_write,    \
                 short_help, extra_help, func_help, func_validate,          \
                 func_action, func_name, _default)                          \
  {                                                                         \
    name, sclass, al_read, al_write, short_help, extra_help, func_help,     \
        SST_ENUM, scateg, slevel,                                           \
        {.enumerator = {&value, sizeof(value), _default, func_validate,     \
                        (val_name_func_t) func_name, 0}},                   \
        func_action, false                                                  \
  }

#define GEN_BITWISE(name, value, sclass, scateg, slevel, al_read, al_write, \
                    short_help, extra_help, func_validate, func_action,     \
                    func_name, _default)                                    \
  {                                                                         \
    name, sclass, al_read, al_write, short_help, extra_help, nullptr,       \
        SST_BITWISE, scateg, slevel,                                        \
        {.bitwise = {(unsigned *) (void *) &value, _default, func_validate, \
                     func_name, 0}},                                        \
        func_action, false                                                  \
  }

// game settings
static struct setting settings[] = {
    // These should be grouped by sclass

    // Map size parameters: adjustable if we don't yet have a map
    GEN_ENUM(
        "mapsize", wld.map.server.mapsize, SSET_MAP_SIZE, SSET_GEOLOGY,
        SSET_VITAL, ALLOW_NONE, ALLOW_BASIC, N_("Map size definition"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("Chooses the method used to define the map size. Other options "
           "specify the parameters for each method.\n"
           "- \"Number of tiles\" (FULLSIZE): Map area (option 'size').\n"
           "- \"Tiles per player\" (PLAYER): Number of (land) tiles per "
           "player (option 'tilesperplayer').\n"
           "- \"Width and height\" (XYSIZE): Map width and height in tiles "
           "(options 'xsize' and 'ysize')."),
        nullptr, mapsize_callback, nullptr, mapsize_name,
        MAP_DEFAULT_MAPSIZE),

    GEN_INT(
        "size", wld.map.server.size, SSET_MAP_SIZE, SSET_GEOLOGY, SSET_VITAL,
        ALLOW_NONE, ALLOW_BASIC, N_("Map area (in thousands of tiles)"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("This value is used to determine the map area.\n"
           "  size = 4 is a normal map of 4,000 tiles (default)\n"
           "  size = 20 is a huge map of 20,000 tiles\n"
           "For this option to take effect, the \"Map size definition\" "
           "option ('mapsize') must be set to \"Number of tiles\" "
           "(FULLSIZE)."),
        nullptr, nullptr, nullptr, MAP_MIN_SIZE, MAP_MAX_SIZE,
        MAP_DEFAULT_SIZE),

    GEN_INT(
        "tilesperplayer", wld.map.server.tilesperplayer, SSET_MAP_SIZE,
        SSET_GEOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Number of (land) tiles per player"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("This value is used to determine the map dimensions. It "
           "calculates the map size at game start based on the number of "
           "players and the value of the setting 'landmass'.\n"
           "For this option to take effect, the \"Map size definition\" "
           "option ('mapsize') must be set to \"Tiles per player\" "
           "(PLAYER)."),
        nullptr, nullptr, nullptr, MAP_MIN_TILESPERPLAYER,
        MAP_MAX_TILESPERPLAYER, MAP_DEFAULT_TILESPERPLAYER),

    GEN_INT(
        "xsize", wld.map.xsize, SSET_MAP_SIZE, SSET_GEOLOGY, SSET_VITAL,
        ALLOW_NONE, ALLOW_BASIC, N_("Map width in tiles"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("Defines the map width.\n"
           "For this option to take effect, the \"Map size definition\" "
           "option ('mapsize') must be set to \"Width and height\" "
           "(XYSIZE)."),
        nullptr, xsize_callback, nullptr, MAP_MIN_LINEAR_SIZE,
        MAP_MAX_LINEAR_SIZE, MAP_DEFAULT_LINEAR_SIZE),
    GEN_INT(
        "ysize", wld.map.ysize, SSET_MAP_SIZE, SSET_GEOLOGY, SSET_VITAL,
        ALLOW_NONE, ALLOW_BASIC, N_("Map height in tiles"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("Defines the map height.\n"
           "For this option to take effect, the \"Map size definition\" "
           "option ('mapsize') must be set to \"Width and height\" "
           "(XYSIZE)."),
        nullptr, ysize_callback, nullptr, MAP_MIN_LINEAR_SIZE,
        MAP_MAX_LINEAR_SIZE, MAP_DEFAULT_LINEAR_SIZE),

    GEN_BITWISE(
        "topology", wld.map.topology_id, SSET_MAP_SIZE, SSET_GEOLOGY,
        SSET_VITAL, ALLOW_NONE, ALLOW_BASIC, N_("Map topology"),
        // TRANS: do not edit the ugly ASCII art
        N_("Freeciv21 maps are always two-dimensional. They may wrap at the "
           "north-south and east-west directions to form a flat map, a "
           "cylinder, or a torus (donut). Individual tiles may be "
           "rectangular or hexagonal, with either an overhead (\"classic\") "
           "or isometric alignment.\n"
           "To play with a particular topology, clients will need a "
           "matching tileset.\n"
           "Overhead rectangular:      Isometric rectangular:\n"
           "      _________               /\\/\\/\\/\\/\\\n"
           "     |_|_|_|_|_|             /\\/\\/\\/\\/\\/\n"
           "     |_|_|_|_|_|             \\/\\/\\/\\/\\/\\\n"
           "     |_|_|_|_|_|             /\\/\\/\\/\\/\\/\n"
           "                             \\/\\/\\/\\/\\/\n"
           "Hex:                       Iso-hex:\n"
           "  /\\/\\/\\/\\/\\/\\               _   _   _   _   _\n"
           "  | | | | | | |             / \\_/ \\_/ \\_/ \\_/ \\\n"
           "  \\/\\/\\/\\/\\/\\/\\"
           "             \\_/ \\_/ \\_/ \\_/ \\_/\n"
           "   | | | | | | |            / \\_/ \\_/ \\_/ \\_/ \\\n"
           "   \\/\\/\\/\\/\\/\\/             \\_/ \\_/ \\_/ \\_/ \\_/\n"),
        topology_callback, topology_action, topology_name, MAP_DEFAULT_TOPO),

    GEN_ENUM(
        "generator", wld.map.server.generator, SSET_MAP_GEN, SSET_GEOLOGY,
        SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Method used to generate map"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single
           quotes (except 'fair') are setting names and shouldn't be
           translated. The strings between parentheses and in uppercase
           must stay as untranslated. */
        N_("Specifies the algorithm used to generate the map. If the "
           "default value of the 'startpos' option is used, then the chosen "
           "generator chooses an appropriate 'startpos' setting; otherwise, "
           "the generated map tries to accommodate the chosen 'startpos' "
           "setting.\n"
           "- \"Scenario map\" (SCENARIO): indicates a pre-generated map. "
           "By default, if the scenario does not specify start positions, "
           "they will be allocated depending on the size of continents.\n"
           "- \"Fully random height\" (RANDOM): generates maps with a "
           "number of equally spaced, relatively small islands. By default, "
           "start positions are allocated depending on continent size.\n"
           "- \"Pseudo-fractal height\" (FRACTAL): generates Earthlike "
           "worlds with one or more large continents and a scattering of "
           "smaller islands. By default, players are all placed on a single "
           "continent.\n"
           "- \"Island-based\" (ISLAND): generates 'fair' maps with a "
           "number of similarly-sized and -shaped islands, each with "
           "approximately the same ratios of terrain types. By default, "
           "each player gets their own island.\n"
           "- \"Fair islands\" (FAIR): generates the exact copy of the same "
           "island for every player or every team.\n"
           "- \"Fracture map\" (FRACTURE): generates maps from a fracture "
           "pattern. Tends to place hills and mountains along the edges of "
           "the continents.\n"
           "If the requested generator is incompatible with other server "
           "settings, the server may fall back to another generator."),
        nullptr, generator_validate, nullptr, generator_name,
        MAP_DEFAULT_GENERATOR),

    GEN_ENUM(
        "startpos", wld.map.server.startpos, SSET_MAP_GEN, SSET_GEOLOGY,
        SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Method used to choose start positions"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single
           quotes (except 'best') are setting names and shouldn't be
           translated. The strings between parentheses and in uppercase
           must stay as untranslated. */
        N_("The method used to choose where each player's initial units "
           "start on the map. (For scenarios which include pre-set start "
           "positions, this setting is ignored.)\n"
           "- \"Generator's choice\" (DEFAULT): the start position "
           "placement will depend on the map generator chosen. See the "
           "'generator' setting.\n"
           "- \"One player per continent\" (SINGLE): one player is placed "
           "on each of a set of continents of approximately equivalent "
           "value (if possible).\n"
           "- \"Two or three players per continent\" (2or3): similar to "
           "SINGLE except that two players will be placed on each "
           "continent, with three on the 'best' continent if there is an "
           "odd number of players.\n"
           "- \"All players on a single continent\" (ALL): all players will "
           "start on the 'best' available continent.\n"
           "- \"Depending on size of continents\" (VARIABLE): players will "
           "be placed on the 'best' available continents such that, as far "
           "as possible, the number of players on each continent is "
           "proportional to its value.\n"
           "If the server cannot satisfy the requested setting due to there "
           "being too many players for continents, it may fall back to one "
           "of the others. (However, map generators try to create the right "
           "number of continents for the choice of this 'startpos' setting "
           "and the number of players, so this is unlikely to occur.)"),
        nullptr, nullptr, nullptr, startpos_name, MAP_DEFAULT_STARTPOS),

    GEN_ENUM(
        "teamplacement", wld.map.server.team_placement, SSET_MAP_GEN,
        SSET_GEOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Method used for placement of team mates"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("After start positions have been generated thanks to the "
           "'startpos' setting, this setting controls how the start "
           "positions will be assigned to the different players of the same "
           "team.\n"
           "- \"Disabled\" (DISABLED): the start positions will be randomly "
           "assigned to players, regardless of teams.\n"
           "- \"As close as possible\" (CLOSEST): players will be placed as "
           "close as possible, regardless of continents.\n"
           "- \"On the same continent\" (CONTINENT): if possible, place all "
           "players of the same team onto the same island/continent.\n"
           "- \"Horizontal placement\" (HORIZONTAL): players of the same "
           "team will be placed horizontally.\n"
           "- \"Vertical placement\" (VERTICAL): players of the same team "
           "will be placed vertically."),
        nullptr, nullptr, nullptr, teamplacement_name,
        MAP_DEFAULT_TEAM_PLACEMENT),

    GEN_BOOL("tinyisles", wld.map.server.tinyisles, SSET_MAP_GEN,
             SSET_GEOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Presence of 1x1 islands"),
             N_("This setting controls whether the map generator is allowed "
                "to make islands of one only tile size."),
             nullptr, nullptr, MAP_DEFAULT_TINYISLES),

    GEN_BOOL("separatepoles", wld.map.server.separatepoles, SSET_MAP_GEN,
             SSET_GEOLOGY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Whether the poles are separate continents"),
             N_("If this setting is disabled, the continents may attach to "
                "poles."),
             nullptr, nullptr, MAP_DEFAULT_SEPARATE_POLES),

    GEN_INT(
        "flatpoles", wld.map.server.flatpoles, SSET_MAP_GEN, SSET_GEOLOGY,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("How much the land at the poles is flattened"),
        /* TRANS: The strings in
           quotes shouldn't be
           translated. */
        N_("Controls how much the height of the poles is flattened during "
           "map generation, preventing a diversity of land terrain there. 0 "
           "is no flattening, 100 is maximum flattening. Only affects the "
           "'RANDOM' and 'FRACTAL' map generators."),
        nullptr, nullptr, nullptr, MAP_MIN_FLATPOLES, MAP_MAX_FLATPOLES,
        MAP_DEFAULT_FLATPOLES),

    GEN_BOOL("singlepole", wld.map.server.single_pole, SSET_MAP_GEN,
             SSET_GEOLOGY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Whether there's just one pole generated"),
             N_("If this setting is enabled, only one side of the map will "
                "have a pole. This setting has no effect if the map wraps "
                "both directions."),
             nullptr, nullptr, MAP_DEFAULT_SINGLE_POLE),

    GEN_BOOL("alltemperate", wld.map.server.alltemperate, SSET_MAP_GEN,
             SSET_GEOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("All the map is temperate"),
             N_("If this setting is enabled, the temperature will be "
                "equivalent everywhere on the map. As a result, the poles "
                "won't be generated."),
             nullptr, nullptr, MAP_DEFAULT_ALLTEMPERATE),

    GEN_INT("temperature", wld.map.server.temperature, SSET_MAP_GEN,
            SSET_GEOLOGY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Average temperature of the planet"),
            N_("Small values will give a cold map, while larger values will "
               "give a hotter map.\n"
               "\n"
               "100 means a very dry and hot planet with no polar arctic "
               "zones, only tropical and dry zones.\n"
               " 70 means a hot planet with little polar ice.\n"
               " 50 means a temperate planet with normal polar, cold, "
               "temperate, and tropical zones; a desert zone overlaps "
               "tropical and temperate zones.\n"
               " 30 means a cold planet with small tropical zones.\n"
               "  0 means a very cold planet with large polar zones and no "
               "tropics."),
            nullptr, nullptr, nullptr, MAP_MIN_TEMPERATURE,
            MAP_MAX_TEMPERATURE, MAP_DEFAULT_TEMPERATURE),

    GEN_INT("landmass", wld.map.server.landpercent, SSET_MAP_GEN,
            SSET_GEOLOGY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Percentage of the map that is land"),
            N_("This setting gives the approximate percentage of the map "
               "that will be made into land."),
            nullptr, nullptr, nullptr, MAP_MIN_LANDMASS, MAP_MAX_LANDMASS,
            MAP_DEFAULT_LANDMASS),

    GEN_INT("steepness", wld.map.server.steepness, SSET_MAP_GEN,
            SSET_GEOLOGY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Amount of hills/mountains"),
            N_("Small values give flat maps, while higher values give a "
               "steeper map with more hills and mountains."),
            nullptr, nullptr, nullptr, MAP_MIN_STEEPNESS, MAP_MAX_STEEPNESS,
            MAP_DEFAULT_STEEPNESS),

    GEN_INT(
        "wetness", wld.map.server.wetness, SSET_MAP_GEN, SSET_GEOLOGY,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Amount of water on landmasses"),
        N_("Small values mean lots of dry, desert-like land; higher values "
           "give a wetter map with more swamps, jungles, and rivers."),
        nullptr, nullptr, nullptr, MAP_MIN_WETNESS, MAP_MAX_WETNESS,
        MAP_DEFAULT_WETNESS),

    GEN_BOOL("globalwarming", game.info.global_warming, SSET_RULES,
             SSET_GEOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Global warming"),
             N_("If turned off, global warming will not occur as a result "
                "of pollution. This setting does not affect pollution."),
             nullptr, nullptr, GAME_DEFAULT_GLOBAL_WARMING),

    GEN_INT("globalwarming_percent", game.server.global_warming_percent,
            SSET_RULES, SSET_GEOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Global warming percent"),
            N_("This is a multiplier for the rate of accumulation of global "
               "warming."),
            nullptr, nullptr, nullptr, GAME_MIN_GLOBAL_WARMING_PERCENT,
            GAME_MAX_GLOBAL_WARMING_PERCENT,
            GAME_DEFAULT_GLOBAL_WARMING_PERCENT),

    GEN_BOOL("nuclearwinter", game.info.nuclear_winter, SSET_RULES,
             SSET_GEOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Nuclear winter"),
             N_("If turned off, nuclear winter will not occur as a result "
                "of nuclear war."),
             nullptr, nullptr, GAME_DEFAULT_NUCLEAR_WINTER),

    GEN_INT("nuclearwinter_percent", game.server.nuclear_winter_percent,
            SSET_RULES, SSET_GEOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Nuclear winter percent"),
            N_("This is a multiplier for the rate of accumulation of "
               "nuclear winter."),
            nullptr, nullptr, nullptr, GAME_MIN_NUCLEAR_WINTER_PERCENT,
            GAME_MAX_NUCLEAR_WINTER_PERCENT,
            GAME_DEFAULT_NUCLEAR_WINTER_PERCENT),

    GEN_INT("mapseed", wld.map.server.seed_setting, SSET_MAP_GEN,
            SSET_INTERNAL, SSET_RARE, ALLOW_HACK, ALLOW_HACK,
            N_("Map generation random seed"),
            N_("The same seed will always produce the same map; for zero "
               "(the default) a seed will be chosen based on the time to "
               "give a random map."),
            nullptr, nullptr, nullptr, MAP_MIN_SEED, MAP_MAX_SEED,
            MAP_DEFAULT_SEED),

    /* Map additional stuff: huts and specials.  gameseed also goes here
     * because huts and specials are the first time the gameseed gets used
     * (?) These are done when the game starts, so these are historical and
     * fixed after the game has started.
     */
    GEN_INT("gameseed", game.server.seed_setting, SSET_MAP_ADD,
            SSET_INTERNAL, SSET_RARE, ALLOW_HACK, ALLOW_HACK,
            N_("Game random seed"),
            N_("For zero (the default) a seed will be chosen based on the "
               "current time."),
            nullptr, nullptr, nullptr, GAME_MIN_SEED, GAME_MAX_SEED,
            GAME_DEFAULT_SEED),

    GEN_INT("specials", wld.map.server.riches, SSET_MAP_ADD, SSET_GEOLOGY,
            SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Amount of \"special\" resource tiles"),
            N_("Special resources improve the basic terrain type they are "
               "on. The server variable's scale is parts per thousand."),
            nullptr, nullptr, nullptr, MAP_MIN_RICHES, MAP_MAX_RICHES,
            MAP_DEFAULT_RICHES),

    GEN_INT(
        "huts", wld.map.server.huts, SSET_MAP_ADD, SSET_GEOLOGY, SSET_VITAL,
        ALLOW_NONE, ALLOW_BASIC, N_("Amount of huts (bonus extras)"),
        N_("Huts are tile extras that usually may be investigated by "
           "units.The server variable's scale is huts per thousand tiles."),
        huts_help, nullptr, huts_action, MAP_MIN_HUTS, MAP_MAX_HUTS,
        MAP_DEFAULT_HUTS),

    GEN_INT("animals", wld.map.server.animals, SSET_MAP_ADD, SSET_GEOLOGY,
            SSET_VITAL, ALLOW_NONE, ALLOW_BASIC, N_("Amount of animals"),
            N_("Number of animals initially created on terrains defined for "
               "them in the ruleset (if the ruleset supports it). The "
               "server variable's scale is animals per thousand tiles."),
            nullptr, nullptr, nullptr, MAP_MIN_ANIMALS, MAP_MAX_ANIMALS,
            MAP_DEFAULT_ANIMALS),

    /* Options affecting numbers of players and AI players.  These only
     * affect the start of the game and can not be adjusted after that.
     */
    GEN_INT("minplayers", game.server.min_players, SSET_PLAYERS,
            SSET_INTERNAL, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Minimum number of players"),
            N_("There must be at least this many players (connected human "
               "players) before the game can start."),
            nullptr, nullptr, nullptr, GAME_MIN_MIN_PLAYERS,
            GAME_MAX_MIN_PLAYERS, GAME_DEFAULT_MIN_PLAYERS),

    GEN_INT(
        "maxplayers", game.server.max_players, SSET_PLAYERS, SSET_INTERNAL,
        SSET_VITAL, ALLOW_NONE, ALLOW_BASIC, N_("Maximum number of players"),
        N_("The maximal number of human and AI players who can be in the "
           "game. When this number of players are connected in the pregame "
           "state, any new players who try to connect will be rejected.\n"
           "When playing a scenario which defines player start positions, "
           "this setting cannot be set to greater than the number of "
           "defined start positions."),
        nullptr, maxplayers_callback, nullptr, GAME_MIN_MAX_PLAYERS,
        GAME_MAX_MAX_PLAYERS, GAME_DEFAULT_MAX_PLAYERS),

    GEN_INT("aifill", game.info.aifill, SSET_PLAYERS, SSET_INTERNAL,
            SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Limited number of AI players"),
            N_("If set to a positive value, then AI players will be "
               "automatically created or removed to keep the total number "
               "of players at this amount. As more players join, these AI "
               "players will be replaced. When set to zero, all AI players "
               "will be removed."),
            nullptr, nullptr, aifill_action, GAME_MIN_AIFILL,
            GAME_MAX_AIFILL, GAME_DEFAULT_AIFILL),

    GEN_ENUM(
        "persistentready", game.info.persistent_ready, SSET_META,
        SSET_NETWORK, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("When the Readiness of a player gets autotoggled off"),
        N_("In pre-game, usually when new players join or old ones leave, "
           "those who have already accepted game to start by toggling "
           "\"Ready\" get that autotoggled off in the changed situation. "
           "This setting can be used to make readiness more persistent."),
        nullptr, nullptr, nullptr, persistentready_name,
        GAME_DEFAULT_PERSISTENTREADY),

    GEN_STRING(
        "nationset", game.server.nationset, SSET_PLAYERS, SSET_INTERNAL,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Set of nations to choose from"),
        /* TRANS: do not translate '/list nationsets' */
        N_("Controls the set of nations allowed in the game. The choices "
           "are defined by the ruleset.\n"
           "Only nations in the set selected here will be allowed in any "
           "circumstances, including new players and civil war; small sets "
           "may thus limit the number of players in a game.\n"
           "If this is left blank, the ruleset's default nation set is "
           "used.\n"
           "See '/list nationsets' for possible choices for the currently "
           "loaded ruleset."),
        nationset_callback, nationset_action, GAME_DEFAULT_NATIONSET),

    GEN_INT("ec_turns", game.server.event_cache.turns, SSET_META,
            SSET_INTERNAL, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Event cache for this number of turns"),
            N_("Event messages are saved for this number of turns. A value "
               "of 0 deactivates the event cache."),
            nullptr, nullptr, nullptr, GAME_MIN_EVENT_CACHE_TURNS,
            GAME_MAX_EVENT_CACHE_TURNS, GAME_DEFAULT_EVENT_CACHE_TURNS),

    GEN_INT(
        "ec_max_size", game.server.event_cache.max_size, SSET_META,
        SSET_INTERNAL, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Size of the event cache"),
        N_("This defines the maximal number of events in the event cache."),
        nullptr, nullptr, nullptr, GAME_MIN_EVENT_CACHE_MAX_SIZE,
        GAME_MAX_EVENT_CACHE_MAX_SIZE, GAME_DEFAULT_EVENT_CACHE_MAX_SIZE),

    GEN_BOOL(
        "ec_chat", game.server.event_cache.chat, SSET_META, SSET_INTERNAL,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Save chat messages in the event cache"),
        N_("If turned on, chat messages will be saved in the event cache."),
        nullptr, nullptr, GAME_DEFAULT_EVENT_CACHE_CHAT),

    GEN_BOOL("ec_info", game.server.event_cache.info, SSET_META,
             SSET_INTERNAL, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Print turn and time for each cached event"),
             /* TRANS: Don't translate the text
                between single quotes. */
             N_("If turned on, all cached events will be marked by the turn "
                "and time of the event like '(T2 - 15:29:52)'."),
             nullptr, nullptr, GAME_DEFAULT_EVENT_CACHE_INFO),

    /* Game initialization parameters (only affect the first start of the
     * game, and not reloads).  Can not be changed after first start of game.
     */
    GEN_STRING(
        "startunits", game.server.start_units, SSET_GAME_INIT,
        SSET_SOCIOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
        N_("List of players' initial units"),
        N_("This should be a string of characters, each of which specifies "
           "a unit role. The first character must be native to at least one "
           "\"Starter\" terrain. The characters and their meanings are:\n"
           "    c   = City founder (eg., Settlers)\n"
           "    w   = Terrain worker (eg., Engineers)\n"
           "    x   = Explorer (eg., Explorer)\n"
           "    k   = Gameloss (eg., King)\n"
           "    s   = Diplomat (eg., Diplomat)\n"
           "    f   = Ferryboat (eg., Trireme)\n"
           "    d   = Ok defense unit (eg., Warriors)\n"
           "    D   = Good defense unit (eg., Phalanx)\n"
           "    a   = Fast attack unit (eg., Horsemen)\n"
           "    A   = Strong attack unit (eg., Catapult)\n"),
        startunits_callback, nullptr, GAME_DEFAULT_START_UNITS),

    GEN_BOOL("startcity", game.server.start_city, SSET_GAME_INIT,
             SSET_SOCIOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Whether player starts with a city"),
             N_("If this is set, game will start with player's first city "
                "already founded to starting location."),
             nullptr, nullptr, GAME_DEFAULT_START_CITY),

    GEN_INT("dispersion", game.server.dispersion, SSET_GAME_INIT,
            SSET_SOCIOLOGY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Area where initial units are located"),
            N_("This is the radius within which the initial units are "
               "dispersed."),
            nullptr, nullptr, nullptr, GAME_MIN_DISPERSION,
            GAME_MAX_DISPERSION, GAME_DEFAULT_DISPERSION),

    GEN_INT(
        "gold", game.info.gold, SSET_GAME_INIT, SSET_ECONOMICS, SSET_VITAL,
        ALLOW_NONE, ALLOW_BASIC, N_("Starting gold per player"),
        N_("At the beginning of the game, each player is given this much "
           "gold."),
        nullptr, nullptr, nullptr, GAME_MIN_GOLD, GAME_MAX_GOLD,
        GAME_DEFAULT_GOLD),

    GEN_INT("techlevel", game.info.tech, SSET_GAME_INIT, SSET_SCIENCE,
            SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Number of initial techs per player"),
            /* TRANS: The string between single quotes is a setting name and
               should not be translated. */
            N_("At the beginning of the game, each player is given this "
               "many technologies. The technologies chosen are random for "
               "each player. Depending on the value of tech_cost_style in "
               "the ruleset, a big value for 'techlevel' can make the next "
               "techs really expensive."),
            nullptr, nullptr, nullptr, GAME_MIN_TECHLEVEL,
            GAME_MAX_TECHLEVEL, GAME_DEFAULT_TECHLEVEL),

    GEN_INT("sciencebox", game.info.sciencebox, SSET_RULES_SCENARIO,
            SSET_SCIENCE, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Technology cost multiplier percentage"),
            N_("This affects how quickly players can research new "
               "technology. All tech costs are multiplied by this amount "
               "(as a percentage). The base tech costs are determined by "
               "the ruleset or other game settings."),
            nullptr, nullptr, nullptr, GAME_MIN_SCIENCEBOX,
            GAME_MAX_SCIENCEBOX, GAME_DEFAULT_SCIENCEBOX),

    GEN_BOOL(
        "multiresearch", game.server.multiresearch, SSET_RULES, SSET_SCIENCE,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Allow researching multiple technologies"),
        N_("Allows switching to any technology without wasting old "
           "research. Bulbs are never transfered to new technology. "
           "Techpenalty options are inefective after enabling that option."),
        nullptr, nullptr, GAME_DEFAULT_MULTIRESEARCH),

    GEN_INT("techpenalty", game.server.techpenalty, SSET_RULES, SSET_SCIENCE,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Percentage penalty when changing tech"),
            N_("If you change your current research technology, and you "
               "have positive research points, you lose this percentage of "
               "those research points. This does not apply when you have "
               "just gained a technology this turn."),
            nullptr, nullptr, nullptr, GAME_MIN_TECHPENALTY,
            GAME_MAX_TECHPENALTY, GAME_DEFAULT_TECHPENALTY),

    GEN_INT("techlost_recv", game.server.techlost_recv, SSET_RULES,
            SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Chance to lose a technology while receiving it"),
            N_("The chance that learning a technology by treaty or theft "
               "will fail."),
            nullptr, nullptr, nullptr, GAME_MIN_TECHLOST_RECV,
            GAME_MAX_TECHLOST_RECV, GAME_DEFAULT_TECHLOST_RECV),

    GEN_INT("techlost_donor", game.server.techlost_donor, SSET_RULES,
            SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Chance to lose a technology while giving it"),
            N_("The chance that your civilization will lose a technology if "
               "you teach it to someone else by treaty, or if it is stolen "
               "from you."),
            nullptr, nullptr, nullptr, GAME_MIN_TECHLOST_DONOR,
            GAME_MAX_TECHLOST_DONOR, GAME_DEFAULT_TECHLOST_DONOR),

    GEN_INT("techleak", game.info.tech_leak_pct, SSET_RULES, SSET_SCIENCE,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC, N_("Tech leakage percent"),
            N_("The rate of the tech leakage."), nullptr, nullptr, nullptr,
            GAME_MIN_TECHLEAK, GAME_MAX_TECHLEAK, GAME_DEFAULT_TECHLEAK),

    GEN_BOOL("team_pooled_research", game.info.team_pooled_research,
             SSET_RULES, SSET_SCIENCE, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Team pooled research"),
             N_("If this setting is turned on, then the team mates will "
                "share the science research. Else, every player of the team "
                "will have to make its own."),
             nullptr, nullptr, GAME_DEFAULT_TEAM_POOLED_RESEARCH),

    GEN_INT("diplbulbcost", game.server.diplbulbcost, SSET_RULES,
            SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Penalty when getting tech from treaty"),
            N_("For each technology you gain from a diplomatic treaty, you "
               "lose research points equal to this percentage of the cost "
               "to research a new technology. If this is non-zero, you can "
               "end up with negative research points."),
            nullptr, nullptr, nullptr, GAME_MIN_DIPLBULBCOST,
            GAME_MAX_DIPLBULBCOST, GAME_DEFAULT_DIPLBULBCOST),

    GEN_INT("diplgoldcost", game.server.diplgoldcost, SSET_RULES,
            SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Penalty when getting gold from treaty"),
            N_("When transferring gold in diplomatic treaties, this "
               "percentage of the agreed sum is lost to both parties; it is "
               "deducted from the donor but not received by the recipient."),
            nullptr, nullptr, nullptr, GAME_MIN_DIPLGOLDCOST,
            GAME_MAX_DIPLGOLDCOST, GAME_DEFAULT_DIPLGOLDCOST),

    GEN_INT("incite_gold_loss_chance", game.server.incite_gold_loss_chance,
            SSET_RULES, SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Probability of gold loss during inciting revolt"),
            N_("When unit trying to incite revolt is eliminated, half of "
               "the gold (or quarter, if unit was caught), prepared to "
               "bribe citizens, can be lost or captured by enemy."),
            nullptr, nullptr, nullptr, GAME_MIN_INCITE_GOLD_LOSS_CHANCE,
            GAME_MAX_INCITE_GOLD_LOSS_CHANCE,
            GAME_DEFAULT_INCITE_GOLD_LOSS_CHANCE),

    GEN_INT(
        "incite_gold_capt_chance", game.server.incite_gold_capt_chance,
        SSET_RULES, SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Probability of gold capture during inciting revolt"),
        N_("When unit trying to incite revolt is eliminated and lose its "
           "gold, there is chance that this gold would be captured by city "
           "defender. Transfer tax would be applied, though. This setting "
           "is irrevelant, if incite_gold_loss_chance is zero."),
        nullptr, nullptr, nullptr, GAME_MIN_INCITE_GOLD_CAPT_CHANCE,
        GAME_MAX_INCITE_GOLD_CAPT_CHANCE,
        GAME_DEFAULT_INCITE_GOLD_CAPT_CHANCE),

    GEN_INT("conquercost", game.server.conquercost, SSET_RULES, SSET_SCIENCE,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Penalty when getting tech from conquering"),
            N_("For each technology you gain by conquering an enemy city, "
               "you lose research points equal to this percentage of the "
               "cost to research a new technology. If this is non-zero, you "
               "can end up with negative research points."),
            nullptr, nullptr, nullptr, GAME_MIN_CONQUERCOST,
            GAME_MAX_CONQUERCOST, GAME_DEFAULT_CONQUERCOST),

    GEN_INT(
        "freecost", game.server.freecost, SSET_RULES, SSET_SCIENCE,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Penalty when getting a free tech"),
        /* TRANS: The strings between single quotes are setting names and
           shouldn't be translated. */
        N_("For each technology you gain \"for free\" (other than covered "
           "by 'diplcost' or 'conquercost': for instance, from huts or from "
           "Great Library effects), you lose research points equal to this "
           "percentage of the cost to research a new technology. If this is "
           "non-zero, you can end up with negative research points."),
        nullptr, nullptr, nullptr, GAME_MIN_FREECOST, GAME_MAX_FREECOST,
        GAME_DEFAULT_FREECOST),

    GEN_INT("techlossforgiveness", game.info.techloss_forgiveness,
            SSET_RULES, SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Research point debt threshold for losing tech"),
            N_("When you have negative research points, and your shortfall "
               "is greater than this percentage of the cost of your current "
               "research, you forget a technology you already knew.\n"
               "The special value -1 prevents loss of technology regardless "
               "of research points."),
            nullptr, nullptr, nullptr, GAME_MIN_TECHLOSSFG,
            GAME_MAX_TECHLOSSFG, GAME_DEFAULT_TECHLOSSFG),

    GEN_INT(
        "techlossrestore", game.server.techloss_restore, SSET_RULES,
        SSET_SCIENCE, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Research points restored after losing a tech"),
        N_("When you lose a technology due to a negative research balance "
           "(see 'techlossforgiveness'), this percentage of its research "
           "cost is credited to your research balance (this may not be "
           "sufficient to make it positive).\n"
           "The special value -1 means that your research balance is always "
           "restored to zero, regardless of your previous shortfall."),
        nullptr, nullptr, nullptr, GAME_MIN_TECHLOSSREST,
        GAME_MAX_TECHLOSSREST, GAME_DEFAULT_TECHLOSSREST),

    GEN_INT("foodbox", game.info.foodbox, SSET_RULES, SSET_ECONOMICS,
            SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Food required for a city to grow"),
            N_("This is the base amount of food required to grow a city. "
               "This value is multiplied by another factor that comes from "
               "the ruleset and is dependent on the size of the city."),
            nullptr, nullptr, nullptr, GAME_MIN_FOODBOX, GAME_MAX_FOODBOX,
            GAME_DEFAULT_FOODBOX),

    GEN_INT("aqueductloss", game.server.aqueductloss, SSET_RULES,
            SSET_ECONOMICS, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Percentage food lost when city can't grow"),
            N_("If a city would expand, but it can't because it lacks some "
               "prerequisite (traditionally an Aqueduct or Sewer System), "
               "this is the base percentage of its foodbox that is lost "
               "each turn; the penalty may be reduced by buildings or other "
               "circumstances, depending on the ruleset."),
            nullptr, nullptr, nullptr, GAME_MIN_AQUEDUCTLOSS,
            GAME_MAX_AQUEDUCTLOSS, GAME_DEFAULT_AQUEDUCTLOSS),

    GEN_INT(
        "shieldbox", game.info.shieldbox, SSET_RULES, SSET_ECONOMICS,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Multiplier percentage for production costs"),
        N_("This affects how quickly units and buildings can be produced.  "
           "The base costs are multiplied by this value (as a percentage)."),
        nullptr, nullptr, nullptr, GAME_MIN_SHIELDBOX, GAME_MAX_SHIELDBOX,
        GAME_DEFAULT_SHIELDBOX),

    /* Notradesize and fulltradesize used to have callbacks to prevent them
       from being set illegally (notradesize > fulltradesize). However this
       provided a problem when setting them both through the client's
       settings dialog, since they cannot both be set atomically.  So the
       callbacks were removed and instead the game now knows how to deal with
       invalid settings. */
    GEN_INT("fulltradesize", game.info.fulltradesize, SSET_RULES,
            SSET_ECONOMICS, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Minimum city size to get full trade"),
            /* TRANS: The strings between single quotes are setting names and
               shouldn't be translated. */
            N_("There is a trade penalty in all cities smaller than this. "
               "The penalty is 100% (no trade at all) for sizes up to "
               "'notradesize', and decreases gradually to 0% (no penalty "
               "except the normal corruption) for size='fulltradesize'. See "
               "also 'notradesize'."),
            nullptr, nullptr, nullptr, GAME_MIN_FULLTRADESIZE,
            GAME_MAX_FULLTRADESIZE, GAME_DEFAULT_FULLTRADESIZE),

    GEN_INT("notradesize", game.info.notradesize, SSET_RULES, SSET_ECONOMICS,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Maximum size of a city without trade"),
            /* TRANS: The strings between single quotes are setting names and
               shouldn't be translated. */
            N_("Cities do not produce any trade at all unless their size is "
               "larger than this amount. The produced trade increases "
               "gradually for cities larger than 'notradesize' and smaller "
               "than 'fulltradesize'. See also 'fulltradesize'."),
            nullptr, nullptr, nullptr, GAME_MIN_NOTRADESIZE,
            GAME_MAX_NOTRADESIZE, GAME_DEFAULT_NOTRADESIZE),

    GEN_INT("tradeworldrelpct", game.info.trade_world_rel_pct, SSET_RULES,
            SSET_ECONOMICS, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("How largely trade distance is relative to world size"),
            /* TRANS: The strings between single quotes are setting names and
               shouldn't be translated. */
            N_("When determining trade between cities, the distance factor "
               "can be partly or fully relative to world size. This setting "
               "determines how big percentage of the bonus calculation is "
               "relative to world size, and how much only absolute distance "
               "matters."),
            nullptr, nullptr, nullptr, GAME_MIN_TRADEWORLDRELPCT,
            GAME_MAX_TRADEWORLDRELPCT, GAME_DEFAULT_TRADEWORLDRELPCT),

    GEN_INT("citymindist", game.info.citymindist, SSET_RULES, SSET_SOCIOLOGY,
            SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Minimum distance between cities"),
            N_("When a player attempts to found a new city, it is prevented "
               "if the distance from any existing city is less than this "
               "setting. For example, when this setting is 3, there must be "
               "at least two clear tiles in any direction between all "
               "existing cities and the new city site. A value of 1 removes "
               "any such restriction on city placement."),
            nullptr, nullptr, nullptr, GAME_MIN_CITYMINDIST,
            GAME_MAX_CITYMINDIST, GAME_DEFAULT_CITYMINDIST),

    GEN_BOOL("trading_tech", game.info.trading_tech, SSET_RULES,
             SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Technology trading"),
             N_("If turned off, trading technologies in the diplomacy "
                "dialog is not allowed."),
             nullptr, nullptr, GAME_DEFAULT_TRADING_TECH),

    GEN_BOOL("trading_gold", game.info.trading_gold, SSET_RULES,
             SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Gold trading"),
             N_("If turned off, trading gold in the diplomacy dialog is not "
                "allowed."),
             nullptr, nullptr, GAME_DEFAULT_TRADING_GOLD),

    GEN_BOOL("trading_city", game.info.trading_city, SSET_RULES,
             SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("City trading"),
             N_("If turned off, trading cities in the diplomacy dialog is "
                "not allowed."),
             nullptr, nullptr, GAME_DEFAULT_TRADING_CITY),

    GEN_ENUM("caravan_bonus_style", game.info.caravan_bonus_style,
             SSET_RULES, SSET_ECONOMICS, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Caravan bonus style"),
             N_("The formula for the bonus when a caravan enters a city. "
                "CLASSIC bonuses are proportional to distance and trade of "
                "source and destination with multipliers for overseas and "
                "international destinations. LOGARITHMIC bonuses are "
                "proportional to log^2(distance + trade). LINEAR bonuses "
                "are similar to CLASSIC, but (like LOGARITHMIC) use the "
                "max trade of the city rather than current. DISTANCE "
                "bonuses are proportional only to distance."),
             nullptr, nullptr, nullptr, caravanbonusstyle_name,
             GAME_DEFAULT_CARAVAN_BONUS_STYLE),

    GEN_ENUM(
        "trade_revenue_style", game.info.trade_revenue_style, SSET_RULES,
        SSET_ECONOMICS, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Trade revenue style"),
        N_("The formula for the trade a city receives from a traderoute. "
           "CLASSIC revenues depend on distance and trade with multipliers "
           "for overseas and international routes. SIMPLE revenues are "
           "proportional to the average trade of the two cities."),
        nullptr, nullptr, nullptr, traderevenuestyle_name,
        GAME_DEFAULT_TRADE_REVENUE_STYLE),

    GEN_INT("trademindist", game.info.trademindist, SSET_RULES,
            SSET_ECONOMICS, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Minimum distance for trade routes"),
            N_("In order for two cities in the same civilization to "
               "establish a trade route, they must be at least this far "
               "apart on the map. For square grids, the distance is "
               "calculated as \"Manhattan distance\", that is, the sum of "
               "the displacements along the x and y directions."),
            nullptr, nullptr, nullptr, GAME_MIN_TRADEMINDIST,
            GAME_MAX_TRADEMINDIST, GAME_DEFAULT_TRADEMINDIST),

    GEN_INT(
        "rapturedelay", game.info.rapturedelay, SSET_RULES, SSET_SOCIOLOGY,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Number of turns between rapture effect"),
        N_("Sets the number of turns between rapture growth of a city. If "
           "set to n a city will grow after celebrating for n+1 turns."),
        nullptr, nullptr, nullptr, GAME_MIN_RAPTUREDELAY,
        GAME_MAX_RAPTUREDELAY, GAME_DEFAULT_RAPTUREDELAY),

    GEN_INT("disasters", game.info.disasters, SSET_RULES_FLEXIBLE,
            SSET_SOCIOLOGY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Frequency of disasters"),
            N_("Affects how often random disasters happen to cities, if any "
               "are defined by the ruleset. The relative frequency of "
               "disaster types is set by the ruleset. Zero prevents any "
               "random disasters from occurring."),
            nullptr, nullptr, nullptr, GAME_MIN_DISASTERS,
            GAME_MAX_DISASTERS, GAME_DEFAULT_DISASTERS),

    GEN_ENUM("traitdistribution", game.server.trait_dist, SSET_RULES,
             SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("AI trait distribution method"),
             N_("How trait values are given to AI players."), nullptr,
             nullptr, nullptr, trait_dist_name,
             GAME_DEFAULT_TRAIT_DIST_MODE),

    GEN_INT("razechance", game.server.razechance, SSET_RULES, SSET_MILITARY,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Chance for conquered building destruction"),
            N_("When a player conquers a city, each city improvement has "
               "this percentage chance to be destroyed."),
            nullptr, nullptr, nullptr, GAME_MIN_RAZECHANCE,
            GAME_MAX_RAZECHANCE, GAME_DEFAULT_RAZECHANCE),

    GEN_INT("occupychance", game.server.occupychance, SSET_RULES,
            SSET_MILITARY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Chance of moving into tile after attack"),
            N_("If set to 0, combat is Civ1/2-style (when you attack, you "
               "remain in place). If set to 100, attacking units will "
               "always move into the tile they attacked when they win the "
               "combat (and no enemy units remain in the tile). If set to a "
               "value between 0 and 100, this will be used as the percent "
               "chance of \"occupying\" territory."),
            nullptr, nullptr, nullptr, GAME_MIN_OCCUPYCHANCE,
            GAME_MAX_OCCUPYCHANCE, GAME_DEFAULT_OCCUPYCHANCE),

    GEN_BOOL(
        "autoattack", game.server.autoattack, SSET_RULES_FLEXIBLE,
        SSET_MILITARY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Turn on/off server-side autoattack"),
        N_("If set to on, units with moves left will automatically consider "
           "attacking enemy units that move adjacent to them."),
        nullptr, nullptr, GAME_DEFAULT_AUTOATTACK),

    GEN_BOOL(
        "killstack", game.info.killstack, SSET_RULES_SCENARIO, SSET_MILITARY,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Do all units in tile die with defender"),
        N_("If this is enabled, each time a defender unit loses in combat, "
           "and is not inside a city or suitable base, all units in the "
           "same tile are destroyed along with the defender. If this is "
           "disabled, only the defender unit is destroyed."),
        nullptr, nullptr, GAME_DEFAULT_KILLSTACK),

    GEN_BOOL("killcitizen", game.info.killcitizen, SSET_RULES, SSET_MILITARY,
             SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Reduce city population after attack"),
             N_("This flag indicates whether a city's population is reduced "
                "after a successful attack by an enemy unit. If this is "
                "disabled, population is never reduced. Even when this is "
                "enabled, only some units may kill citizens."),
             nullptr, nullptr, GAME_DEFAULT_KILLCITIZEN),

    GEN_INT(
        "killunhomed", game.info.killunhomed, SSET_RULES, SSET_MILITARY,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Slowly kill units without home cities (e.g., starting units)"),
        N_("If greater than 0, then every unit without a homecity will lose "
           "hitpoints each turn. The number of hitpoints lost is given by "
           "'killunhomed' percent of the hitpoints of the unit type. At "
           "least one hitpoint is lost every turn until the death of the "
           "unit."),
        nullptr, nullptr, nullptr, GAME_MIN_KILLUNHOMED,
        GAME_MAX_KILLUNHOMED, GAME_DEFAULT_KILLUNHOMED),

    GEN_ENUM("borders", game.info.borders, SSET_RULES, SSET_MILITARY,
             SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("National borders"),
             N_("If this is not disabled, then any land tiles around a city "
                "or border-claiming extra (like the classic ruleset's "
                "Fortress base) will be owned by that nation. SEE_INSIDE "
                "and EXPAND makes everything inside a player's borders "
                "visible at once. ENABLED will, in some rulesets, grant the "
                "same visibility if certain conditions are met."),
             nullptr, nullptr, nullptr, borders_name, GAME_DEFAULT_BORDERS),

    GEN_ENUM(
        "happyborders", game.info.happyborders, SSET_RULES, SSET_MILITARY,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Units inside borders cause no unhappiness"),
        N_("If this is set, units will not cause unhappiness when inside "
           "your borders, or even allies borders, depending on value."),
        nullptr, nullptr, nullptr, happyborders_name,
        GAME_DEFAULT_HAPPYBORDERS),

    GEN_ENUM("diplomacy", game.info.diplomacy, SSET_RULES, SSET_MILITARY,
             SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Ability to do diplomacy with other players"),
             N_("This setting controls the ability to do diplomacy with "
                "other players."),
             nullptr, nullptr, nullptr, diplomacy_name,
             GAME_DEFAULT_DIPLOMACY),

    GEN_ENUM("citynames", game.server.allowed_city_names, SSET_RULES,
             SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Allowed city names"),
             /* TRANS: The strings between double quotes are also translated
              * separately (they must match!). The strings between
              * parentheses and in uppercase must not be translated.
              */
             N_("- \"No restrictions\" (NO_RESTRICTIONS): players can have "
                "multiple cities with the same names.\n"
                "- \"Unique to a player\" (PLAYER_UNIQUE): one player can't "
                "have multiple cities with the same name.\n"
                "- \"Globally unique\" (GLOBAL_UNIQUE): all cities in a "
                "game have to have different names.\n"
                "- \"No city name stealing\" (NO_STEALING): like \"Globally "
                "unique\", but a player isn't allowed to use a default city "
                "name of another nation unless it is a default for their "
                "nation also."),
             nullptr, nullptr, nullptr, citynames_name,
             GAME_DEFAULT_ALLOWED_CITY_NAMES),

    GEN_ENUM(
        "plrcolormode", game.server.plrcolormode, SSET_RULES, SSET_INTERNAL,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC, N_("How to pick player colors"),
        /* TRANS: The strings between double quotes are also translated
         * separately (they must match!). The strings between single quotes
         * are setting names and shouldn't be translated. The strings between
         * parentheses and in uppercase must not be translated.
         */
        N_("This setting determines how player colors are chosen. Player "
           "colors are used in the Nations report, for national borders on "
           "the map, and so on.\n"
           "- \"Per-player, in order\" (PLR_ORDER): colors are assigned to "
           "individual players in order from a list defined by the "
           "ruleset.\n"
           "- \"Per-player, random\" (PLR_RANDOM): colors are assigned to "
           "individual players randomly from the set defined by the "
           "ruleset.\n"
           "- \"Set manually\" (PLR_SET): colors can be set with the "
           "'playercolor' command before the game starts; these are not "
           "restricted to the ruleset colors. Any players for which no "
           "color is set when the game starts get a random color from the "
           "ruleset.\n"
           "- \"Per-team, in order\" (TEAM_ORDER): colors are assigned to "
           "teams from the list in the ruleset. Every player on the same "
           "team gets the same color.\n"
           "- \"Per-nation, in order\" (NATION_ORDER): if the ruleset "
           "defines a color for a player's nation, the player takes that "
           "color. Any players whose nations don't have associated colors "
           "get a random color from the list in the ruleset.\n"
           "Regardless of this setting, individual player colors can be "
           "changed after the game starts with the 'playercolor' command."),
        nullptr, plrcol_validate, plrcol_action, plrcol_name,
        GAME_DEFAULT_PLRCOLORMODE),

    /* Flexible rules: these can be changed after the game has started.
     * The distinction between "rules" and "flexible rules" is not always
     * clearcut, and some existing cases may be largely historical or
     * accidental.  However some generalizations can be made:
     *   -- Low-level game mechanics should not be flexible (eg, rulesets).
     * -- Options which would affect the game "state" (city production etc)
     * should not be flexible (eg, foodbox).   -- Options which are
     * explicitly sent to the client (eg, in      packet_game_info) should
     * probably not be flexible, or at      least need extra care to be
     * flexible.
     */
    GEN_ENUM("barbarians", game.server.barbarianrate, SSET_RULES_FLEXIBLE,
             SSET_MILITARY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Barbarian appearance frequency"),
             /* TRANS: The string between single quotes is a setting name and
                should not be translated. */
             N_("This setting controls how frequently the barbarians appear "
                "in the game. See also the 'onsetbarbs' setting."),
             nullptr, nullptr, nullptr, barbarians_name,
             GAME_DEFAULT_BARBARIANRATE),

    GEN_INT("onsetbarbs", game.server.onsetbarbarian, SSET_RULES_FLEXIBLE,
            SSET_MILITARY, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Barbarian onset turn"),
            N_("Barbarians will not appear before this turn."), nullptr,
            nullptr, nullptr, GAME_MIN_ONSETBARBARIAN,
            GAME_MAX_ONSETBARBARIAN, GAME_DEFAULT_ONSETBARBARIAN),

    GEN_ENUM(
        "revolentype", game.info.revolentype, SSET_RULES, SSET_SOCIOLOGY,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Way to determine revolution length"),
        N_("Which method is used in determining how long period of anarchy "
           "lasts when changing government. The actual value is set with "
           "'revolen' setting. The 'quickening' methods depend on how many "
           "times any player has changed to this type of government before, "
           "so it becomes easier to establish a new system of government if "
           "it has been done before."),
        nullptr, nullptr, nullptr, revolentype_name,
        GAME_DEFAULT_REVOLENTYPE),

    GEN_INT("revolen", game.server.revolution_length, SSET_RULES_FLEXIBLE,
            SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Length of revolution"),
            N_("When changing governments, a period of anarchy will occur. "
               "Value of this setting, used the way 'revolentype' setting "
               "dictates, defines the length of the anarchy."),
            nullptr, nullptr, nullptr, GAME_MIN_REVOLUTION_LENGTH,
            GAME_MAX_REVOLUTION_LENGTH, GAME_DEFAULT_REVOLUTION_LENGTH),

    GEN_BOOL("fogofwar", game.info.fogofwar, SSET_RULES, SSET_MILITARY,
             SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Whether to enable fog of war"),
             N_("If this is enabled, only those units and cities within the "
                "vision range of your own units and cities will be revealed "
                "to you. You will not see new cities or terrain changes in "
                "tiles not observed."),
             nullptr, nullptr, GAME_DEFAULT_FOGOFWAR),

    GEN_BOOL("foggedborders", game.server.foggedborders, SSET_RULES,
             SSET_MILITARY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Whether fog of war applies to border changes"),
             N_("If this setting is enabled, players will not be able to "
                "see changes in tile ownership if they do not have direct "
                "sight of the affected tiles. Otherwise, players can see "
                "any or all changes to borders as long as they have "
                "previously seen the tiles."),
             nullptr, nullptr, GAME_DEFAULT_FOGGEDBORDERS),

    GEN_BITWISE(
        "airliftingstyle", game.info.airlifting_style, SSET_RULES_FLEXIBLE,
        SSET_MILITARY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Airlifting style"),
        /* TRANS: The strings between double quotes are also translated
         * separately (they must match!). The strings between parenthesis and
         * in uppercase must not be translated.
         */
        N_("This setting affects airlifting units between cities. It can be "
           "a set of the following values:\n"
           "- \"Allows units to be airlifted from allied cities\" "
           "(FROM_ALLIES).\n"
           "- \"Allows units to be airlifted to allied cities\" "
           "(TO_ALLIES).\n"
           "- \"Unlimited units from source city\" (SRC_UNLIMITED): note "
           "that airlifting from a city doesn't reduce the airlifted "
           "counter, but still needs airlift capacity of at least 1.\n"
           "- \"Unlimited units to destination city\" (DEST_UNLIMITED): "
           "note that airlifting to a city doesn't reduce the airlifted "
           "counter, and doesn't need any airlift capacity."),
        nullptr, nullptr, airliftingstyle_name,
        GAME_DEFAULT_AIRLIFTINGSTYLE),

    GEN_INT(
        "diplchance", game.server.diplchance, SSET_RULES_FLEXIBLE,
        SSET_MILITARY, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("Base chance for diplomats and spies to succeed"),
        N_("The base chance of a spy returning from a successful mission "
           "and the base chance of success for diplomats and spies."),
        nullptr, nullptr, nullptr, GAME_MIN_DIPLCHANCE, GAME_MAX_DIPLCHANCE,
        GAME_DEFAULT_DIPLCHANCE),

    GEN_BITWISE(
        "victories", game.info.victory_conditions, SSET_RULES_FLEXIBLE,
        SSET_INTERNAL, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
        N_("What kinds of victories are possible"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("This setting controls how game can be won. One can always win "
           "by conquering entire planet, but other victory conditions can "
           "be enabled or disabled:\n"
           "- \"Spacerace\" (SPACERACE): Spaceship is built and travels to "
           "Alpha Centauri.\n"
           "- \"Allied\" (ALLIED): After defeating enemies, all remaining "
           "players are allied.\n"
           "- \"Culture\" (CULTURE): Player meets ruleset defined cultural "
           "domination criteria.\n"),
        nullptr, nullptr, victory_conditions_name,
        GAME_DEFAULT_VICTORY_CONDITIONS),

    GEN_BOOL("endspaceship", game.server.endspaceship, SSET_RULES_FLEXIBLE,
             SSET_SCIENCE, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Should the game end if the spaceship arrives?"),
             N_("If this option is turned on, the game will end with the "
                "arrival of a spaceship at Alpha Centauri."),
             nullptr, nullptr, GAME_DEFAULT_END_SPACESHIP),

    GEN_INT(
        "spaceship_travel_time", game.server.spaceship_travel_time,
        SSET_RULES_FLEXIBLE, SSET_SCIENCE, SSET_VITAL, ALLOW_NONE,
        ALLOW_BASIC, N_("Percentage to multiply spaceship travel time by"),
        N_("This percentage is multiplied onto the time it will take for a "
           "spaceship to arrive at Alpha Centauri."),
        nullptr, nullptr, nullptr, GAME_MIN_SPACESHIP_TRAVEL_TIME,
        GAME_MAX_SPACESHIP_TRAVEL_TIME, GAME_DEFAULT_SPACESHIP_TRAVEL_TIME),

    GEN_INT("civilwarsize", game.server.civilwarsize, SSET_RULES_FLEXIBLE,
            SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Minimum number of cities for civil war"),
            N_("A civil war is triggered when a player has at least this "
               "many cities and the player's capital is captured. If this "
               "option is set to the maximum value, civil wars are turned "
               "off altogether."),
            nullptr, nullptr, nullptr, GAME_MIN_CIVILWARSIZE,
            GAME_MAX_CIVILWARSIZE, GAME_DEFAULT_CIVILWARSIZE),

    GEN_BOOL("restrictinfra", game.info.restrictinfra, SSET_RULES_FLEXIBLE,
             SSET_MILITARY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
             N_("Restrict the use of the infrastructure for enemy units"),
             N_("If this option is enabled, the use of roads and rails will "
                "be restricted for enemy units."),
             nullptr, nullptr, GAME_DEFAULT_RESTRICTINFRA),

    GEN_BOOL("unreachableprotects", game.info.unreachable_protects,
             SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, ALLOW_NONE,
             ALLOW_BASIC, N_("Does unreachable unit protect reachable ones"),
             N_("This option controls whether tiles with both unreachable "
                "and reachable units can be attacked. If disabled, any tile "
                "with reachable units can be attacked. If enabled, tiles "
                "with an unreachable unit in them cannot be attacked. Some "
                "units in some rulesets may override this, never protecting "
                "reachable units on their tile."),
             nullptr, nullptr, GAME_DEFAULT_UNRPROTECTS),

    GEN_INT("contactturns", game.server.contactturns, SSET_RULES_FLEXIBLE,
            SSET_MILITARY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Turns until player contact is lost"),
            N_("Players may meet for diplomacy this number of turns after "
               "their units have last met, even when they do not have an "
               "embassy. If set to zero, then players cannot meet unless "
               "they have an embassy."),
            nullptr, nullptr, nullptr, GAME_MIN_CONTACTTURNS,
            GAME_MAX_CONTACTTURNS, GAME_DEFAULT_CONTACTTURNS),

    GEN_BOOL(
        "savepalace", game.server.savepalace, SSET_RULES_FLEXIBLE,
        SSET_MILITARY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Rebuild palace whenever capital is conquered"),
        N_("If this is turned on, when the capital is conquered the palace "
           "is automatically rebuilt for free in another randomly chosen "
           "city. This is significant because the technology requirement "
           "for building a palace will be ignored. (In some rulesets, "
           "buildings other than the palace are affected by this setting.)"),
        nullptr, nullptr, GAME_DEFAULT_SAVEPALACE),

    GEN_BOOL("homecaughtunits", game.server.homecaughtunits,
             SSET_RULES_FLEXIBLE, SSET_MILITARY, SSET_RARE, ALLOW_NONE,
             ALLOW_BASIC, N_("Give caught units a homecity"),
             /* TRANS: The string between single quotes is a setting name and
              * should not be translated.
              */
             N_("If unset, caught units will have no homecity and will be "
                "subject to the 'killunhomed' option."),
             nullptr, nullptr, GAME_DEFAULT_HOMECAUGHTUNITS),

    GEN_BOOL("naturalcitynames", game.server.natural_city_names,
             SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE,
             ALLOW_BASIC, N_("Whether to use natural city names"),
             N_("If enabled, the default city names will be determined "
                "based on the surrounding terrain."),
             nullptr, nullptr, GAME_DEFAULT_NATURALCITYNAMES),

    GEN_BOOL(
        "migration", game.server.migration, SSET_RULES_FLEXIBLE,
        SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Whether to enable citizen migration"),
        /* TRANS: The strings between single quotes are setting names
           and should not be translated. */
        N_("This is the master setting that controls whether citizen "
           "migration is active in the game. If enabled, citizens may "
           "automatically move from less desirable cities to more desirable "
           "ones. The \"desirability\" of a given city is calculated from a "
           "number of factors. In general larger cities with more income "
           "and improvements will be preferred. Citizens will never migrate "
           "out of the capital, or cause a wonder to be lost by disbanding "
           "a city. A number of other settings control how migration "
           "behaves:\n"
           "  'mgr_turninterval' - How often citizens try to migrate.\n"
           "  'mgr_foodneeded'   - Whether destination food is checked.\n"
           "  'mgr_distance'     - How far citizens will migrate.\n"
           "  'mgr_worldchance'  - Chance for inter-nation migration.\n"
           "  'mgr_nationchance' - Chance for intra-nation migration."),
        nullptr, nullptr, GAME_DEFAULT_MIGRATION),

    GEN_INT("mgr_turninterval", game.server.mgr_turninterval,
            SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE,
            ALLOW_BASIC,
            N_("Number of turns between migrations from a city"),
            /* TRANS: Do not translate 'migration'
               setting name. */
            N_("This setting controls the number of turns between migration "
               "checks for a given city. The interval is calculated from "
               "the founding turn of the city. So for example if this "
               "setting is 5, citizens will look for a suitable migration "
               "destination every five turns from the founding of their "
               "current city. Migration will never occur the same turn that "
               "a city is built. This setting has no effect unless "
               "migration is enabled by the 'migration' setting."),
            nullptr, nullptr, nullptr, GAME_MIN_MGR_TURNINTERVAL,
            GAME_MAX_MGR_TURNINTERVAL, GAME_DEFAULT_MGR_TURNINTERVAL),

    GEN_BOOL("mgr_foodneeded", game.server.mgr_foodneeded,
             SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE,
             ALLOW_BASIC, N_("Whether migration is limited by food"),
             /* TRANS: Do not translate 'migration'
                setting name. */
             N_("If this setting is enabled, citizens will not migrate to "
                "cities which would not have enough food to support them. "
                "This setting has no effect unless migration is enabled by "
                "the 'migration' setting."),
             nullptr, nullptr, GAME_DEFAULT_MGR_FOODNEEDED),

    GEN_INT(
        "mgr_distance", game.server.mgr_distance, SSET_RULES_FLEXIBLE,
        SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Maximum distance citizens may migrate"),
        /* TRANS: Do not translate 'migration'
           setting name. */
        N_("This setting controls how far citizens may look for a suitable "
           "migration destination when deciding which city to migrate to. "
           "The value is added to the candidate target city's radius and "
           "compared to the distance between the two cities. If the "
           "distance is lower or equal, migration is possible. (So with a "
           "setting of 0, citizens will only consider migrating if their "
           "city's center is within the destination city's working radius.) "
           "This setting has no effect unless migration is enabled by the "
           "'migration' setting."),
        nullptr, nullptr, nullptr, GAME_MIN_MGR_DISTANCE,
        GAME_MAX_MGR_DISTANCE, GAME_DEFAULT_MGR_DISTANCE),

    GEN_INT("mgr_nationchance", game.server.mgr_nationchance,
            SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE,
            ALLOW_BASIC,
            N_("Percent probability for migration within the same nation"),
            /* TRANS: Do not translate
               'migration' setting name. */
            N_("This setting controls how likely it is for citizens to "
               "migrate between cities owned by the same player. Zero "
               "indicates migration will never occur, 100 means that "
               "migration will always occur if the citizens find a suitable "
               "destination. This setting has no effect unless migration is "
               "activated by the 'migration' setting."),
            nullptr, nullptr, nullptr, GAME_MIN_MGR_NATIONCHANCE,
            GAME_MAX_MGR_NATIONCHANCE, GAME_DEFAULT_MGR_NATIONCHANCE),

    GEN_INT("mgr_worldchance", game.server.mgr_worldchance,
            SSET_RULES_FLEXIBLE, SSET_SOCIOLOGY, SSET_RARE, ALLOW_NONE,
            ALLOW_BASIC,
            N_("Percent probability for migration between foreign cities"),
            /* TRANS: Do not translate
               'migration' setting name. */
            N_("This setting controls how likely it is for migration to "
               "occur between cities owned by different players. Zero "
               "indicates migration will never occur, 100 means that "
               "citizens will always migrate if they find a suitable "
               "destination. This setting has no effect if migration is not "
               "enabled by the 'migration' setting."),
            nullptr, nullptr, nullptr, GAME_MIN_MGR_WORLDCHANCE,
            GAME_MAX_MGR_WORLDCHANCE, GAME_DEFAULT_MGR_WORLDCHANCE),

    /* Meta options: these don't affect the internal rules of the game, but
     * do affect players.  Also options which only produce extra server
     * "output" and don't affect the actual game. ("endturn" is here, and not
     * RULES_FLEXIBLE, because it doesn't affect what happens in the game, it
     * just determines when the players stop playing and look at the score.)
     */
    GEN_STRING(
        "allowtake", game.server.allow_take, SSET_META, SSET_NETWORK,
        SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
        N_("Players that users are allowed to take"),
        /* TRANS: the strings in double quotes are server command names and
           should not be translated. */
        N_("This should be a string of characters, each of which specifies "
           "a type or status of a civilization (player).\n"
           "Clients will only be permitted to take or observe those players "
           "which match one of the specified letters. This only affects "
           "future uses of the \"take\" or \"observe\" commands; it is not "
           "retroactive. The characters and their meanings are:\n"
           "    o,O = Global observer\n"
           "    b   = Barbarian players\n"
           "    d   = Dead players\n"
           "    a,A = AI players\n"
           "    h,H = Human players\n"
           "The first description on this list which matches a player is "
           "the one which applies. Thus 'd' does not include dead "
           "barbarians, 'a' does not include dead AI players, and so on. "
           "Upper case letters apply before the game has started, lower "
           "case letters afterwards.\n"
           "Each character above may be followed by one of the following "
           "numbers to allow or restrict the manner of connection:\n"
           "(none) = Controller allowed, observers allowed, can displace "
           "connections. (Displacing a connection means that you may take "
           "over a player, even when another user already controls that "
           "player.)\n"
           "     1 = Controller allowed, observers allowed, can't displace "
           "connections;\n"
           "     2 = Controller allowed, no observers allowed, can displace "
           "connections;\n"
           "     3 = Controller allowed, no observers allowed, can't "
           "displace connections;\n"
           "     4 = No controller allowed, observers allowed"),
        allowtake_callback, nullptr, GAME_DEFAULT_ALLOW_TAKE),

    GEN_BOOL("autotoggle", game.server.auto_ai_toggle, SSET_META,
             SSET_NETWORK, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Whether AI-status toggles with connection"),
             N_("If enabled, AI status is turned off when a player "
                "connects, and on when a player disconnects."),
             nullptr, autotoggle_action, GAME_DEFAULT_AUTO_AI_TOGGLE),

    GEN_INT("endturn", game.server.end_turn, SSET_META, SSET_SOCIOLOGY,
            SSET_VITAL, ALLOW_NONE, ALLOW_BASIC, N_("Turn the game ends"),
            N_("The game will end at the end of the given turn."), nullptr,
            endturn_callback, nullptr, GAME_MIN_END_TURN, GAME_MAX_END_TURN,
            GAME_DEFAULT_END_TURN),

    GEN_BITWISE(
        "revealmap", game.server.revealmap, SSET_GAME_INIT, SSET_MILITARY,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC, N_("Reveal the map"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must not be translated. */
        N_("If \"Reveal map at game start\" (START) is set, the initial "
           "state of the entire map will be known to all players from the "
           "start of the game, although it may still be fogged (depending "
           "on the 'fogofwar' setting). If \"Unfog map for dead players\" "
           "(DEAD) is set, dead players can see the entire map, if they are "
           "alone in their team."),
        nullptr, nullptr, revealmap_name, GAME_DEFAULT_REVEALMAP),

    GEN_INT(
        "timeout", game.info.timeout, SSET_META, SSET_INTERNAL, SSET_VITAL,
        ALLOW_NONE, ALLOW_BASIC, N_("Maximum seconds per turn"),
        /* TRANS: \"Turn Done\" refers to the client button; it is also
           translated separately, the translation should be the same.
           \"timeoutincrease\" is a command name and must not to be
           translated. */
        N_("If all players have not hit \"Turn Done\" before this time is "
           "up, then the turn ends automatically. Zero means there is no "
           "timeout. In servers compiled with debugging, a timeout of -1 "
           "sets the autogame test mode. Only connections with hack level "
           "access may set the timeout to fewer than 30 seconds. Use this "
           "with the command \"timeoutincrease\" to have a dynamic timer. "
           "The first turn is treated as a special case and is controlled "
           "by the 'first_timeout' setting."),
        nullptr, timeout_callback, timeout_action, GAME_MIN_TIMEOUT,
        GAME_MAX_TIMEOUT, GAME_DEFAULT_TIMEOUT),

    GEN_INT(
        "first_timeout", game.info.first_timeout, SSET_META, SSET_INTERNAL,
        SSET_VITAL, ALLOW_NONE, ALLOW_BASIC, N_("First turn timeout"),
        /* TRANS: The strings between single quotes are setting names and
           should not be translated. */
        N_("If greater than 0, T1 will last for 'first_timeout' seconds.\n"
           "If set to 0, T1 will not have a timeout.\n"
           "If set to -1, the special treatment of T1 will be disabled.\n"
           "See also 'timeout'."),
        nullptr, first_timeout_callback, first_timeout_action,
        GAME_MIN_FIRST_TIMEOUT, GAME_MAX_FIRST_TIMEOUT,
        GAME_DEFAULT_FIRST_TIMEOUT),

    GEN_INT("timeaddenemymove", game.server.timeoutaddenemymove, SSET_META,
            SSET_INTERNAL, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Timeout at least n seconds when enemy moved"),
            N_("Any time a unit moves while in sight of an enemy player, "
               "the remaining timeout is increased to this value."),
            nullptr, nullptr, nullptr, 0, GAME_MAX_TIMEOUT,
            GAME_DEFAULT_TIMEOUTADDEMOVE),

    GEN_INT("unitwaittime", game.server.unitwaittime, SSET_RULES_FLEXIBLE,
            SSET_INTERNAL, SSET_VITAL, ALLOW_NONE, ALLOW_BASIC,
            N_("Minimum time between unit actions over turn change"),
            /* TRANS: The string between single quotes is a setting name and
               should not be translated. */
            N_("This setting gives the minimum amount of time in seconds "
               "between unit moves and other significant actions (such as "
               "building cities) after a turn change occurs. For example, "
               "if this setting is set to 20 and a unit moves 5 seconds "
               "before the turn change, it will not be able to move or act "
               "in the next turn for at least 15 seconds. This value is "
               "limited to a maximum value of 2/3 'timeout'."),
            nullptr, unitwaittime_callback, nullptr, GAME_MIN_UNITWAITTIME,
            GAME_MAX_UNITWAITTIME, GAME_DEFAULT_UNITWAITTIME),

    GEN_BITWISE(
        "unitwaittime_style", game.server.unitwaittime_style,
        SSET_RULES_FLEXIBLE, SSET_INTERNAL, SSET_VITAL, ALLOW_NONE,
        ALLOW_BASIC, N_("Unitwaittime style"),
        /* TRANS: The strings between double quotes are also translated
         * separately (they must match!). The strings between single
         * quotes are setting names and shouldn't be translated. The
         * strings between parentheses and in uppercase must stay as
         * untranslated. */
        N_("This setting affects unitwaittime:\n"
           "- \"Activities\" (ACTIVITIES): Units moved less than "
           "'unitwaittime' seconds from turn change will not complete "
           "activities such as pillaging and building roads during "
           "turn change, but during the next turn when their wait "
           "expires."),
        nullptr, nullptr, unitwaittime_name,
        GAME_DEFAULT_UNITWAITTIME_STYLE),

    GEN_BOOL(
        "unitwaittime_extended", game.server.unitwaittime_extended,
        SSET_RULES_FLEXIBLE, SSET_INTERNAL, SSET_VITAL, ALLOW_NONE,
        ALLOW_BASIC,
        N_("Unitwaittime also applies to newly-built and captured/bribed "
           "units."),
        N_("If set, newly-built units are subject to unitwaittime so that "
           "the moment the city production was last touched counts as their "
           "last \"action\". Also, getting captured/bribed counts as action "
           "for the victim. "),
        nullptr, nullptr, GAME_DEFAULT_UNITWAITTIME_EXTENDED),

    /* This setting points to the "stored" value; changing it won't have an
       effect until the next synchronization point (i.e., the start of the
       next turn). */
    GEN_ENUM("phasemode", game.server.phase_mode_stored, SSET_META,
             SSET_INTERNAL, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Control of simultaneous player/team phases"),
             N_("This setting controls whether players may make moves at "
                "the same time during a turn. Change in setting takes "
                "effect next turn."),
             phasemode_help, nullptr, nullptr, phasemode_name,
             GAME_DEFAULT_PHASE_MODE),

    GEN_INT("netwait", game.server.netwait, SSET_META, SSET_NETWORK,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC,
            N_("Max seconds for network buffers to drain"),
            N_("The server will wait for up to the value of this parameter "
               "in seconds, for all client connection network buffers to "
               "unblock. Zero means the server will not wait at all."),
            nullptr, nullptr, nullptr, GAME_MIN_NETWAIT, GAME_MAX_NETWAIT,
            GAME_DEFAULT_NETWAIT),

    GEN_INT("pingtime", game.server.pingtime, SSET_META, SSET_NETWORK,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC, N_("Seconds between PINGs"),
            N_("The server will poll the clients with a PING request each "
               "time this period elapses."),
            nullptr, nullptr, nullptr, GAME_MIN_PINGTIME, GAME_MAX_PINGTIME,
            GAME_DEFAULT_PINGTIME),

    GEN_INT("pingtimeout", game.server.pingtimeout, SSET_META, SSET_NETWORK,
            SSET_RARE, ALLOW_NONE, ALLOW_BASIC, N_("Time to cut a client"),
            N_("If a client doesn't reply to a PING in this time the client "
               "is disconnected."),
            nullptr, nullptr, nullptr, GAME_MIN_PINGTIMEOUT,
            GAME_MAX_PINGTIMEOUT, GAME_DEFAULT_PINGTIMEOUT),

    GEN_BOOL("turnblock", game.server.turnblock, SSET_META, SSET_INTERNAL,
             SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Turn-blocking game play mode"),
             N_("If this is turned on, the game turn is not advanced until "
                "all players have finished their turn, including "
                "disconnected players."),
             nullptr, nullptr, GAME_DEFAULT_TURNBLOCK),

    GEN_BOOL("fixedlength", game.server.fixedlength, SSET_META,
             SSET_INTERNAL, SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
             N_("Fixed-length turns play mode"),
             /* TRANS: \"Turn Done\" refers to the client button; it is also
                translated separately, the translation should be the same. */
             N_("If this is turned on the game turn will not advance until "
                "the timeout has expired, even after all players have "
                "clicked on \"Turn Done\"."),
             nullptr, nullptr, false),

    GEN_STRING(
        "demography", game.server.demography, SSET_META, SSET_INTERNAL,
        SSET_SITUATIONAL, ALLOW_NONE, ALLOW_BASIC,
        N_("What is in the Demographics report"),
        // TRANS: The strings between double quotes should be translated.
        N_("This should be a string of characters, each of which specifies "
           "the inclusion of a line of information in the Demographics "
           "report.\n"
           "The characters and their meanings are:\n"
           "    s = include Score\n"
           "    z = include League Score\n"
           "    N = include Population\n"
           "    n = include Population in Citizen Units\n"
           "    c = include Cities\n"
           "    i = include Improvements\n"
           "    w = include Wonders\n"
           "    A = include Land Area\n"
           "    S = include Settled Area\n"
           "    L = include Literacy\n"
           "    a = include Agriculture\n"
           "    P = include Production\n"
           "    E = include Economics\n"
           "    g = include Gold Income\n"
           "    R = include Research Speed\n"
           "    M = include Military Service\n"
           "    m = include Military Units\n"
           "    u = include Built Units\n"
           "    k = include Killed Units\n"
           "    l = include Lost Units\n"
           "    O = include Pollution\n"
           "    C = include Culture\n"
           "Additionally, the following characters control whether or not "
           "certain columns are displayed in the report:\n"
           "    q = display \"quantity\" column\n"
           "    r = display \"rank\" column\n"
           "    b = display \"best nation\" column\n"
           "The order of characters is not significant, but their "
           "capitalization is."),
        demography_callback, nullptr, GAME_DEFAULT_DEMOGRAPHY),

    GEN_INT("saveturns", game.server.save_nturns, SSET_META, SSET_INTERNAL,
            SSET_VITAL, ALLOW_HACK, ALLOW_HACK, N_("Turns per auto-save"),
            /* TRANS: The string between double quotes is also translated
               separately (it must match!). The string between single quotes
               is a setting name and shouldn't be translated. */
            N_("How many turns elapse between automatic game saves. This "
               "setting only has an effect when the 'autosaves' setting "
               "includes \"New turn\"."),
            nullptr, nullptr, nullptr, GAME_MIN_SAVETURNS,
            GAME_MAX_SAVETURNS, GAME_DEFAULT_SAVETURNS),

    GEN_INT("savefrequency", game.server.save_frequency, SSET_META,
            SSET_INTERNAL, SSET_VITAL, ALLOW_HACK, ALLOW_HACK,
            N_("Minutes per auto-save"),
            /* TRANS: The string between double quotes is also translated
               separately (it must match!). The string between single quotes
               is a setting name and shouldn't be translated. */
            N_("How many minutes elapse between automatic game saves. "
               "Unlike other save types, this save is only meant as backup "
               "for computer memory, and it always uses the same name, "
               "older saves are not kept. This setting only has an effect "
               "when the 'autosaves' setting includes \"Timer\"."),
            nullptr, nullptr, nullptr, GAME_MIN_SAVEFREQUENCY,
            GAME_MAX_SAVEFREQUENCY, GAME_DEFAULT_SAVEFREQUENCY),

    GEN_BITWISE(
        "autosaves", game.server.autosaves, SSET_META, SSET_INTERNAL,
        SSET_VITAL, ALLOW_HACK, ALLOW_HACK,
        N_("Which savegames are generated automatically"),
        /* TRANS: The strings between double quotes are also translated
           separately (they must match!). The strings between single quotes
           are setting names and shouldn't be translated. The strings between
           parentheses and in uppercase must stay as untranslated. */
        N_("This setting controls which autosave types get generated:\n"
           "- \"New turn\" (TURN): Save when turn begins, once every "
           "'saveturns' turns.\n"
           "- \"Game over\" (GAMEOVER): Final save when game ends.\n"
           "- \"No player connections\" (QUITIDLE): Save before server "
           "restarts due to lack of players.\n"
           "- \"Server interrupted\" (INTERRUPT): Save when server quits "
           "due to interrupt.\n"
           "- \"Timer\" (TIMER): Save every 'savefrequency' minutes."),
        autosaves_callback, nullptr, autosaves_name, GAME_DEFAULT_AUTOSAVES),

    GEN_BOOL("threaded_save", game.server.threaded_save, SSET_META,
             SSET_INTERNAL, SSET_RARE, ALLOW_HACK, ALLOW_HACK,
             N_("Whether to do saving in separate thread"),
             /* TRANS: The string between single quotes is a setting name and
                should not be translated. */
             N_("If this is turned in, compressing and saving the actual "
                "file containing the game situation takes place in the "
                "background while game otherwise continues. This way users "
                "are not required to wait for the save to finish."),
             nullptr, nullptr, GAME_DEFAULT_THREADED_SAVE),

    GEN_ENUM("compresstype", game.server.save_compress_type, SSET_META,
             SSET_INTERNAL, SSET_RARE, ALLOW_HACK, ALLOW_HACK,
             N_("Savegame compression algorithm"),
             N_("Compression library to use for savegames."), nullptr,
             compresstype_callback, nullptr, compresstype_name,
             GAME_DEFAULT_COMPRESS_TYPE),

    GEN_STRING(
        "savename", game.server.save_name, SSET_META, SSET_INTERNAL,
        SSET_VITAL, ALLOW_HACK, ALLOW_HACK,
        N_("Definition of the save file name"),
        /* TRANS: %R, %S, %T and %Y must not be translated. The strings
         * (examples and setting names) between single quotes neither. The
         * strings between <> should be translated. xgettext:no-c-format
         */
        N_("Within the string the following custom formats are allowed:\n"
           "  %R = <reason>\n"
           "  %S = <suffix>\n"
           "  %T = <turn-number>\n"
           "  %Y = <game-year>\n"
           "\n"
           "Example: 'freeciv-T%04T-Y%+05Y-%R' => "
           "'freeciv-T0100-Y00001-manual'\n"
           "\n"
           "Be careful to use at least one of %T and %Y, else newer "
           "savegames will overwrite old ones. If none of the formats is "
           "used '-T%04T-Y%05Y-%R' is appended to the value of 'savename'."),
        savename_validate, nullptr, GAME_DEFAULT_SAVE_NAME),

    GEN_BOOL(
        "scorelog", game.server.scorelog, SSET_META, SSET_INTERNAL,
        SSET_SITUATIONAL, ALLOW_HACK, ALLOW_HACK,
        N_("Whether to log player statistics"),
        /* TRANS: The string between single quotes is a setting name and
         * should not be translated.
         */
        N_("If this is turned on, player statistics are appended to the "
           "file defined by the option 'scorefile' every turn. These "
           "statistics can be used to create power graphs after the game."),
        nullptr, scorelog_action, GAME_DEFAULT_SCORELOG),

    GEN_ENUM(
        "scoreloglevel", game.server.scoreloglevel, SSET_META, SSET_INTERNAL,
        SSET_SITUATIONAL, ALLOW_HACK, ALLOW_HACK, N_("Scorelog level"),
        N_("Whether scores are logged for all players including AIs, or "
           "only for human players."),
        nullptr, nullptr, nullptr, scoreloglevel_name,
        GAME_DEFAULT_SCORELOGLEVEL),

    GEN_STRING("scorefile", game.server.scorefile, SSET_META, SSET_INTERNAL,
               SSET_SITUATIONAL, ALLOW_HACK, ALLOW_HACK,
               N_("Name for the score log file"),
               /* TRANS:
                  Don't
                  translate
                  the
                  string
                  in
                  single
                  quotes.
                */
               N_("The default name for the score log file is "
                  "'freeciv-score.log'."),
               scorefile_validate, nullptr, GAME_DEFAULT_SCOREFILE),

    GEN_INT("maxconnectionsperhost", game.server.maxconnectionsperhost,
            SSET_RULES_FLEXIBLE, SSET_NETWORK, SSET_RARE, ALLOW_NONE,
            ALLOW_BASIC,
            N_("Maximum number of connections to the server per host"),
            N_("New connections from a given host will be rejected if the "
               "total number of connections from the very same host equals "
               "or exceeds this value. A value of 0 means that there is no "
               "limit, at least up to the maximum number of connections "
               "supported by the server."),
            nullptr, nullptr, nullptr, GAME_MIN_MAXCONNECTIONSPERHOST,
            GAME_MAX_MAXCONNECTIONSPERHOST,
            GAME_DEFAULT_MAXCONNECTIONSPERHOST),

    GEN_INT("kicktime", game.server.kick_time, SSET_RULES_FLEXIBLE,
            SSET_NETWORK, SSET_RARE, ALLOW_HACK, ALLOW_HACK,
            N_("Time before a kicked user can reconnect"),
            /* TRANS: the string in double quotes is a server command name
               and should not be translated */
            N_("Gives the time in seconds before a user kicked using the "
               "\"kick\" command may reconnect. Changing this setting will "
               "affect users kicked in the past."),
            nullptr, nullptr, nullptr, GAME_MIN_KICK_TIME,
            GAME_MAX_KICK_TIME, GAME_DEFAULT_KICK_TIME),

    GEN_STRING(
        "metamessage", game.server.meta_info.user_message, SSET_META,
        SSET_INTERNAL, SSET_RARE, ALLOW_CTRL, ALLOW_CTRL,
        N_("Metaserver info line"),
        N_("User defined metaserver info line. For most of the time a user "
           "defined metamessage will be used instead of an automatically "
           "generated message. Set to empty (\"\", not \"empty\") to always "
           "use an automatically generated meta server message."),
        nullptr, metamessage_action, GAME_DEFAULT_USER_META_MESSAGE)};

#undef GEN_BOOL
#undef GEN_INT
#undef GEN_STRING
#undef GEN_ENUM
#undef GEN_BITWISE

// The number of settings, not including the END.
static const int SETTINGS_NUM = ARRAY_SIZE(settings);

/**
   Returns the setting to the given id.
 */
struct setting *setting_by_number(int id)
{
  return (0 <= id && id < SETTINGS_NUM ? settings + id : nullptr);
}

/**
   Returns the setting to the given name.
 */
struct setting *setting_by_name(const char *name)
{
  fc_assert_ret_val(name, nullptr);

  settings_iterate(SSET_ALL, pset)
  {
    if (0 == strcmp(name, pset->name)) {
      return pset;
    }
  }
  settings_iterate_end;
  return nullptr;
}

/**
   Returns the id to the given setting.
 */
int setting_number(const struct setting *pset)
{
  fc_assert_ret_val(pset != nullptr, 0);
  return pset - settings;
}

/**
   Access function for the setting name.
 */
const char *setting_name(const struct setting *pset) { return pset->name; }

/**
   Access function for the short help (not translated yet) of the setting.
 */
const char *setting_short_help(const struct setting *pset)
{
  return pset->short_help;
}

/**
   Access function for the long (extra) help of the setting.
   If 'constant' is TRUE, static, not-yet-translated string is always
 returned.
 */
const char *setting_extra_help(const struct setting *pset, bool constant)
{
  if (!constant && pset->help_func != nullptr) {
    return pset->help_func(pset);
  }

  return _(pset->extra_help);
}

/**
   Access function for the setting type.
 */
enum sset_type setting_type(const struct setting *pset)
{
  return pset->stype;
}

/**
   Access function for the setting level (used by the /show command).
 */
enum sset_level setting_level(const struct setting *pset)
{
  return pset->slevel;
}

/**
   Access function for the setting category.
 */
enum sset_category setting_category(const struct setting *pset)
{
  return pset->scategory;
}

/**
   Returns whether the specified server setting (option) can currently
   be changed without breaking data consistency (map dimension options
   can't change when map has already been created with certain dimensions)
 */
static bool setting_is_free_to_change(const struct setting *pset,
                                      char *reject_msg,
                                      size_t reject_msg_len)
{
  switch (pset->sclass) {
  case SSET_MAP_SIZE:
  case SSET_MAP_GEN:
    // Only change map options if we don't yet have a map:
    if (map_is_empty()) {
      return true;
    }

    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The setting '%s' can't be modified after the map is fixed."),
        setting_name(pset));
    return false;

  case SSET_RULES_SCENARIO:
    /* Like SSET_RULES except that it can be changed before the game starts
     * for heavy scenarios. A heavy scenario comes with players. It can
     * include cities, units, diplomatic relations and other complex state.
     * Make sure that changing a setting can't make the state of a heavy
     * scenario illegal if you want to change it from SSET_RULES to
     * SSET_RULES_SCENARIO. */

    if (game.scenario.is_scenario && game.scenario.players
        && server_state() == S_S_INITIAL) {
      // Special case detected.
      return true;
    }

    /* The special case didn't make it legal to change the setting. Don't
     * give up. It could still be legal. Fall through so the non special
     * cases are checked too. */
    fc__fallthrough;

  case SSET_MAP_ADD:
  case SSET_PLAYERS:
  case SSET_GAME_INIT:
  case SSET_RULES:
    /* Only change start params and most rules if we don't yet have a map, or
     * if we do have a map but its a scenario one (ie, the game has never
     * actually been started).
     */
    if (map_is_empty() || game.info.is_new_game) {
      return true;
    }

    settings_snprintf(
        reject_msg, reject_msg_len,
        _("The setting '%s' can't be modified after the game has started."),
        setting_name(pset));
    return false;

  case SSET_RULES_FLEXIBLE:
  case SSET_META:
    // These can always be changed:
    return true;
  }

  qCritical("Wrong class variant for setting %s (%d): %d.",
            setting_name(pset), setting_number(pset), pset->sclass);
  settings_snprintf(reject_msg, reject_msg_len, _("Internal error."));

  return false;
}

/**
   Returns whether the specified server setting (option) can currently
   be changed by the caller. If it returns FALSE, the reason of the failure
   is available by the function setting_error().
 */
bool setting_is_changeable(const struct setting *pset,
                           struct connection *caller, char *reject_msg,
                           size_t reject_msg_len)
{
  if (caller && (caller->access_level < pset->access_level_write)) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("You are not allowed to change the setting '%s'."),
                      setting_name(pset));
    return false;
  }

  if (setting_locked(pset)) {
    // setting is locked by the ruleset
    settings_snprintf(reject_msg, reject_msg_len,
                      _("The setting '%s' is locked by the ruleset."),
                      setting_name(pset));
    return false;
  }

  return setting_is_free_to_change(pset, reject_msg, reject_msg_len);
}

/**
   Returns whether the specified server setting (option) can be seen by a
   caller with the specified access level.
 */
bool setting_is_visible_at_level(const struct setting *pset,
                                 enum cmdlevel plevel)
{
  return (plevel >= pset->access_level_read);
}

/**
   Returns whether the specified server setting (option) can be seen by the
   caller.
 */
bool setting_is_visible(const struct setting *pset,
                        struct connection *caller)
{
  return (!caller
          || setting_is_visible_at_level(pset, caller->access_level));
}

/**
   Convert the string prefix to an integer representation.
   NB: This function is used for SST_ENUM *and* SST_BITWISE.

   FIXME: this mostly duplicate match_prefix_full().
 */
static enum m_pre_result
setting_match_prefix_base(const val_name_func_t name_fn, const char *prefix,
                          int *ind_result, QVector<QString> &matches,
                          size_t max_matches, size_t *pnum_matches)
{
  const struct sset_val_name *name;
  size_t len = qstrlen(prefix);
  size_t num_matches;
  int i;

  *pnum_matches = 0;

  if (0 == len) {
    return M_PRE_EMPTY;
  }

  for (i = 0, num_matches = 0; (name = name_fn(i)); i++) {
    if (0 == fc_strncasecmp(name->support, prefix, len)) {
      if (strlen(name->support) == len) {
        *ind_result = i;
        return M_PRE_EXACT;
      }
      if (num_matches < max_matches) {
        matches.append(name->support);
        (*pnum_matches)++;
      }
      if (0 == num_matches++) {
        *ind_result = i;
      }
    }
  }

  if (1 == num_matches) {
    return M_PRE_ONLY;
  } else if (1 < num_matches) {
    return M_PRE_AMBIGUOUS;
  } else {
    return M_PRE_FAIL;
  }
}

/**
   Convert the string prefix to an integer representation.
   NB: This function is used for SST_ENUM *and* SST_BITWISE.
 */
static bool setting_match_prefix(const val_name_func_t name_fn,
                                 const char *prefix, int *pvalue,
                                 char *reject_msg, size_t reject_msg_len)
{
  QVector<QString> matches;
  matches.reserve(16);
  size_t num_matches;

  switch (setting_match_prefix_base(name_fn, prefix, pvalue, matches, 16,
                                    &num_matches)) {
  case M_PRE_EXACT:
  case M_PRE_ONLY:
    return true; // Ok.
  case M_PRE_AMBIGUOUS: {
    fc_assert(2 <= num_matches);
    settings_snprintf(reject_msg, reject_msg_len,
                      _("\"%s\" prefix is ambiguous. Candidates are: %s."),
                      prefix, qUtf8Printable(strvec_to_and_list(matches)));
  }
    return false;
  case M_PRE_EMPTY:
    settings_snprintf(reject_msg, reject_msg_len, _("Missing value."));
    return false;
  case M_PRE_LONG:
  case M_PRE_FAIL:
  case M_PRE_LAST:
    break;
  }

  settings_snprintf(reject_msg, reject_msg_len, _("No match for \"%s\"."),
                    prefix);
  return false;
}

/**
   Compute the string representation of the value for this boolean setting.
 */
static const char *setting_bool_to_str(const struct setting *pset,
                                       bool value, bool pretty, char *buf,
                                       size_t buf_len)
{
  const struct sset_val_name *name = pset->boolean.name(value);

  if (pretty) {
    fc_snprintf(buf, buf_len, "%s", Q_(name->pretty));
  } else {
    fc_strlcpy(buf, name->support, buf_len);
  }
  return buf;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.

   FIXME: also check the access level of pconn.
 */
static bool setting_bool_validate_base(const struct setting *pset,
                                       const char *val, int *pint_val,
                                       struct connection *caller,
                                       char *reject_msg,
                                       size_t reject_msg_len)
{
  char buf[256];

  if (SST_BOOL != pset->stype) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("This setting is not a boolean."));
    return false;
  }

  sz_strlcpy(buf, val);
  remove_leading_trailing_spaces(buf);

  return (setting_match_prefix(pset->boolean.name, buf, pint_val, reject_msg,
                               reject_msg_len)
          && (nullptr == pset->boolean.validate
              || pset->boolean.validate(0 != *pint_val, caller, reject_msg,
                                        reject_msg_len)));
}

/**
   Set the setting to 'val'. Returns TRUE on success. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.
 */
bool setting_bool_set(struct setting *pset, const char *val,
                      struct connection *caller, char *reject_msg,
                      size_t reject_msg_len)
{
  int int_val;

  if (!setting_is_changeable(pset, caller, reject_msg, reject_msg_len)
      || !setting_bool_validate_base(pset, val, &int_val, caller, reject_msg,
                                     reject_msg_len)) {
    return false;
  }

  *pset->boolean.pvalue = (0 != int_val);
  return true;
}

/**
   Get value of boolean setting
 */
bool setting_bool_get(struct setting *pset)
{
  fc_assert(setting_type(pset) == SST_BOOL);

  return *pset->boolean.pvalue;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.
 */
bool setting_bool_validate(const struct setting *pset, const char *val,
                           struct connection *caller, char *reject_msg,
                           size_t reject_msg_len)
{
  int int_val;

  return setting_bool_validate_base(pset, val, &int_val, caller, reject_msg,
                                    reject_msg_len);
}

/**
   Convert the integer to the long support string representation of a boolean
   setting. This function must match the secfile_enum_name_data_fn_t type.
 */
static const char *setting_bool_secfile_str(secfile_data_t data, int val)
{
  const struct sset_val_name *name =
      (static_cast<const struct setting *>(data))->boolean.name(val);

  return (nullptr != name ? name->support : nullptr);
}

/**
   Compute the string representation of the value for this integer setting.
 */
static const char *setting_int_to_str(const struct setting *pset, int value,
                                      bool pretty, char *buf, size_t buf_len)
{
  fc_snprintf(buf, buf_len, "%d", value);
  return buf;
}

/**
   Returns the minimal integer value for this setting.
 */
int setting_int_min(const struct setting *pset)
{
  fc_assert_ret_val(pset->stype == SST_INT, 0);
  return pset->integer.min_value;
}

/**
   Returns the maximal integer value for this setting.
 */
int setting_int_max(const struct setting *pset)
{
  fc_assert_ret_val(pset->stype == SST_INT, 0);
  return pset->integer.max_value;
}

/**
   Set the setting to 'val'. Returns TRUE on success. If it fails, the
   reason of the failure is available by the function setting_error().
 */
bool setting_int_set(struct setting *pset, int val,
                     struct connection *caller, char *reject_msg,
                     size_t reject_msg_len)
{
  if (!setting_is_changeable(pset, caller, reject_msg, reject_msg_len)
      || !setting_int_validate(pset, val, caller, reject_msg,
                               reject_msg_len)) {
    return false;
  }

  *pset->integer.pvalue = val;
  return true;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available by the function setting_error().

   FIXME: also check the access level of pconn.
 */
bool setting_int_validate(const struct setting *pset, int val,
                          struct connection *caller, char *reject_msg,
                          size_t reject_msg_len)
{
  if (SST_INT != pset->stype) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("This setting is not an integer."));
    return false;
  }

  if (val < pset->integer.min_value || val > pset->integer.max_value) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("Value out of range: %d (min: %d; max: %d)."), val,
                      pset->integer.min_value, pset->integer.max_value);
    return false;
  }

  return (
      !pset->integer.validate
      || pset->integer.validate(val, caller, reject_msg, reject_msg_len));
}

/**
   Get value of integer setting
 */
int setting_int_get(struct setting *pset)
{
  fc_assert(setting_type(pset) == SST_INT);

  return *pset->integer.pvalue;
}

/**
   Compute the string representation of the value for this string setting.
 */
static const char *setting_str_to_str(const struct setting *pset,
                                      const char *value, bool pretty,
                                      char *buf, size_t buf_len)
{
  if (pretty) {
    fc_snprintf(buf, buf_len, "\"%s\"", value);
  } else {
    fc_strlcpy(buf, value, buf_len);
  }
  return buf;
}

/**
   Set the setting to 'val'. Returns TRUE on success. If it fails, the
   reason of the failure is available by the function setting_error().
 */
bool setting_str_set(struct setting *pset, const char *val,
                     struct connection *caller, char *reject_msg,
                     size_t reject_msg_len)
{
  if (!setting_is_changeable(pset, caller, reject_msg, reject_msg_len)
      || !setting_str_validate(pset, val, caller, reject_msg,
                               reject_msg_len)) {
    return false;
  }

  fc_strlcpy(pset->string.value, val, pset->string.value_size);
  return true;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available by the function setting_error().

   FIXME: also check the access level of pconn.
 */
bool setting_str_validate(const struct setting *pset, const char *val,
                          struct connection *caller, char *reject_msg,
                          size_t reject_msg_len)
{
  if (SST_STRING != pset->stype) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("This setting is not a string."));
    return false;
  }

  if (strlen(val) >= pset->string.value_size) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("String value too long (max length: %lu)."),
                      (unsigned long) pset->string.value_size);
    return false;
  }

  return (!pset->string.validate
          || pset->string.validate(val, caller, reject_msg, reject_msg_len));
}

/**
   Get value of string setting
 */
char *setting_str_get(struct setting *pset)
{
  fc_assert(setting_type(pset) == SST_STRING);

  return pset->string.value;
}

/**
   Convert the integer to the long support string representation of an
   enumerator. This function must match the secfile_enum_name_data_fn_t type.
 */
const char *setting_enum_secfile_str(secfile_data_t data, int val)
{
  const struct sset_val_name *name =
      (static_cast<const struct setting *>(data))->enumerator.name(val);

  return (nullptr != name ? name->support : nullptr);
}

/**
   Convert the integer to the string representation of an enumerator.
   Return nullptr if 'val' is not a valid enumerator.
 */
const char *setting_enum_val(const struct setting *pset, int val,
                             bool pretty)
{
  const struct sset_val_name *name;

  fc_assert_ret_val(SST_ENUM == pset->stype, nullptr);
  name = pset->enumerator.name(val);
  if (nullptr == name) {
    return nullptr;
  } else if (pretty) {
    return _(name->pretty);
  } else {
    return name->support;
  }
}

/**
   Compute the string representation of the value for this enumerator
   setting.
 */
static const char *setting_enum_to_str(const struct setting *pset, int value,
                                       bool pretty, char *buf,
                                       size_t buf_len)
{
  const struct sset_val_name *name = pset->enumerator.name(value);

  if (pretty) {
    fc_snprintf(buf, buf_len, "\"%s\" (%s)", Q_(name->pretty),
                name->support);
  } else {
    fc_strlcpy(buf, name->support, buf_len);
  }
  return buf;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.

   FIXME: also check the access level of pconn.
 */
static bool setting_enum_validate_base(const struct setting *pset,
                                       const char *val, int *pint_val,
                                       struct connection *caller,
                                       char *reject_msg,
                                       size_t reject_msg_len)
{
  char buf[256];

  if (SST_ENUM != pset->stype) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("This setting is not an enumerator."));
    return false;
  }

  sz_strlcpy(buf, val);
  remove_leading_trailing_spaces(buf);

  return (setting_match_prefix(pset->enumerator.name, buf, pint_val,
                               reject_msg, reject_msg_len)
          && (nullptr == pset->enumerator.validate
              || pset->enumerator.validate(*pint_val, caller, reject_msg,
                                           reject_msg_len)));
}

/**
   Helper function to write value to enumerator setting
 */
static bool set_enum_value(struct setting *pset, int val)
{
  switch (pset->enumerator.store_size) {
  case sizeof(int): {
    int *to_int = static_cast<int *>(pset->enumerator.pvalue);

    *to_int = val;
  } break;
  case sizeof(char): {
    char *to_char = static_cast<char *>(pset->enumerator.pvalue);

    *to_char = static_cast<char>(val);
  } break;
  case sizeof(short): {
    short *to_short = static_cast<short *>(pset->enumerator.pvalue);

    *to_short = static_cast<short>(val);
  } break;
  default:
    return false;
  }

  return true;
}

/**
   Helper function to read value from enumerator setting
 */
int read_enum_value(const struct setting *pset)
{
  int val;

  switch (pset->enumerator.store_size) {
  case sizeof(int):
    val = *(static_cast<int *>(pset->enumerator.pvalue));
    break;
  case sizeof(char):
    val = *(static_cast<char *>(pset->enumerator.pvalue));
    break;
  case sizeof(short):
    val = *(static_cast<short *>(pset->enumerator.pvalue));
    break;
  default:
    qCritical("Illegal enum store size %d, can't read value",
              pset->enumerator.store_size);
    return 0;
  }

  return val;
}

/**
   Set the setting to 'val'. Returns TRUE on success. If it fails, the
   reason of the failure is available in the optionnal parameter
   'reject_msg'.
 */
bool setting_enum_set(struct setting *pset, const char *val,
                      struct connection *caller, char *reject_msg,
                      size_t reject_msg_len)
{
  int int_val;

  if (!setting_is_changeable(pset, caller, reject_msg, reject_msg_len)) {
    return false;
  }

  if (!setting_enum_validate_base(pset, val, &int_val, caller, reject_msg,
                                  reject_msg_len)) {
    return false;
  }

  if (!set_enum_value(pset, int_val)) {
    qCritical("Illegal enumerator value size %d for %s",
              pset->enumerator.store_size, val);
    return false;
  }

  return true;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.
 */
bool setting_enum_validate(const struct setting *pset, const char *val,
                           struct connection *caller, char *reject_msg,
                           size_t reject_msg_len)
{
  int int_val;

  return setting_enum_validate_base(pset, val, &int_val, caller, reject_msg,
                                    reject_msg_len);
}

/**
   Convert the integer to the long support string representation of an
   enumerator. This function must match the secfile_enum_name_data_fn_t type.
 */
const char *setting_bitwise_secfile_str(secfile_data_t data, int bit)
{
  const struct sset_val_name *name =
      (static_cast<const struct setting *>(data))->bitwise.name(bit);

  return (nullptr != name ? name->support : nullptr);
}

/**
   Convert the bit number to its string representation.
   Return nullptr if 'bit' is not a valid bit.
 */
const char *setting_bitwise_bit(const struct setting *pset, int bit,
                                bool pretty)
{
  const struct sset_val_name *name;

  fc_assert_ret_val(SST_BITWISE == pset->stype, nullptr);
  name = pset->bitwise.name(bit);
  if (nullptr == name) {
    return nullptr;
  } else if (pretty) {
    return _(name->pretty);
  } else {
    return name->support;
  }
}

/**
   Compute the string representation of the value for this bitwise setting.
 */
static const char *setting_bitwise_to_str(const struct setting *pset,
                                          unsigned value, bool pretty,
                                          char *buf, size_t buf_len)
{
  const struct sset_val_name *name;
  char *old_buf = buf;
  int bit;

  if (pretty) {
    char buf2[256];
    QVector<QString> vec;
    size_t len;

    for (bit = 0; (name = pset->bitwise.name(bit)); bit++) {
      if ((1 << bit) & value) {
        // TRANS: only emphasizing a string.
        fc_snprintf(buf2, sizeof(buf2), _("\"%s\""), Q_(name->pretty));
        vec.append(buf2);
      }
    }

    if (0 == vec.count()) {
      // No value.
      fc_assert(0 == value);
      // TRANS: Bitwise setting has no bits set.
      fc_strlcpy(buf, _("empty value"), buf_len);
      return buf;
    }
    fc_strlcpy(buf, qUtf8Printable(strvec_to_and_list(vec)), buf_len);
    fc_strlcat(buf, " (", buf_len);
    len = qstrlen(buf);
    buf += len;
    buf_len -= len;
  }

  // Long support part.
  buf[0] = '\0';
  for (bit = 0; (name = pset->bitwise.name(bit)); bit++) {
    if ((1 << bit) & value) {
      if ('\0' != buf[0]) {
        fc_strlcat(buf, "|", buf_len);
      }
      fc_strlcat(buf, name->support, buf_len);
    }
  }

  if (pretty) {
    fc_strlcat(buf, ")", buf_len);
  }
  return old_buf;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.

   FIXME: also check the access level of pconn.
 */
static bool
setting_bitwise_validate_base(const struct setting *pset, const char *val,
                              unsigned *pint_val, struct connection *caller,
                              char *reject_msg, size_t reject_msg_len)
{
  char buf[256];
  const char *p;
  int bit;

  if (SST_BITWISE != pset->stype) {
    settings_snprintf(reject_msg, reject_msg_len,
                      _("This setting is not a bitwise."));
    return false;
  }

  *pint_val = 0;

  // Value names are separated by '|'.
  do {
    p = strchr(val, '|');
    if (nullptr != p) {
      p++;
      fc_strlcpy(buf, val, MIN(p - val, sizeof(buf)));
    } else {
      // Last segment, full copy.
      sz_strlcpy(buf, val);
    }
    remove_leading_trailing_spaces(buf);
    if (nullptr == p && '\0' == buf[0] && 0 == *pint_val) {
      // Empty string = value 0.
      break;
    } else if (!setting_match_prefix(pset->bitwise.name, buf, &bit,
                                     reject_msg, reject_msg_len)) {
      return false;
    }
    *pint_val |= 1 << bit;
    val = p;
  } while (nullptr != p);

  return (nullptr == pset->bitwise.validate
          || pset->bitwise.validate(*pint_val, caller, reject_msg,
                                    reject_msg_len));
}

/**
   Set the setting to 'val'. Returns TRUE on success. If it fails, the
   reason of the failure is available in the optionnal parameter
   'reject_msg'.
 */
bool setting_bitwise_set(struct setting *pset, const char *val,
                         struct connection *caller, char *reject_msg,
                         size_t reject_msg_len)
{
  unsigned int_val;

  if (!setting_is_changeable(pset, caller, reject_msg, reject_msg_len)
      || !setting_bitwise_validate_base(pset, val, &int_val, caller,
                                        reject_msg, reject_msg_len)) {
    return false;
  }

  *pset->bitwise.pvalue = int_val;
  return true;
}

/**
   Returns TRUE if 'val' is a valid value for this setting. If it's not,
   the reason of the failure is available in the optionnal parameter
   'reject_msg'.
 */
bool setting_bitwise_validate(const struct setting *pset, const char *val,
                              struct connection *caller, char *reject_msg,
                              size_t reject_msg_len)
{
  unsigned int_val;

  return setting_bitwise_validate_base(pset, val, &int_val, caller,
                                       reject_msg, reject_msg_len);
}

/**
   Get value of bitwise setting
 */
int setting_bitwise_get(struct setting *pset)
{
  fc_assert(setting_type(pset) == SST_BITWISE);

  return *pset->bitwise.pvalue;
}

/**
   Compute the name of the current value of the setting.
 */
const char *setting_value_name(const struct setting *pset, bool pretty,
                               char *buf, size_t buf_len)
{
  fc_assert_ret_val(nullptr != pset, nullptr);
  fc_assert_ret_val(nullptr != buf, nullptr);
  fc_assert_ret_val(0 < buf_len, nullptr);

  switch (pset->stype) {
  case SST_BOOL:
    return setting_bool_to_str(pset, *pset->boolean.pvalue, pretty, buf,
                               buf_len);
  case SST_INT:
    return setting_int_to_str(pset, *pset->integer.pvalue, pretty, buf,
                              buf_len);
  case SST_STRING:
    return setting_str_to_str(pset, pset->string.value, pretty, buf,
                              buf_len);
  case SST_ENUM:
    return setting_enum_to_str(pset, read_enum_value(pset), pretty, buf,
                               buf_len);
  case SST_BITWISE:
    return setting_bitwise_to_str(pset, *pset->bitwise.pvalue, pretty, buf,
                                  buf_len);
  case SST_COUNT:
    // Error logged below.
    break;
  }

  qCritical("%s(): Setting \"%s\" (nb %d) not handled in switch statement.",
            __FUNCTION__, setting_name(pset), setting_number(pset));
  return nullptr;
}

/**
   Compute the name of the default value of the setting.
 */
const char *setting_default_name(const struct setting *pset, bool pretty,
                                 char *buf, size_t buf_len)
{
  fc_assert_ret_val(nullptr != pset, nullptr);
  fc_assert_ret_val(nullptr != buf, nullptr);
  fc_assert_ret_val(0 < buf_len, nullptr);

  switch (pset->stype) {
  case SST_BOOL:
    return setting_bool_to_str(pset, pset->boolean.default_value, pretty,
                               buf, buf_len);
  case SST_INT:
    return setting_int_to_str(pset, pset->integer.default_value, pretty, buf,
                              buf_len);
  case SST_STRING:
    return setting_str_to_str(pset, pset->string.default_value, pretty, buf,
                              buf_len);
  case SST_ENUM:
    return setting_enum_to_str(pset, pset->enumerator.default_value, pretty,
                               buf, buf_len);
  case SST_BITWISE:
    return setting_bitwise_to_str(pset, pset->bitwise.default_value, pretty,
                                  buf, buf_len);
  case SST_COUNT:
    // Error logged below.
    break;
  }

  qCritical("%s(): Setting \"%s\" (nb %d) not handled in switch statement.",
            __FUNCTION__, setting_name(pset), setting_number(pset));
  return nullptr;
}

/**
   Update the setting to the default value
 */
void setting_set_to_default(struct setting *pset)
{
  switch (pset->stype) {
  case SST_BOOL:
    (*pset->boolean.pvalue) = pset->boolean.default_value;
    break;
  case SST_INT:
    (*pset->integer.pvalue) = pset->integer.default_value;
    break;
  case SST_STRING:
    fc_strlcpy(pset->string.value, pset->string.default_value,
               pset->string.value_size);
    break;
  case SST_ENUM:
    set_enum_value(pset, pset->enumerator.default_value);
    break;
  case SST_BITWISE:
    (*pset->bitwise.pvalue) = pset->bitwise.default_value;
    break;
  case SST_COUNT:
    fc_assert(pset->stype != SST_COUNT);
    break;
  }

  pset->setdef = SETDEF_INTERNAL;
}

/**
   Execute the action callback if needed.
 */
void setting_action(const struct setting *pset)
{
  if (pset->action != nullptr) {
    pset->action(pset);
  }
}

/**
   Load game settings from ruleset file 'game.ruleset'.
 */
bool settings_ruleset(struct section_file *file, const char *section,
                      bool act)
{
  const char *name;
  int j;

  // Unlock all settings.
  settings_iterate(SSET_ALL, pset)
  {
    setting_lock_set(pset, false);
    setting_set_to_default(pset);
  }
  settings_iterate_end;

  // settings
  if (nullptr == secfile_section_by_name(file, section)) {
    // no settings in ruleset file
    qDebug("no [%s] section for game settings in %s", section,
           secfile_name(file));
  } else {
    for (j = 0; (name = secfile_lookup_str_default(
                     file, nullptr, "%s.set%d.name", section, j));
         j++) {
      char path[256];
      fc_snprintf(path, sizeof(path), "%s.set%d", section, j);

      if (!setting_ruleset_one(file, name, path)) {
        qCritical("unknown setting in '%s': %s", secfile_name(file), name);
      }
    }
  }

  /* Execute all setting actions to consider actions due to the default
   * values. */
  if (act) {
    settings_iterate(SSET_ALL, pset) { setting_action(pset); }
    settings_iterate_end;
  }

  autolock_settings();

  // send game settings
  send_server_settings(nullptr);

  return true;
}

/**
   Set one setting from the game.ruleset file.
 */
static bool setting_ruleset_one(struct section_file *file, const char *name,
                                const char *path)
{
  struct setting *pset = nullptr;
  char reject_msg[256], buf[256];
  bool lock;

  settings_iterate(SSET_ALL, pset_check)
  {
    if (0 == fc_strcasecmp(setting_name(pset_check), name)) {
      pset = pset_check;
      break;
    }
  }
  settings_iterate_end;

  if (pset == nullptr) {
    // no setting found
    return false;
  }

  switch (pset->stype) {
  case SST_BOOL: {
    int ival;
    bool val;

    /* Allow string with same boolean representation as accepted on server
     * command line */
    if (secfile_lookup_enum_data(file, &ival, false,
                                 setting_bool_secfile_str, pset, "%s.value",
                                 path)) {
      val = (ival != 0);
    } else if (!secfile_lookup_bool(file, &val, "%s.value", path)) {
      qCritical("Can't read value for setting '%s': %s", name,
                secfile_error());
      break;
    }
    if (val != *pset->boolean.pvalue) {
      if (nullptr == pset->boolean.validate
          || pset->boolean.validate(val, nullptr, reject_msg,
                                    sizeof(reject_msg))) {
        *pset->boolean.pvalue = val;
        qInfo(_("Ruleset: '%s' has been set to %s."), setting_name(pset),
              setting_value_name(pset, true, buf, sizeof(buf)));
      } else {
        qCritical("%s", reject_msg);
      }
    }
  } break;

  case SST_INT: {
    int val;

    if (!secfile_lookup_int(file, &val, "%s.value", path)) {
      qCritical("Can't read value for setting '%s': %s", name,
                secfile_error());
    } else if (val != *pset->integer.pvalue) {
      if (setting_int_set(pset, val, nullptr, reject_msg,
                          sizeof(reject_msg))) {
        qInfo(_("Ruleset: '%s' has been set to %s."), setting_name(pset),
              setting_value_name(pset, true, buf, sizeof(buf)));
      } else {
        qCritical("%s", reject_msg);
      }
    }
  } break;

  case SST_STRING: {
    const char *val = secfile_lookup_str(file, "%s.value", path);

    if (nullptr == val) {
      qCritical("Can't read value for setting '%s': %s", name,
                secfile_error());
    } else if (0 != strcmp(val, pset->string.value)) {
      if (setting_str_set(pset, val, nullptr, reject_msg,
                          sizeof(reject_msg))) {
        qInfo(_("Ruleset: '%s' has been set to %s."), setting_name(pset),
              setting_value_name(pset, true, buf, sizeof(buf)));
      } else {
        qCritical("%s", reject_msg);
      }
    }
  } break;

  case SST_ENUM: {
    int val;

    if (!secfile_lookup_enum_data(file, &val, false,
                                  setting_enum_secfile_str, pset, "%s.value",
                                  path)) {
      qCritical("Can't read value for setting '%s': %s", name,
                secfile_error());
    } else if (val != read_enum_value(pset)) {
      if (nullptr == pset->enumerator.validate
          || pset->enumerator.validate(val, nullptr, reject_msg,
                                       sizeof(reject_msg))) {
        set_enum_value(pset, val);
        qInfo(_("Ruleset: '%s' has been set to %s."), setting_name(pset),
              setting_value_name(pset, true, buf, sizeof(buf)));
      } else {
        qCritical("%s", reject_msg);
      }
    }
  } break;

  case SST_BITWISE: {
    int val;

    if (!secfile_lookup_enum_data(file, &val, true,
                                  setting_bitwise_secfile_str, pset,
                                  "%s.value", path)) {
      qCritical("Can't read value for setting '%s': %s", name,
                secfile_error());
    } else if (val != *pset->bitwise.pvalue) {
      if (nullptr == pset->bitwise.validate
          || pset->bitwise.validate(static_cast<unsigned>(val), nullptr,
                                    reject_msg, sizeof(reject_msg))) {
        *pset->bitwise.pvalue = val;
        qInfo(_("Ruleset: '%s' has been set to %s."), setting_name(pset),
              setting_value_name(pset, true, buf, sizeof(buf)));
      } else {
        qCritical("%s", reject_msg);
      }
    }
  } break;

  case SST_COUNT:
    fc_assert(pset->stype != SST_COUNT);
    break;
  }

  pset->setdef = SETDEF_RULESET;

  // set lock
  lock = secfile_lookup_bool_default(file, false, "%s.lock", path);

  if (lock) {
    // set lock
    setting_lock_set(pset, lock);
    qInfo(_("Ruleset: '%s' has been locked by the ruleset."),
          setting_name(pset));
  }

  return true;
}

/**
   Returns whether the setting has non-default value.
 */
bool setting_non_default(const struct setting *pset)
{
  switch (setting_type(pset)) {
  case SST_BOOL:
    return (*pset->boolean.pvalue != pset->boolean.default_value);
  case SST_INT:
    return (*pset->integer.pvalue != pset->integer.default_value);
  case SST_STRING:
    return (0 != strcmp(pset->string.value, pset->string.default_value));
  case SST_ENUM:
    return (read_enum_value(pset) != pset->enumerator.default_value);
  case SST_BITWISE:
    return (*pset->bitwise.pvalue != pset->bitwise.default_value);
  case SST_COUNT:
    // Error logged below.
    break;
  }

  qCritical("%s(): Setting \"%s\" (nb %d) not handled in switch statement.",
            __FUNCTION__, setting_name(pset), setting_number(pset));
  return false;
}

/**
   Returns if the setting is locked by the ruleset.
 */
bool setting_locked(const struct setting *pset) { return pset->locked; }

/**
   Set the value for the lock of a setting.
 */
void setting_lock_set(struct setting *pset, bool lock)
{
  pset->locked = lock;
}

/**
   Save the setting value of the current game.
 */
static void setting_game_set(struct setting *pset, bool init)
{
  switch (setting_type(pset)) {
  case SST_BOOL:
    pset->boolean.game_value = *pset->boolean.pvalue;
    break;

  case SST_INT:
    pset->integer.game_value = *pset->integer.pvalue;
    break;

  case SST_STRING:
    if (init) {
      pset->string.game_value = new char[pset->string.value_size]{};
    }
    fc_strlcpy(pset->string.game_value, pset->string.value,
               pset->string.value_size);
    break;

  case SST_ENUM:
    pset->enumerator.game_value = read_enum_value(pset);
    break;

  case SST_BITWISE:
    pset->bitwise.game_value = *pset->bitwise.pvalue;
    break;

  case SST_COUNT:
    fc_assert(setting_type(pset) != SST_COUNT);
    break;
  }
}

/**
   Free the memory used for the settings at game start.
 */
static void setting_game_free(struct setting *pset)
{
  if (setting_type(pset) == SST_STRING) {
    delete[] pset->string.game_value;
    pset->string.game_value = nullptr;
  }
}

/**
   Restore the setting to the value used at the start of the current game.
 */
static void setting_game_restore(struct setting *pset)
{
  char reject_msg[256] = "", buf[256];
  bool res = false;

  if (!setting_is_changeable(pset, nullptr, reject_msg,
                             sizeof(reject_msg))) {
    log_debug("Can't restore '%s': %s", setting_name(pset), reject_msg);
    return;
  }

  switch (setting_type(pset)) {
  case SST_BOOL:
    res = (nullptr
               != setting_bool_to_str(pset, pset->boolean.game_value, false,
                                      buf, sizeof(buf))
           && setting_bool_set(pset, buf, nullptr, reject_msg,
                               sizeof(reject_msg)));
    break;

  case SST_INT:
    res = setting_int_set(pset, pset->integer.game_value, nullptr,
                          reject_msg, sizeof(reject_msg));
    break;

  case SST_STRING:
    res = setting_str_set(pset, pset->string.game_value, nullptr, reject_msg,
                          sizeof(reject_msg));
    break;

  case SST_ENUM:
    res = (nullptr
               != setting_enum_to_str(pset, pset->enumerator.game_value,
                                      false, buf, sizeof(buf))
           && setting_enum_set(pset, buf, nullptr, reject_msg,
                               sizeof(reject_msg)));
    break;

  case SST_BITWISE:
    res = (nullptr
               != setting_bitwise_to_str(pset, pset->bitwise.game_value,
                                         false, buf, sizeof(buf))
           && setting_bitwise_set(pset, buf, nullptr, reject_msg,
                                  sizeof(reject_msg)));
    break;

  case SST_COUNT:
    res = false;
    break;
  }

  if (!res) {
    qCritical(
        "Error restoring setting '%s' to the value from game start: %s",
        setting_name(pset), reject_msg);
  }
}

/**
   Save setting values at the start of  the game.
 */
void settings_game_start()
{
  settings_iterate(SSET_ALL, pset) { setting_game_set(pset, false); }
  settings_iterate_end;

  // Settings from the start of the game are saved.
  game.server.settings_gamestart_valid = true;
}

/**
   Save game settings.
 */
void settings_game_save(struct section_file *file, const char *section)
{
  int set_count = 0;

  settings_iterate(SSET_ALL, pset)
  {
    char errbuf[200];

    if ( // It's explicitly set to some value to save
        setting_get_setdef(pset) == SETDEF_CHANGED
        /* It must be same at loading time as it was saving time, even if
           freeciv's default has changed. */
        || !setting_is_free_to_change(pset, errbuf, sizeof(errbuf))) {
      secfile_insert_str(file, setting_name(pset), "%s.set%d.name", section,
                         set_count);
      switch (setting_type(pset)) {
      case SST_BOOL:
        secfile_insert_bool(file, *pset->boolean.pvalue, "%s.set%d.value",
                            section, set_count);
        secfile_insert_bool(file, pset->boolean.game_value,
                            "%s.set%d.gamestart", section, set_count);
        break;
      case SST_INT:
        secfile_insert_int(file, *pset->integer.pvalue, "%s.set%d.value",
                           section, set_count);
        secfile_insert_int(file, pset->integer.game_value,
                           "%s.set%d.gamestart", section, set_count);
        break;
      case SST_STRING:
        secfile_insert_str(file, pset->string.value, "%s.set%d.value",
                           section, set_count);
        secfile_insert_str(file, pset->string.game_value,
                           "%s.set%d.gamestart", section, set_count);
        break;
      case SST_ENUM:
        secfile_insert_enum_data(file, read_enum_value(pset), false,
                                 setting_enum_secfile_str, pset,
                                 "%s.set%d.value", section, set_count);
        secfile_insert_enum_data(file, pset->enumerator.game_value, false,
                                 setting_enum_secfile_str, pset,
                                 "%s.set%d.gamestart", section, set_count);
        break;
      case SST_BITWISE:
        secfile_insert_enum_data(file, *pset->bitwise.pvalue, true,
                                 setting_bitwise_secfile_str, pset,
                                 "%s.set%d.value", section, set_count);
        secfile_insert_enum_data(file, pset->bitwise.game_value, true,
                                 setting_bitwise_secfile_str, pset,
                                 "%s.set%d.gamestart", section, set_count);
        break;
      case SST_COUNT:
        fc_assert(setting_type(pset) != SST_COUNT);
        secfile_insert_str(file, "Unknown setting type", "%s.set%d.value",
                           section, set_count);
        secfile_insert_str(file, "Unknown setting type",
                           "%s.set%d.gamestart", section, set_count);
        break;
      }
      set_count++;
    }
  }
  settings_iterate_end;

  secfile_insert_int(file, set_count, "%s.set_count", section);
  secfile_insert_bool(file, game.server.settings_gamestart_valid,
                      "%s.gamestart_valid", section);
}

/**
   Restore all settings from a savegame.
 */
void settings_game_load(struct section_file *file, const char *section)
{
  const char *name;
  char reject_msg[256], buf[256];
  int i, set_count;
  int oldcitymindist = game.info.citymindist; // backwards compat, see below

  /* Compatibility with savegames created with older versions is usually
   * handled as conversions in savecompat.c compat_load_<version>() */

  if (!secfile_lookup_int(file, &set_count, "%s.set_count", section)) {
    // Old savegames and scenarios doesn't contain this, not an error.
    qDebug("Can't read the number of settings in the save file.");
    return;
  }

  // Check if the saved settings are valid settings from game start.
  game.server.settings_gamestart_valid = secfile_lookup_bool_default(
      file, false, "%s.gamestart_valid", section);

  for (i = 0; i < set_count; i++) {
    name = secfile_lookup_str(file, "%s.set%d.name", section, i);

    settings_iterate(SSET_ALL, pset)
    {
      if (fc_strcasecmp(setting_name(pset), name) != 0) {
        continue;
      }

      // Load the current value of the setting.
      switch (pset->stype) {
      case SST_BOOL: {
        bool val;

        if (!secfile_lookup_bool(file, &val, "%s.set%d.value", section, i)) {
          qDebug("Option '%s' not defined in the savegame: %s", name,
                 secfile_error());
        } else {
          pset->setdef = SETDEF_CHANGED;

          if (val != *pset->boolean.pvalue) {
            if (setting_is_changeable(pset, nullptr, reject_msg,
                                      sizeof(reject_msg))
                && (nullptr == pset->boolean.validate
                    || pset->boolean.validate(val, nullptr, reject_msg,
                                              sizeof(reject_msg)))) {
              *pset->boolean.pvalue = val;
              qInfo(_("Savegame: '%s' has been set to %s."),
                    setting_name(pset),
                    setting_value_name(pset, true, buf, sizeof(buf)));
            } else {
              qCritical("Savegame: error restoring '%s' . (%s)",
                        setting_name(pset), reject_msg);
            }
          } else {
            qInfo(
                _("Savegame: '%s' explicitly set to value same as default."),
                setting_name(pset));
          }
        }
      } break;

      case SST_INT: {
        int val;

        if (!secfile_lookup_int(file, &val, "%s.set%d.value", section, i)) {
          qDebug("Option '%s' not defined in the savegame: %s", name,
                 secfile_error());
        } else {
          pset->setdef = SETDEF_CHANGED;

          if (val != *pset->integer.pvalue) {
            if (setting_is_changeable(pset, nullptr, reject_msg,
                                      sizeof(reject_msg))
                && (nullptr == pset->integer.validate
                    || pset->integer.validate(val, nullptr, reject_msg,
                                              sizeof(reject_msg)))) {
              *pset->integer.pvalue = val;
              qInfo(_("Savegame: '%s' has been set to %s."),
                    setting_name(pset),
                    setting_value_name(pset, true, buf, sizeof(buf)));
            } else {
              qCritical("Savegame: error restoring '%s' . (%s)",
                        setting_name(pset), reject_msg);
            }
          } else {
            qInfo(
                _("Savegame: '%s' explicitly set to value same as default."),
                setting_name(pset));
          }
        }
      } break;

      case SST_STRING: {
        const char *val =
            secfile_lookup_str(file, "%s.set%d.value", section, i);

        if (nullptr == val) {
          qDebug("Option '%s' not defined in the savegame: %s", name,
                 secfile_error());
        } else {
          pset->setdef = SETDEF_CHANGED;

          if (0 != strcmp(val, pset->string.value)) {
            if (setting_str_set(pset, val, nullptr, reject_msg,
                                sizeof(reject_msg))) {
              qInfo(_("Savegame: '%s' has been set to %s."),
                    setting_name(pset),
                    setting_value_name(pset, true, buf, sizeof(buf)));
            } else {
              qCritical("Savegame: error restoring '%s' . (%s)",
                        setting_name(pset), reject_msg);
            }
          } else {
            qInfo(
                _("Savegame: '%s' explicitly set to value same as default."),
                setting_name(pset));
          }
        }
      } break;

      case SST_ENUM: {
        int val;

        if (!secfile_lookup_enum_data(file, &val, false,
                                      setting_enum_secfile_str, pset,
                                      "%s.set%d.value", section, i)) {
          qDebug("Option '%s' not defined in the savegame: %s", name,
                 secfile_error());
        } else {
          pset->setdef = SETDEF_CHANGED;

          if (val != read_enum_value(pset)) {
            if (setting_is_changeable(pset, nullptr, reject_msg,
                                      sizeof(reject_msg))
                && (nullptr == pset->enumerator.validate
                    || pset->enumerator.validate(val, nullptr, reject_msg,
                                                 sizeof(reject_msg)))) {
              set_enum_value(pset, val);
              qInfo(_("Savegame: '%s' has been set to %s."),
                    setting_name(pset),
                    setting_value_name(pset, true, buf, sizeof(buf)));
            } else {
              qCritical("Savegame: error restoring '%s' . (%s)",
                        setting_name(pset), reject_msg);
            }
          } else {
            qInfo(
                _("Savegame: '%s' explicitly set to value same as default."),
                setting_name(pset));
          }
        }
      } break;

      case SST_BITWISE: {
        int val;

        if (!secfile_lookup_enum_data(file, &val, true,
                                      setting_bitwise_secfile_str, pset,
                                      "%s.set%d.value", section, i)) {
          qDebug("Option '%s' not defined in the savegame: %s", name,
                 secfile_error());
        } else {
          pset->setdef = SETDEF_CHANGED;

          if (val != *pset->bitwise.pvalue) {
            if (setting_is_changeable(pset, nullptr, reject_msg,
                                      sizeof(reject_msg))
                && (nullptr == pset->bitwise.validate
                    || pset->bitwise.validate(val, nullptr, reject_msg,
                                              sizeof(reject_msg)))) {
              *pset->bitwise.pvalue = val;
              qInfo(_("Savegame: '%s' has been set to %s."),
                    setting_name(pset),
                    setting_value_name(pset, true, buf, sizeof(buf)));
            } else {
              qCritical("Savegame: error restoring '%s' . (%s)",
                        setting_name(pset), reject_msg);
            }
          } else {
            qInfo(
                _("Savegame: '%s' explicitly set to value same as default."),
                setting_name(pset));
          }
        }
      } break;

      case SST_COUNT:
        fc_assert(pset->stype != SST_COUNT);
        break;
      }

      if (game.server.settings_gamestart_valid) {
        // Load the value of the setting at the start of the game.
        switch (pset->stype) {
        case SST_BOOL:
          pset->boolean.game_value = secfile_lookup_bool_default(
              file, *pset->boolean.pvalue, "%s.set%d.gamestart", section, i);
          break;

        case SST_INT:
          pset->integer.game_value = secfile_lookup_int_default(
              file, *pset->integer.pvalue, "%s.set%d.gamestart", section, i);
          break;

        case SST_STRING:
          fc_strlcpy(pset->string.game_value,
                     secfile_lookup_str_default(file, pset->string.value,
                                                "%s.set%d.gamestart",
                                                section, i),
                     pset->string.value_size);
          break;

        case SST_ENUM:
          pset->enumerator.game_value = secfile_lookup_enum_default_data(
              file, read_enum_value(pset), false, setting_enum_secfile_str,
              pset, "%s.set%d.gamestart", section, i);
          break;

        case SST_BITWISE:
          pset->bitwise.game_value = secfile_lookup_enum_default_data(
              file, *pset->bitwise.pvalue, true, setting_bitwise_secfile_str,
              pset, "%s.set%d.gamestart", section, i);
          break;

        case SST_COUNT:
          fc_assert(pset->stype != SST_COUNT);
          break;
        }

        pset->setdef = SETDEF_CHANGED;
      }
    }
    settings_iterate_end;
  }

  /* Backwards compatibility for pre-2.4 savegames: citymindist=0 used to
   * mean take from ruleset min_dist_bw_cities, but that no longer exists.
   * This is here rather than in savegame2.c compat functions, as we need to
   * have loaded the relevant ruleset to know what to set it to (the ruleset
   * and any 'citymindist' setting it contains will have been loaded before
   * this function was called). */
  if (game.info.citymindist == 0) {
    game.info.citymindist = oldcitymindist;
  }

  settings_iterate(SSET_ALL, pset)
  {
    /* Have to do this at the end due to dependencies ('aifill' and
     * 'maxplayer'). */
    setting_action(pset);
  }
  settings_iterate_end;
}

/**
   Reset all settings to the values at game start.
 */
bool settings_game_reset()
{
  if (!game.server.settings_gamestart_valid) {
    log_debug("No saved settings from the game start available.");
    return false;
  }

  settings_iterate(SSET_ALL, pset) { setting_game_restore(pset); }
  settings_iterate_end;

  return true;
}

/**
   Initialize stuff related to this code module.
 */
void settings_init(bool act)
{
  settings_list_init();

  settings_iterate(SSET_ALL, pset)
  {
    setting_lock_set(pset, false);
    setting_set_to_default(pset);
    setting_game_set(pset, true);
    if (act) {
      setting_action(pset);
    }
  }
  settings_iterate_end;

  settings_list_update();
}

/**
   Reset all settings iff they are changeable.
 */
void settings_reset()
{
  settings_iterate(SSET_ALL, pset)
  {
    if (setting_is_changeable(pset, nullptr, nullptr, 0)) {
      setting_set_to_default(pset);
      setting_action(pset);
    }
  }
  settings_iterate_end;
}

/**
   Update stuff every turn that is related to this code module. Run this
   on turn end.
 */
void settings_turn()
{ // Nothing at the moment.
}

/**
   Deinitialize stuff related to this code module.
 */
void settings_free()
{
  settings_iterate(SSET_ALL, pset) { setting_game_free(pset); }
  settings_iterate_end;

  settings_list_free();
}

/**
   Returns the total number of settings.
 */
int settings_number() { return SETTINGS_NUM; }

/**
   Tell the client about just one server setting.  Call this after a setting
   is saved.
 */
void send_server_setting(struct conn_list *dest, const struct setting *pset)
{
  if (!dest) {
    dest = game.est_connections;
  }

#define PACKET_COMMON_INIT(packet, pset, pconn)                             \
  memset(&packet, 0, sizeof(packet));                                       \
  packet.id = setting_number(pset);                                         \
  packet.is_visible = setting_is_visible(pset, pconn);                      \
  packet.is_changeable = setting_is_changeable(pset, pconn, nullptr, 0);    \
  packet.initial_setting = game.info.is_new_game;                           \
  packet.setdef = setting_get_setdef(pset);

  switch (setting_type(pset)) {
  case SST_BOOL: {
    struct packet_server_setting_bool packet;

    conn_list_iterate(dest, pconn)
    {
      PACKET_COMMON_INIT(packet, pset, pconn);
      if (packet.is_visible) {
        packet.val = *pset->boolean.pvalue;
        packet.default_val = pset->boolean.default_value;
      }
      send_packet_server_setting_bool(pconn, &packet);
    }
    conn_list_iterate_end;
  } break;
  case SST_INT: {
    struct packet_server_setting_int packet;

    conn_list_iterate(dest, pconn)
    {
      PACKET_COMMON_INIT(packet, pset, pconn);
      if (packet.is_visible) {
        packet.val = *pset->integer.pvalue;
        packet.default_val = pset->integer.default_value;
        packet.min_val = pset->integer.min_value;
        packet.max_val = pset->integer.max_value;
      }
      send_packet_server_setting_int(pconn, &packet);
    }
    conn_list_iterate_end;
  } break;
  case SST_STRING: {
    struct packet_server_setting_str packet;

    conn_list_iterate(dest, pconn)
    {
      PACKET_COMMON_INIT(packet, pset, pconn);
      if (packet.is_visible) {
        sz_strlcpy(packet.val, pset->string.value);
        sz_strlcpy(packet.default_val, pset->string.default_value);
      }
      send_packet_server_setting_str(pconn, &packet);
    }
    conn_list_iterate_end;
  } break;
  case SST_ENUM: {
    struct packet_server_setting_enum packet;
    const struct sset_val_name *val_name;
    int i;

    conn_list_iterate(dest, pconn)
    {
      PACKET_COMMON_INIT(packet, pset, pconn);
      if (packet.is_visible) {
        packet.val = read_enum_value(pset);
        packet.default_val = pset->enumerator.default_value;
        for (i = 0; (val_name = pset->enumerator.name(i)); i++) {
          sz_strlcpy(packet.support_names[i], val_name->support);
          // Send untranslated string
          sz_strlcpy(packet.pretty_names[i], val_name->pretty);
        }
        packet.values_num = i;
        fc_assert(i <= ARRAY_SIZE(packet.support_names));
        fc_assert(i <= ARRAY_SIZE(packet.pretty_names));
      }
      send_packet_server_setting_enum(pconn, &packet);
    }
    conn_list_iterate_end;
  } break;
  case SST_BITWISE: {
    struct packet_server_setting_bitwise packet;
    const struct sset_val_name *val_name;
    int i;

    conn_list_iterate(dest, pconn)
    {
      PACKET_COMMON_INIT(packet, pset, pconn);
      if (packet.is_visible) {
        packet.val = *pset->bitwise.pvalue;
        packet.default_val = pset->bitwise.default_value;
        for (i = 0; (val_name = pset->bitwise.name(i)); i++) {
          sz_strlcpy(packet.support_names[i], val_name->support);
          // Send untranslated string
          sz_strlcpy(packet.pretty_names[i], val_name->pretty);
        }
        packet.bits_num = i;
        fc_assert(i <= ARRAY_SIZE(packet.support_names));
        fc_assert(i <= ARRAY_SIZE(packet.pretty_names));
      }
      send_packet_server_setting_bitwise(pconn, &packet);
    }
    conn_list_iterate_end;
  } break;

  case SST_COUNT:
    fc_assert(setting_type(pset) != SST_COUNT);
    break;
  }

#undef PACKET_INIT
}

/**
   Tell the client about all server settings.
 */
void send_server_settings(struct conn_list *dest)
{
  settings_iterate(SSET_ALL, pset) { send_server_setting(dest, pset); }
  settings_iterate_end;
}

/**
   Send the server settings that got a different visibility or changability
   after a connection access level change. Usually called when the access
   level of the user changes.
 */
void send_server_access_level_settings(struct conn_list *dest,
                                       enum cmdlevel old_level,
                                       enum cmdlevel new_level)
{
  enum cmdlevel min_level;
  enum cmdlevel max_level;

  if (old_level == new_level) {
    return;
  }

  if (old_level < new_level) {
    min_level = old_level;
    max_level = new_level;
  } else {
    min_level = new_level;
    max_level = old_level;
  }

  settings_iterate(SSET_ALL, pset)
  {
    if ((pset->access_level_read >= min_level
         && pset->access_level_read <= max_level)
        || (pset->access_level_write >= min_level
            && pset->access_level_write <= max_level)) {
      send_server_setting(dest, pset);
    }
  }
  settings_iterate_end;
}

/**
   Tell the client about all server settings.
 */
void send_server_setting_control(struct connection *pconn)
{
  struct packet_server_setting_control control;
  struct packet_server_setting_const setting;
  int i;

  control.settings_num = SETTINGS_NUM;

  // Fill in the category strings.
  fc_assert(SSET_NUM_CATEGORIES <= ARRAY_SIZE(control.category_names));
  control.categories_num = SSET_NUM_CATEGORIES;
  for (i = 0; i < SSET_NUM_CATEGORIES; i++) {
    // Send untranslated name
    sz_strlcpy(control.category_names[i],
               sset_category_name(sset_category(i)));
  }

  // Send off the control packet.
  send_packet_server_setting_control(pconn, &control);

  // Send the constant and common part of the settings.
  settings_iterate(SSET_ALL, pset)
  {
    setting.id = setting_number(pset);
    sz_strlcpy(setting.name, setting_name(pset));
    // Send untranslated strings to client
    sz_strlcpy(setting.short_help, setting_short_help(pset));
    sz_strlcpy(setting.extra_help, setting_extra_help(pset, true));
    setting.category = pset->scategory;

    send_packet_server_setting_const(pconn, &setting);
  }
  settings_iterate_end;
}

/**
   Initialise sorted settings.
 */
static void settings_list_init()
{
  struct setting *pset;
  int i;

  fc_assert_ret(setting_sorted.init == false);

  // Do it for all values of enum sset_level.
  for (i = 0; i < OLEVELS_NUM; i++) {
    setting_sorted.level[i] = setting_list_new();
  }

  for (i = 0; (pset = setting_by_number(i)); i++) {
    // Add the setting to the list of all settings.
    setting_list_append(setting_sorted.level[SSET_ALL], pset);

    switch (setting_level(pset)) {
    case SSET_NONE:
      // No setting should be in this level.
      fc_assert_msg(setting_level(pset) != SSET_NONE,
                    "No setting level defined for '%s'.",
                    setting_name(pset));
      break;
    case SSET_ALL:
      // Done above - list of all settings.
      break;
    case SSET_VITAL:
      setting_list_append(setting_sorted.level[SSET_VITAL], pset);
      break;
    case SSET_SITUATIONAL:
      setting_list_append(setting_sorted.level[SSET_SITUATIONAL], pset);
      break;
    case SSET_RARE:
      setting_list_append(setting_sorted.level[SSET_RARE], pset);
      break;
    case SSET_CHANGED:
    case SSET_LOCKED:
      // This is done in settings_list_update.
      break;
    case OLEVELS_NUM:
      // No setting should be in this level.
      fc_assert_msg(setting_level(pset) != OLEVELS_NUM,
                    "Invalid setting level for '%s' (%s).",
                    setting_name(pset),
                    sset_level_name(setting_level(pset)));
      break;
    }
  }

  // Sort the lists.
  for (i = 0; i < OLEVELS_NUM; i++) {
    setting_list_sort(setting_sorted.level[i], settings_list_cmp);
  }

  setting_sorted.init = true;
}

/**
   Update sorted settings (changed and locked values).
 */
void settings_list_update()
{
  struct setting *pset;
  int i;

  fc_assert_ret(setting_sorted.init == true);

  // Clear the lists for changed and locked values.
  setting_list_clear(setting_sorted.level[SSET_CHANGED]);
  setting_list_clear(setting_sorted.level[SSET_LOCKED]);

  // Refill them.
  for (i = 0; (pset = setting_by_number(i)); i++) {
    if (setting_non_default(pset)) {
      setting_list_append(setting_sorted.level[SSET_CHANGED], pset);
    }
    if (setting_locked(pset)) {
      setting_list_append(setting_sorted.level[SSET_LOCKED], pset);
    }
  }

  // Sort them.
  setting_list_sort(setting_sorted.level[SSET_CHANGED], settings_list_cmp);
  setting_list_sort(setting_sorted.level[SSET_LOCKED], settings_list_cmp);
}

/**
   Update sorted settings (changed and locked values).
 */
int settings_list_cmp(const struct setting *const *ppset1,
                      const struct setting *const *ppset2)
{
  const struct setting *pset1 = *ppset1;
  const struct setting *pset2 = *ppset2;

  return fc_strcasecmp(setting_name(pset1), setting_name(pset2));
}

/**
   Get a settings list of a certain level. Call settings_list_update() before
   if something was changed.
 */
struct setting_list *settings_list_get(enum sset_level level)
{
  fc_assert_ret_val(setting_sorted.init == true, nullptr);
  fc_assert_ret_val(setting_sorted.level[level] != nullptr, nullptr);
  fc_assert_ret_val(sset_level_is_valid(level), nullptr);

  return setting_sorted.level[level];
}

/**
   Free sorted settings.
 */
static void settings_list_free()
{
  int i;

  fc_assert_ret(setting_sorted.init == true);

  // Free the lists.
  for (i = 0; i < OLEVELS_NUM; i++) {
    setting_list_destroy(setting_sorted.level[i]);
  }

  setting_sorted.init = false;
}

/**
   Mark setting changed
 */
void setting_changed(struct setting *pset) { pset->setdef = SETDEF_CHANGED; }

/**
   Is the setting in changed state, or the default
 */
enum setting_default_level setting_get_setdef(const struct setting *pset)
{
  return pset->setdef;
}
