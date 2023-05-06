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

// utility
#include "log.h"
#include "shared.h"
#include "support.h"

// aicore
#include "cm.h"

// common
#include "achievements.h"
#include "actions.h"
#include "ai.h"
#include "city.h"
#include "disaster.h"
#include "effects.h"
#include "extras.h"
#include "government.h"
#include "idex.h"
#include "map.h"
#include "multipliers.h"
#include "nation.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "specialist.h"
#include "style.h"
#include "tech.h"
#include "terrain.h"
#include "traderoutes.h"
#include "unit.h"

#include "game.h"

struct civ_game game;
struct world wld;

bool am_i_server = false;

static void game_defaults(bool keep_ruleset_value);

/**
   Is program type server?
 */
bool is_server() { return am_i_server; }

/**
   Set program type to server.
 */
void i_am_server() { am_i_server = true; }

/**
   Set program type to client.
 */
void i_am_client() { am_i_server = false; }

/**
   Count the # of thousand citizen in a civilisation.
 */
int civ_population(const struct player *pplayer)
{
  int ppl = 0;
  city_list_iterate(pplayer->cities, pcity) ppl += city_population(pcity);
  city_list_iterate_end;
  return ppl;
}

/**
   Find city with given name from any player.
 */
struct city *game_city_by_name(const char *name)
{
  players_iterate(pplayer)
  {
    struct city *pcity = city_list_find_name(pplayer->cities, name);

    if (pcity) {
      return pcity;
    }
  }
  players_iterate_end;

  return nullptr;
}

/**
   Often used function to get a city pointer from a city ID.
   City may be any city in the game.  This now always uses fast idex
   method, instead of looking through all cities of all players.
 */
struct city *game_city_by_number(int id)
{
  return idex_lookup_city(&wld, id);
}

/**
   Find unit out of all units in game: now uses fast idex method,
   instead of looking through all units of all players.
 */
struct unit *game_unit_by_number(int id)
{
  return idex_lookup_unit(&wld, id);
}

/**
   In the server call wipe_unit(), and never this function directly.
 */
void game_remove_unit(struct world *gworld, struct unit *punit)
{
  struct city *pcity;

  /* It's possible that during city transfer homecity/unit owner
   * information is inconsistent, and client then tries to remove
   * now unseen unit so that homecity is not in the list of cities
   * of the player (seemingly) owning the unit.
   * Thus cannot use player_city_by_number() here, but have to
   * consider cities of all players. */
  pcity = game_city_by_number(punit->homecity);
  if (pcity) {
    unit_list_remove(pcity->units_supported, punit);

    log_debug("game_remove_unit()"
              " at (%d,%d) unit %d, %s %s home (%d,%d) city %d, %s %s",
              TILE_XY(unit_tile(punit)), punit->id,
              nation_rule_name(nation_of_unit(punit)), unit_rule_name(punit),
              TILE_XY(pcity->tile), punit->homecity,
              nation_rule_name(nation_of_city(pcity)), city_name_get(pcity));
  } else if (IDENTITY_NUMBER_ZERO == punit->homecity) {
    log_debug("game_remove_unit() at (%d,%d) unit %d, %s %s home %d",
              TILE_XY(unit_tile(punit)), punit->id,
              nation_rule_name(nation_of_unit(punit)), unit_rule_name(punit),
              punit->homecity);
  } else {
    qCritical("game_remove_unit() at (%d,%d) unit %d, %s %s home %d invalid",
              TILE_XY(unit_tile(punit)), punit->id,
              nation_rule_name(nation_of_unit(punit)), unit_rule_name(punit),
              punit->homecity);
  }

  unit_list_remove(unit_tile(punit)->units, punit);
  unit_list_remove(unit_owner(punit)->units, punit);

  idex_unregister_unit(gworld, punit);

  if (game.callbacks.unit_deallocate) {
    (game.callbacks.unit_deallocate)(punit->id);
  }
  unit_virtual_destroy(punit);
}

/**
   Remove city from game.
 */
void game_remove_city(struct world *gworld, struct city *pcity)
{
  struct tile *pcenter = city_tile(pcity);
  struct player *powner = city_owner(pcity);

  if (nullptr != powner) {
    // always unlink before clearing data
    city_list_remove(powner->cities, pcity);
  }

  if (nullptr == pcenter) {
    log_debug("game_remove_city() virtual city %d, %s", pcity->id,
              city_name_get(pcity));
  } else {
    log_debug("game_remove_city() at (%d,%d) city %d, %s %s",
              TILE_XY(pcenter), pcity->id,
              nation_rule_name(nation_of_player(powner)),
              city_name_get(pcity));

    city_tile_iterate(city_map_radius_sq_get(pcity), pcenter, ptile)
    {
      if (tile_worked(ptile) == pcity) {
        tile_set_worked(ptile, nullptr);
      }
    }
    city_tile_iterate_end;
  }

  idex_unregister_city(gworld, pcity);
  destroy_city_virtual(pcity);
}

/**
   Set default game values.
 */
static void game_defaults(bool keep_ruleset_value)
{
  int i;

  // The control packet.
  game.control.government_count = 0;
  game.control.nation_count = 0;
  game.control.num_base_types = 0;
  game.control.num_road_types = 0;
  game.control.num_resource_types = 0;
  game.control.num_impr_types = 0;
  game.control.num_specialist_types = 0;
  game.control.num_tech_types = 0;
  game.control.num_unit_classes = 0;
  game.control.num_unit_types = 0;
  game.control.num_disaster_types = 0;
  game.control.num_achievement_types = 0;
  game.control.num_styles = 0;
  game.control.num_music_styles = 0;
  game.control.preferred_tileset[0] = '\0';
  game.control.preferred_soundset[0] = '\0';
  game.control.preferred_musicset[0] = '\0';
  game.control.styles_count = 0;
  game.control.terrain_count = 0;

  game.ruleset_summary = nullptr;
  game.ruleset_description = nullptr;
  game.ruleset_capabilities = nullptr;

  // The info packet.
  game.info.aifill = GAME_DEFAULT_AIFILL;
  game.info.airlifting_style = GAME_DEFAULT_AIRLIFTINGSTYLE;
  game.info.angrycitizen = GAME_DEFAULT_ANGRYCITIZEN;
  game.info.borders = GAME_DEFAULT_BORDERS;
  game.calendar.calendar_skip_0 = false;
  game.info.caravan_bonus_style = GAME_DEFAULT_CARAVAN_BONUS_STYLE;
  game.info.celebratesize = GAME_DEFAULT_CELEBRATESIZE;
  game.info.citymindist = GAME_DEFAULT_CITYMINDIST;
  game.info.cooling = 0;
  game.info.coolinglevel = 0; // set later
  game.info.diplomacy = GAME_DEFAULT_DIPLOMACY;
  game.info.fogofwar = GAME_DEFAULT_FOGOFWAR;
  game.info.foodbox = GAME_DEFAULT_FOODBOX;
  game.info.fulltradesize = GAME_DEFAULT_FULLTRADESIZE;
  game.info.global_advance_count = 0;
  for (i = 0; i < A_LAST; i++) {
    // game.num_tech_types = 0 here
    game.info.global_advances[i] = false;
  }
  for (i = 0; i < B_LAST; i++) {
    // game.num_impr_types = 0 here
    game.info.great_wonder_owners[i] = WONDER_NOT_OWNED;
  }
  game.info.globalwarming = 0;
  game.info.global_warming = GAME_DEFAULT_GLOBAL_WARMING;
  game.info.gold = GAME_DEFAULT_GOLD;
  game.info._unused = 0;
  game.info.revolentype = GAME_DEFAULT_REVOLENTYPE;
  game.info.default_government_id = G_LAST;
  game.info.government_during_revolution_id = G_LAST;
  game.info.happyborders = GAME_DEFAULT_HAPPYBORDERS;
  game.info.heating = 0;
  game.info.is_edit_mode = false;
  game.info.is_new_game = true;
  game.info.killstack = GAME_DEFAULT_KILLSTACK;
  game.info.killcitizen = GAME_DEFAULT_KILLCITIZEN;
  game.calendar.negative_year_label[0] = '\0';
  game.info.notradesize = GAME_DEFAULT_NOTRADESIZE;
  game.info.nuclearwinter = 0;
  game.info.nuclear_winter = GAME_DEFAULT_NUCLEAR_WINTER;
  game.calendar.positive_year_label[0] = '\0';
  game.info.rapturedelay = GAME_DEFAULT_RAPTUREDELAY;
  game.info.disasters = GAME_DEFAULT_DISASTERS;
  game.info.restrictinfra = GAME_DEFAULT_RESTRICTINFRA;
  game.info.sciencebox = GAME_DEFAULT_SCIENCEBOX;
  game.info.shieldbox = GAME_DEFAULT_SHIELDBOX;
  game.info.skill_level = GAME_DEFAULT_SKILL_LEVEL;
  game.info.victory_conditions = GAME_DEFAULT_VICTORY_CONDITIONS;
  game.info.team_pooled_research = GAME_DEFAULT_TEAM_POOLED_RESEARCH;
  game.info.tech = GAME_DEFAULT_TECHLEVEL;
  game.info.timeout = GAME_DEFAULT_TIMEOUT;
  game.info.trademindist = GAME_DEFAULT_TRADEMINDIST;
  game.info.trade_revenue_style = GAME_DEFAULT_TRADE_REVENUE_STYLE;
  game.info.trading_city = GAME_DEFAULT_TRADING_CITY;
  game.info.trading_gold = GAME_DEFAULT_TRADING_GOLD;
  game.info.trading_tech = GAME_DEFAULT_TRADING_TECH;
  game.info.turn = 0;
  game.info.warminglevel = 0; // set later
  game.info.year_0_hack = false;
  game.info.year = GAME_START_YEAR;
  game.info.killunhomed = GAME_DEFAULT_KILLUNHOMED;

  // The scenario packets.
  game.scenario.is_scenario = false;
  game.scenario.name[0] = '\0';
  game.scenario.authors[0] = '\0';
  game.scenario.players = true;
  game.scenario.startpos_nations = false;
  game.scenario.handmade = false;
  game.scenario.prevent_new_cities = false;
  game.scenario.lake_flooding = true;
  game.scenario.have_resources = true;
  game.scenario.ruleset_locked = true;
  game.scenario.save_random = false;
  game.scenario.allow_ai_type_fallback = true;

  game.scenario_desc.description[0] = '\0';

  // Veteran system.
  game.veteran = nullptr;

  // player colors
  game.plr_bg_color = nullptr;

  if (is_server()) {
    /* All settings only used by the server (./server/ and ./ai/ */
    sz_strlcpy(game.server.allow_take, GAME_DEFAULT_ALLOW_TAKE);
    game.server.allowed_city_names = GAME_DEFAULT_ALLOWED_CITY_NAMES;
    game.server.aqueductloss = GAME_DEFAULT_AQUEDUCTLOSS;
    game.server.auto_ai_toggle = GAME_DEFAULT_AUTO_AI_TOGGLE;
    game.server.autoattack = GAME_DEFAULT_AUTOATTACK;
    game.server.barbarianrate = GAME_DEFAULT_BARBARIANRATE;
    game.server.civilwarsize = GAME_DEFAULT_CIVILWARSIZE;
    game.server.connectmsg[0] = '\0';
    game.server.conquercost = GAME_DEFAULT_CONQUERCOST;
    game.server.contactturns = GAME_DEFAULT_CONTACTTURNS;
    for (i = 0; i < DEBUG_LAST; i++) {
      game.server.debug[i] = false;
    }
    sz_strlcpy(game.server.demography, GAME_DEFAULT_DEMOGRAPHY);
    game.server.diplchance = GAME_DEFAULT_DIPLCHANCE;
    game.server.diplbulbcost = GAME_DEFAULT_DIPLBULBCOST;
    game.server.diplgoldcost = GAME_DEFAULT_DIPLGOLDCOST;
    game.server.incite_gold_loss_chance =
        GAME_DEFAULT_INCITE_GOLD_LOSS_CHANCE;
    game.server.incite_gold_capt_chance =
        GAME_DEFAULT_INCITE_GOLD_CAPT_CHANCE;
    game.server.dispersion = GAME_DEFAULT_DISPERSION;
    game.server.endspaceship = GAME_DEFAULT_END_SPACESHIP;
    game.server.end_turn = GAME_DEFAULT_END_TURN;
    game.server.event_cache.chat = GAME_DEFAULT_EVENT_CACHE_CHAT;
    game.server.event_cache.info = GAME_DEFAULT_EVENT_CACHE_INFO;
    game.server.event_cache.max_size = GAME_DEFAULT_EVENT_CACHE_MAX_SIZE;
    game.server.event_cache.turns = GAME_DEFAULT_EVENT_CACHE_TURNS;
    game.server.foggedborders = GAME_DEFAULT_FOGGEDBORDERS;
    game.server.fogofwar_old = game.info.fogofwar;
    game.server.last_updated_year = false;
    game.server.freecost = GAME_DEFAULT_FREECOST;
    game.server.global_warming_percent = GAME_DEFAULT_GLOBAL_WARMING_PERCENT;
    game.server.homecaughtunits = GAME_DEFAULT_HOMECAUGHTUNITS;
    game.server.kick_time = GAME_DEFAULT_KICK_TIME;
    game.server.maxconnectionsperhost = GAME_DEFAULT_MAXCONNECTIONSPERHOST;
    game.server.last_ping = 0;
    game.server.max_players = GAME_DEFAULT_MAX_PLAYERS;
    game.server.meta_info.user_message[0] = '\0';
    /* Do not clear meta_info.type here as it's already set to correct value
     */
    game.server.mgr_distance = GAME_DEFAULT_MGR_DISTANCE;
    game.server.mgr_foodneeded = GAME_DEFAULT_MGR_FOODNEEDED;
    game.server.mgr_nationchance = GAME_DEFAULT_MGR_NATIONCHANCE;
    game.server.mgr_turninterval = GAME_DEFAULT_MGR_TURNINTERVAL;
    game.server.mgr_worldchance = GAME_DEFAULT_MGR_WORLDCHANCE;
    game.server.multiresearch = GAME_DEFAULT_MULTIRESEARCH;
    game.server.migration = GAME_DEFAULT_MIGRATION;
    game.server.trait_dist = GAME_DEFAULT_TRAIT_DIST_MODE;
    game.server.min_players = GAME_DEFAULT_MIN_PLAYERS;
    game.server.natural_city_names = GAME_DEFAULT_NATURALCITYNAMES;
    game.server.nuclear_winter_percent = GAME_DEFAULT_NUCLEAR_WINTER_PERCENT;
    game.server.plrcolormode = GAME_DEFAULT_PLRCOLORMODE;
    game.server.netwait = GAME_DEFAULT_NETWAIT;
    game.server.occupychance = GAME_DEFAULT_OCCUPYCHANCE;
    game.server.onsetbarbarian = GAME_DEFAULT_ONSETBARBARIAN;
    game.server.additional_phase_seconds = 0;
    game.server.phase_mode_stored = GAME_DEFAULT_PHASE_MODE;
    game.server.pingtime = GAME_DEFAULT_PINGTIME;
    game.server.pingtimeout = GAME_DEFAULT_PINGTIMEOUT;
    game.server.razechance = GAME_DEFAULT_RAZECHANCE;
    game.server.revealmap = GAME_DEFAULT_REVEALMAP;
    game.server.revolution_length = GAME_DEFAULT_REVOLUTION_LENGTH;
    if (!keep_ruleset_value) {
      sz_strlcpy(game.server.rulesetdir, GAME_DEFAULT_RULESETDIR);
    }
    game.server.save_compress_type = GAME_DEFAULT_COMPRESS_TYPE;
    sz_strlcpy(game.server.save_name, GAME_DEFAULT_SAVE_NAME);
    game.server.save_nturns = GAME_DEFAULT_SAVETURNS;
    game.server.save_options.save_known = true;
    game.server.save_options.save_private_map = true;
    game.server.save_options.save_starts = true;
    game.server.savepalace = GAME_DEFAULT_SAVEPALACE;
    game.server.scorelog = GAME_DEFAULT_SCORELOG;
    game.server.scoreloglevel = GAME_DEFAULT_SCORELOGLEVEL;
    game.server.scoreturn = GAME_DEFAULT_SCORETURN - 1;
    game.server.seed = GAME_DEFAULT_SEED;
    sz_strlcpy(game.server.start_units, GAME_DEFAULT_START_UNITS);
    game.server.spaceship_travel_time = GAME_DEFAULT_SPACESHIP_TRAVEL_TIME;
    game.server.start_year = GAME_START_YEAR;
    game.server.techlost_donor = GAME_DEFAULT_TECHLOST_DONOR;
    game.server.techlost_recv = GAME_DEFAULT_TECHLOST_RECV;
    game.server.techpenalty = GAME_DEFAULT_TECHPENALTY;
    game.server.timeoutaddenemymove = GAME_DEFAULT_TIMEOUTADDEMOVE;
    game.server.timeoutcounter = GAME_DEFAULT_TIMEOUTCOUNTER;
    game.server.timeoutinc = GAME_DEFAULT_TIMEOUTINC;
    game.server.timeoutincmult = GAME_DEFAULT_TIMEOUTINCMULT;
    game.server.timeoutint = GAME_DEFAULT_TIMEOUTINT;
    game.server.timeoutintinc = GAME_DEFAULT_TIMEOUTINTINC;
    game.server.turnblock = GAME_DEFAULT_TURNBLOCK;
    game.server.unitwaittime = GAME_DEFAULT_UNITWAITTIME;
    game.server.unitwaittime_extended = GAME_DEFAULT_UNITWAITTIME_EXTENDED;
    game.server.plr_colors = nullptr;
  } else {
    // Client side takes care of itself in client_main()
  }
}

/**
   Initialise all game settings.

   The variables are listed in alphabetical order.
 */
void game_init(bool keep_ruleset_value)
{
  game_defaults(keep_ruleset_value);
  player_slots_init();
  map_init(&wld.map, is_server());
  team_slots_init();
  game_ruleset_init();
  idex_init(&wld);
  cm_init();
  researches_init();
  universal_found_functions_init();
}

/**
   Initialize map-specific parts of the game structure.  Maybe these should
   be moved into the map structure?
 */
void game_map_init()
{
  /* FIXME: it's not clear where these values should be initialized.  It
   * can't be done in game_init because the map isn't created yet.  Maybe it
   * should be done in the mapgen code or in the maphand code.  It should
   * surely be called when the map is generated. */
  game.info.warminglevel = (map_num_tiles() + 499) / 500;
  game.info.coolinglevel = (map_num_tiles() + 499) / 500;
}

/**
   Frees all memory of the game.
 */
void game_free()
{
  player_slots_free();
  main_map_free();
  free_city_map_index();
  idex_free(&wld);
  team_slots_free();
  game_ruleset_free();
  researches_free();
  cm_free();
}

/**
   Do all changes to change view, and not full
   game_free()/game_init().
 */
void game_reset()
{
  if (is_server()) {
    game_free();
    game_init(false);
  } else {
    // Reset the players infos.
    players_iterate(pplayer) { player_clear(pplayer, false); }
    players_iterate_end;

    main_map_free();
    free_city_map_index();
    idex_free(&wld);

    map_init(&wld.map, false);
    idex_init(&wld);
    researches_init();
  }
}

/**
   Initialize the objects which will read from a ruleset.
 */
void game_ruleset_init()
{
  nation_sets_groups_init();
  ruleset_cache_init();
  disaster_types_init();
  achievements_init();
  actions_init();
  trade_route_types_init();
  terrains_init();
  extras_init();
  goods_init();
  improvements_init();
  techs_init();
  unit_classes_init();
  unit_types_init();
  specialists_init();
  user_unit_class_flags_init();
  user_unit_type_flags_init();
  user_terrain_flags_init();
  user_extra_flags_init();
  tech_classes_init();
  user_tech_flags_init();
  multipliers_init();
  clause_infos_init();

  if (is_server()) {
    game.server.luadata = nullptr;
    game.server.ruledit.nationlist = nullptr;
    game.server.ruledit.embedded_nations = nullptr;
    game.server.ruledit.embedded_nations_count = 0;
    game.server.ruledit.allowed_govs = nullptr;
    game.server.ruledit.allowed_terrains = nullptr;
    game.server.ruledit.allowed_styles = nullptr;
    game.server.ruledit.nc_agovs = nullptr;
    game.server.ruledit.nc_aterrs = nullptr;
    game.server.ruledit.nc_astyles = nullptr;
    game.server.ruledit.ag_count = 0;
    game.server.ruledit.at_count = 0;
    game.server.ruledit.as_count = 0;
  }
}

/**
   Frees all memory which in objects which are read from a ruleset.
 */
void game_ruleset_free()
{
  int i;

  CALL_FUNC_EACH_AI(units_ruleset_close);

  /* Clear main structures which can points to the ruleset dependent
   * structures. */
  players_iterate(pplayer) { player_ruleset_close(pplayer); }
  players_iterate_end;
  game.government_during_revolution = nullptr;

  specialists_free();
  unit_classes_free();
  techs_free();
  governments_free();
  nations_free();
  unit_types_free();
  unit_type_flags_free();
  unit_class_flags_free();
  role_unit_precalcs_free();
  improvements_free();
  goods_free();
  extras_free();
  music_styles_free();
  city_styles_free();
  styles_free();
  actions_free();
  achievements_free();
  disaster_types_free();
  terrains_free();
  user_tech_flags_free();
  extra_flags_free();
  user_terrain_flags_free();
  ruleset_cache_free();
  nation_sets_groups_free();
  multipliers_free();
  clause_infos_free();

  // Destroy the default veteran system.
  veteran_system_destroy(game.veteran);
  game.veteran = nullptr;

  // Player colors.
  if (game.plr_bg_color != nullptr) {
    rgbcolor_destroy(game.plr_bg_color);
    game.plr_bg_color = nullptr;
  }

  if (is_server()) {
    if (game.server.luadata != nullptr) {
      secfile_destroy(game.server.luadata);
    }
    delete[] game.server.ruledit.description_file;
    game.server.ruledit.description_file = nullptr;
    delete[] game.server.ruledit.nationlist;
    game.server.ruledit.nationlist = nullptr;
    if (game.server.ruledit.embedded_nations != nullptr) {
      for (i = 0; i < game.server.ruledit.embedded_nations_count; i++) {
        delete[] game.server.ruledit.embedded_nations[i];
      }
      delete[] game.server.ruledit.embedded_nations;
      game.server.ruledit.embedded_nations = nullptr;
      game.server.ruledit.embedded_nations_count = 0;
      if (game.server.ruledit.allowed_govs != nullptr) {
        for (i = 0; i < game.server.ruledit.ag_count; i++) {
          delete[] game.server.ruledit.nc_agovs[i];
        }
        delete[] game.server.ruledit.allowed_govs;
        game.server.ruledit.allowed_govs = nullptr;
        game.server.ruledit.nc_agovs = nullptr;
      }
      if (game.server.ruledit.allowed_terrains != nullptr) {
        for (i = 0; i < game.server.ruledit.at_count; i++) {
          delete[] game.server.ruledit.nc_aterrs[i];
        }
        delete[] game.server.ruledit.allowed_terrains;
        game.server.ruledit.allowed_terrains = nullptr;
        game.server.ruledit.nc_aterrs = nullptr;
      }
      if (game.server.ruledit.allowed_styles != nullptr) {
        for (i = 0; i < game.server.ruledit.as_count; i++) {
          delete[] game.server.ruledit.nc_astyles[i];
        }
        delete[] game.server.ruledit.allowed_styles;
        game.server.ruledit.allowed_styles = nullptr;
        game.server.ruledit.nc_astyles = nullptr;
      }
    }
  }

  for (i = 0; i < MAX_CALENDAR_FRAGMENTS; i++) {
    game.calendar.calendar_fragment_name[i][0] = '\0';
  }

  delete[] game.ruleset_summary;
  delete[] game.ruleset_description;
  delete[] game.ruleset_capabilities;
  game.ruleset_summary = nullptr;
  game.ruleset_description = nullptr;
  game.ruleset_capabilities = nullptr;
}

/**
   Initialize wonder information.
 */
void initialize_globals()
{
  players_iterate(pplayer)
  {
    city_list_iterate(pplayer->cities, pcity)
    {
      city_built_iterate(pcity, pimprove)
      {
        if (is_wonder(pimprove)) {
          if (is_great_wonder(pimprove)) {
            game.info.great_wonder_owners[improvement_index(pimprove)] =
                player_number(pplayer);
          }
          if (get_building_bonus(pcity, pimprove, EFT_WONDER_VISIBLE) > 0) {
            pplayer->wonders[improvement_index(pimprove)] = pcity->id;
          }
        }
      }
      city_built_iterate_end;
    }
    city_list_iterate_end;
  }
  players_iterate_end;
}

/**
   Return TRUE if it is this player's phase.
   NB: The meaning of the 'phase' argument must match its use in the
   function begin_turn() in server/srv_main.c.
   NB: The phase mode PMT_TEAMS_ALTERNATE assumes that every player is
   on a team, i.e. that pplayer->team is never nullptr.
 */
bool is_player_phase(const struct player *pplayer, int phase)
{
  switch (game.info.phase_mode) {
  case PMT_CONCURRENT:
    return true;
    break;
  case PMT_PLAYERS_ALTERNATE:
    return player_number(pplayer) == phase;
    break;
  case PMT_TEAMS_ALTERNATE:
    fc_assert_ret_val(nullptr != pplayer->team, false);
    return team_number(pplayer->team) == phase;
    break;
  default:
    break;
  }

  fc_assert_msg(false, "Unrecognized phase mode %d in is_player_phase().",
                phase);
  return true;
}

/**
   Return a prettily formatted string containing the population text.  The
   population is passed in as the number of citizens, in unit
   (tens/hundreds/thousands...) defined in cities.ruleset.
 */
const char *population_to_text(int thousand_citizen)
{
  /* big_int_to_text can't handle negative values, and in any case we'd
   * better not have a negative population. */
  fc_assert_ret_val(thousand_citizen >= 0, nullptr);
  return big_int_to_text(thousand_citizen, game.info.pop_report_zeroes - 1);
}

/**
   Return a string containing the save year.
 */
static char *year_suffix()
{
  static char buf[MAX_LEN_NAME];
  const char *suffix;
  char safe_year_suffix[MAX_LEN_NAME];
  const char *max = safe_year_suffix + MAX_LEN_NAME - 1;
  char *c = safe_year_suffix;

  if (game.info.year < 0) {
    suffix = game.calendar.negative_year_label;
  } else {
    suffix = game.calendar.positive_year_label;
  }

  // Remove all non alphanumeric characters from the year suffix.
  for (; '\0' != *suffix && c < max; suffix++) {
    if (QChar::isLetterOrNumber(*suffix)) {
      *c++ = *suffix;
    }
  }
  *c = '\0';

  fc_snprintf(buf, sizeof(buf), "%s", safe_year_suffix);

  return buf;
}

/**
   Generate a default save file name and place it in the provided buffer.
   Within the name the following custom formats are allowed:

     %R = <reason>
     %S = <suffix>
     %T = <game.info.turn>
     %Y = <game.info.year>

   Examples:
     'freeciv-T%04T-Y%+04Y-%R' => 'freeciv-T0099-Y-0050-manual'
                               => 'freeciv-T0100-Y00001-auto'

   Returns the number of characters written, or the number of characters
   that would have been written if truncation occurs.

   NB: If you change the format definition, be sure to update the above
       function comment and the help text for the 'savename' setting.
 */
int generate_save_name(const char *format, char *buf, int buflen,
                       const char *reason)
{
  struct cf_sequence sequences[5] = {
      cf_str_seq('R', (reason == nullptr) ? "auto" : reason),
      cf_str_seq('S', year_suffix()),
      {cf_type(0)},
      {cf_type(0)},
      cf_end()};

  cf_int_seq('T', game.info.turn, &sequences[2]);
  cf_int_seq('Y', game.info.year, &sequences[3]);

  fc_vsnprintcf(buf, buflen, format, sequences, -1);

  if (0 == strcmp(format, buf)) {
    /* Use the default savename if 'format' does not contain
     * printf information. */
    char savename[512];

    fc_snprintf(savename, sizeof(savename), "%s-T%%04T-Y%%05Y-%%R", format);
    fc_vsnprintcf(buf, buflen, savename, sequences, -1);
  }

  log_debug("save name generated from '%s': %s", format, buf);

  return qstrlen(buf);
}

/**
   Initialize user flag.
 */
void user_flag_init(struct user_flag *flag)
{
  flag->name = nullptr;
  flag->helptxt = nullptr;
}

/**
   Free user flag.
 */
void user_flag_free(struct user_flag *flag)
{
  delete[] flag->name;
  delete[] flag->helptxt;
  flag->name = nullptr;
  flag->helptxt = nullptr;
}

/**
   Return timeout value for the current turn.
 */
int current_turn_timeout()
{
  if (game.info.turn == 1 && game.info.first_timeout != -1) {
    return game.info.first_timeout;
  } else {
    return game.info.timeout;
  }
}
