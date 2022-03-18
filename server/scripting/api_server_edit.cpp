/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Sol
#include "sol/sol.hpp"

// utility
#include "fcintl.h"
#include "rand.h"

// common
#include "map.h"
#include "movement.h"
#include "research.h"
#include "unittype.h"

/* common/scriptcore */
#include "api_game_find.h"
#include "luascript.h"

// server
#include "aiiface.h"
#include "barbarian.h"
#include "citytools.h"
#include "console.h" // enum rfc_status
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "srv_main.h" // game_was_started()
#include "stdinhand.h"
#include "techtools.h"
#include "unittools.h"

/* server/scripting */
#include "script_server.h"

/* server/generator */
#include "mapgen_utils.h"

#include "api_server_edit.h"

namespace {

/**
   Place partisans for a player around a tile (normally around a city).
 */
void edit_place_partisans(sol::this_state s, tile *ptile, player *pplayer,
                          int count, int sq_radius)
{
  LUASCRIPT_CHECK_ARG(s, 0 <= sq_radius, 5, "radius must be positive");
  LUASCRIPT_CHECK(s, 0 < num_role_units(L_PARTISAN),
                  "no partisans in ruleset");

  return place_partisans(ptile, pplayer, count, sq_radius);
}

/**
   Create a new unit.
 */
unit *edit_create_unit_full(sol::this_state s, player *pplayer, tile *ptile,
                            unit_type *ptype, int veteran_level,
                            city *homecity, int moves_left, int hp_left,
                            unit *ptransport)
{
  struct fc_lua *fcl;
  struct city *pcity;

  fcl = luascript_get_fcl(s);

  if (ptype == nullptr || ptype < unit_type_array_first()
      || ptype > unit_type_array_last()) {
    return nullptr;
  }

  if (ptransport) {
    // Extensive check to see if transport and unit are compatible
    int ret;
    struct unit *pvirt =
        unit_virtual_create(pplayer, nullptr, ptype, veteran_level);
    unit_tile_set(pvirt, ptile);
    pvirt->homecity = homecity ? homecity->id : 0;
    ret = can_unit_load(pvirt, ptransport);
    unit_virtual_destroy(pvirt);
    if (!ret) {
      luascript_log(fcl, LOG_ERROR,
                    "create_unit_full: '%s' cannot transport "
                    "'%s' here",
                    utype_rule_name(unit_type_get(ptransport)),
                    utype_rule_name(ptype));
      return nullptr;
    }
  } else if (!can_exist_at_tile(&(wld.map), ptype, ptile)) {
    luascript_log(fcl, LOG_ERROR,
                  "create_unit_full: '%s' cannot exist at "
                  "tile",
                  utype_rule_name(ptype));
    return nullptr;
  }

  if (is_non_allied_unit_tile(ptile, pplayer)) {
    luascript_log(fcl, LOG_ERROR,
                  "create_unit_full: tile is occupied by "
                  "enemy unit");
    return nullptr;
  }

  pcity = tile_city(ptile);
  if (pcity != nullptr && !pplayers_allied(pplayer, city_owner(pcity))) {
    luascript_log(fcl, LOG_ERROR,
                  "create_unit_full: tile is occupied by "
                  "enemy city");
    return nullptr;
  }

  return create_unit_full(pplayer, ptile, ptype, veteran_level,
                          homecity ? homecity->id : 0, moves_left, hp_left,
                          ptransport);
}

/**
   Create a new unit.
 */
unit *edit_create_unit(sol::this_state s, player *pplayer, tile *ptile,
                       unit_type *ptype, int veteran_level, city *homecity,
                       int moves_left)
{
  return edit_create_unit_full(s, pplayer, ptile, ptype, veteran_level,
                               homecity, moves_left, -1, nullptr);
}

/**
   Teleport unit to destination tile
 */
bool edit_unit_teleport(unit *punit, tile *dest)
{
  bool alive;
  struct city *pcity;

  // Teleport first so destination is revealed even if unit dies
  alive = unit_move(
      punit, dest, 0,
      /* Auto embark kept for backward compatibility. I have
       * no objection if you see the old behavior as a bug and
       * remove auto embarking completely or for transports
       * the unit can't legally board. -- Sveinung */
      nullptr, true,
      /* Backwards compatibility for old scripts in rulesets
       * and (scenario) savegames. I have no objection if you
       * see the old behavior as a bug and remove auto
       * conquering completely or for cities the unit can't
       * legally conquer. -- Sveinung */
      ((pcity = tile_city(dest))
       && (unit_owner(punit)->ai_common.barbarian_type != ANIMAL_BARBARIAN)
       && uclass_has_flag(unit_class_get(punit), UCF_CAN_OCCUPY_CITY)
       && !unit_has_type_flag(punit, UTYF_CIVILIAN)
       && pplayers_at_war(unit_owner(punit), city_owner(pcity))));
  if (alive) {
    struct player *owner = unit_owner(punit);

    if (!can_unit_exist_at_tile(&(wld.map), punit, dest)) {
      wipe_unit(punit, ULR_NONNATIVE_TERR, nullptr);
      return false;
    }
    if (is_non_allied_unit_tile(dest, owner)
        || (pcity && !pplayers_allied(city_owner(pcity), owner))) {
      wipe_unit(punit, ULR_STACK_CONFLICT, nullptr);
      return false;
    }
  }

  return alive;
}

/**
   Change unit orientation
 */
void edit_unit_turn(unit *punit, Direction dir)
{
  if (direction8_is_valid(dir)) {
    punit->facing = dir;

    send_unit_info(nullptr, punit);
  } else {
    qCritical("Illegal direction %d for unit from lua script", dir);
  }
}

/**
   Kill the unit.
 */
void edit_unit_kill(sol::this_state s, unit *punit, const char *reason,
                    player *killer)
{
  enum unit_loss_reason loss_reason;

  loss_reason = unit_loss_reason_by_name(reason, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, unit_loss_reason_is_valid(loss_reason), 3,
                      "Invalid unit loss reason");

  wipe_unit(punit, loss_reason, killer);
}

/**
   Change terrain on tile
 */
bool edit_change_terrain(tile *ptile, terrain *pterr)
{
  struct terrain *old_terrain;

  old_terrain = tile_terrain(ptile);

  if (old_terrain == pterr
      || (terrain_has_flag(pterr, TER_NO_CITIES)
          && tile_city(ptile) != nullptr)) {
    return false;
  }

  tile_change_terrain(ptile, pterr);
  fix_tile_on_terrain_change(ptile, old_terrain, false);
  if (need_to_reassign_continents(old_terrain, pterr)) {
    assign_continent_numbers();
    send_all_known_tiles(nullptr);
  }

  update_tile_knowledge(ptile);

  return true;
}

/**
   Create a new city.
 */
void edit_create_city(player *pplayer, tile *ptile, const char *name)
{
  if (!name || name[0] == '\0') {
    name = city_name_suggestion(pplayer, ptile);
  }

  // TODO: Allow initial citizen to be of nationality other than owner
  create_city(pplayer, ptile, name, pplayer);
}

/**
   Create a new player.
 */
player *edit_create_player(sol::this_state s, const char *username,
                           nation_type *pnation, const char *ai)
{
  struct player *pplayer = nullptr;
  char buf[128] = "";
  struct fc_lua *fcl;

  if (!ai) {
    ai = default_ai_type_name();
  }

  fcl = luascript_get_fcl(s);

  if (game_was_started()) {
    create_command_newcomer(username, ai, false, pnation, &pplayer, buf,
                            sizeof(buf));
  } else {
    create_command_pregame(username, ai, false, &pplayer, buf, sizeof(buf));
  }

  if (strlen(buf) > 0) {
    luascript_log(fcl, LOG_NORMAL, "%s", buf);
  }

  return pplayer;
}

/**
   Change pplayer's gold by amount.
 */
void edit_change_gold(player *pplayer, int amount)
{
  pplayer->economic.gold = MAX(0, pplayer->economic.gold + amount);
}

/**
   Give pplayer technology ptech. Quietly returns nullptr if
   player already has this tech; otherwise returns the tech granted.
   Use nullptr for ptech to grant a random tech.
   sends script signal "tech_researched" with the given reason
 */
advance *edit_give_technology(sol::this_state s, player *pplayer,
                              advance *ptech, int cost, bool notify,
                              const char *reason)
{
  struct research *presearch;
  int id;
  advance *result;

  LUASCRIPT_CHECK_ARG(s, cost >= -3, 4, "Unknown give_tech() cost value",
                      nullptr);

  presearch = research_get(pplayer);
  if (ptech) {
    id = advance_number(ptech);
  } else {
    id = pick_free_tech(presearch);
  }

  if (is_future_tech(id)
      || research_invention_state(presearch, id) != TECH_KNOWN) {
    if (cost < 0) {
      if (cost == -1) {
        cost = game.server.freecost;
      } else if (cost == -2) {
        cost = game.server.conquercost;
      } else if (cost == -3) {
        cost = game.server.diplbulbcost;
      }
    }
    research_apply_penalty(presearch, id, cost);
    found_new_tech(presearch, id, false, true);
    result = advance_by_number(id);
    script_tech_learned(presearch, pplayer, result, reason);

    if (notify && result != nullptr) {
      QString adv_name = research_advance_name_translation(presearch, id);
      char research_name[MAX_LEN_NAME * 2];

      research_pretty_name(presearch, research_name, sizeof(research_name));

      notify_player(pplayer, nullptr, E_TECH_GAIN, ftc_server,
                    Q_("?fromscript:You acquire %s."),
                    qUtf8Printable(adv_name));
      notify_research(presearch, pplayer, E_TECH_GAIN, ftc_server,
                      /* TRANS: "The Greeks ..." or "The members of
                       * team Red ..." */
                      Q_("?fromscript:The %s acquire %s and share this "
                         "advance with you."),
                      nation_plural_for_player(pplayer),
                      qUtf8Printable(adv_name));
      notify_research_embassies(presearch, nullptr, E_TECH_EMBASSY,
                                ftc_server,
                                /* TRANS: "The Greeks ..." or "The members of
                                 * team Red ..." */
                                Q_("?fromscript:The %s acquire %s."),
                                research_name, qUtf8Printable(adv_name));
    }

    return result;
  } else {
    return nullptr;
  }
}

/**
   Modify player's trait value.
 */
bool edit_trait_mod_set(sol::this_state s, player *pplayer,
                        const char *tname, const int mod)
{
  enum trait tr;

  tr = trait_by_name(tname, fc_strcasecmp);

  LUASCRIPT_CHECK_ARG(s, trait_is_valid(tr), 3, "no such trait", 0);

  pplayer->ai_common.traits[tr].mod += mod;

  return true;
}

/**
   Create a new owned extra.
 */
void edit_create_owned_extra(tile *ptile, const char *name, player *pplayer)
{
  struct extra_type *pextra;

  if (!name) {
    return;
  }

  pextra = extra_type_by_rule_name(name);

  if (pextra) {
    create_extra(ptile, pextra, pplayer);
    update_tile_knowledge(ptile);
  }
}

/**
   Create a new extra.
 */
void edit_create_extra(tile *ptile, const char *name)
{
  edit_create_owned_extra(ptile, name, nullptr);
}

/**
   Remove extra from tile, if present
 */
void edit_remove_extra(tile *ptile, const char *name)
{
  struct extra_type *pextra;

  if (!name) {
    return;
  }

  pextra = extra_type_by_rule_name(name);

  if (pextra != nullptr && tile_has_extra(ptile, pextra)) {
    tile_extra_rm_apply(ptile, pextra);
    update_tile_knowledge(ptile);
  }
}

/**
   Set tile label text.
 */
void edit_tile_set_label(tile *ptile, const char *label)
{
  tile_set_label(ptile, label);
  if (server_state() >= S_S_RUNNING) {
    send_tile_info(nullptr, ptile, false);
  }
}

/**
   Global climate change.
 */
void edit_climate_change(sol::this_state s, enum climate_change_type type,
                         int effect)
{
  LUASCRIPT_CHECK_ARG(s,
                      type == CLIMATE_CHANGE_GLOBAL_WARMING
                          || type == CLIMATE_CHANGE_NUCLEAR_WINTER,
                      2, "invalid climate change type");
  LUASCRIPT_CHECK_ARG(s, effect > 0, 3, "effect must be greater than zero");

  ::climate_change(type == CLIMATE_CHANGE_GLOBAL_WARMING, effect);
}

/**
   Provoke a civil war.
 */
player *edit_civil_war(sol::this_state s, player *pplayer, int probability)
{
  LUASCRIPT_CHECK_ARG(s, probability >= 0 && probability <= 100, 3,
                      "must be a percentage", nullptr);

  if (!civil_war_possible(pplayer, false, false)) {
    return nullptr;
  }

  if (probability == 0) {
    // Calculate chance with normal rules
    if (!civil_war_triggered(pplayer)) {
      return nullptr;
    }
  } else {
    // Fixed chance specified by script
    if (fc_rand(100) >= probability) {
      return nullptr;
    }
  }

  return civil_war(pplayer);
}

/**
   Make player winner of the scenario
 */
void edit_player_victory(player *pplayer)
{
  player_status_add(pplayer, PSTATUS_WINNER);
}

/**
   Move a unit.
 */
bool edit_unit_move(sol::this_state s, unit *punit, tile *ptile,
                    int movecost)
{
  struct city *pcity;

  LUASCRIPT_CHECK_ARG(s, movecost >= 0, 4, "Negative move cost!", false);

  return unit_move(
      punit, ptile, movecost,
      /* Auto embark kept for backward compatibility. I have
       * no objection if you see the old behavior as a bug and
       * remove auto embarking completely or for transports
       * the unit can't legally board. -- Sveinung */
      nullptr, true,
      /* Backwards compatibility for old scripts in rulesets
       * and (scenario) savegames. I have no objection if you
       * see the old behavior as a bug and remove auto
       * conquering completely or for cities the unit can't
       * legally conquer. -- Sveinung */
      ((pcity = tile_city(ptile))
       && (unit_owner(punit)->ai_common.barbarian_type != ANIMAL_BARBARIAN)
       && uclass_has_flag(unit_class_get(punit), UCF_CAN_OCCUPY_CITY)
       && !unit_has_type_flag(punit, UTYF_CIVILIAN)
       && pplayers_at_war(unit_owner(punit), city_owner(pcity))));
}

/**
   Prohibit unit from moving
 */
void edit_movement_disallow(unit *punit)
{
  if (punit != nullptr) {
    punit->stay = true;
  }
}

/**
   Allow unit to move
 */
void edit_movement_allow(unit *punit)
{
  if (punit != nullptr) {
    punit->stay = false;
  }
}

/**
   Add history to a city
 */
void edit_add_city_history(city *pcity, int amount)
{
  pcity->history += amount;
}

/**
   Add history to a player
 */
void edit_add_player_history(player *pplayer, int amount)
{
  pplayer->history += amount;
}

const std::string setup_lua = R"(
-- Server functions for Player module
function Player:create_unit(tile, utype, veteran_level, homecity, moves_left)
  return edit.create_unit(self, tile, utype, veteran_level, homecity,
                          moves_left)
end

function Player:create_unit_full(tile, utype, veteran_level, homecity,
                                 moves_left, hp_left, ptransport)
  return edit.create_unit_full(self, tile, utype, veteran_level, homecity,
                               moves_left, hp_left, ptransport)
end

function Player:civilization_score()
  return server.civilization_score(self)
end

function Player:create_city(tile, name)
  edit.create_city(self, tile, name)
end

function Player:change_gold(amount)
  edit.change_gold(self, amount)
end

function Player:give_tech(tech, cost, notify, reason)
  return edit.give_tech(self, tech, cost, notify, reason)
end

function Player:trait_mod(trait, mod)
  return edit.trait_mod(self, trait, mod)
end

function Player:civil_war(probability)
  return edit.civil_war(self, probability)
end

function Player:victory()
  edit.player_victory(self)
end

function Player:add_history(amount)
  edit.add_player_history(self, amount)
end

-- Server functions for City module
function City:add_history(amount)
  edit.add_city_history(self, amount)
end

-- Server functions for Unit module
function Unit:teleport(dest)
  return edit.unit_teleport(self, dest)
end

function Unit:turn(direction)
  edit.unit_turn(self, direction)
end

function Unit:kill(reason, killer)
  edit.unit_kill(self, reason, killer)
end

function Unit:move(moveto, movecost)
  return edit.unit_move(self, moveto, movecost)
end

function Unit:movement_disallow()
  edit.movement_disallow(self)
end

function Unit:movement_allow()
  edit.movement_allow(self)
end

-- Server functions for Tile module
function Tile:create_owned_extra(name, player)
  edit.create_owned_extra(self, name, player)
end

function Tile:create_extra(name)
  edit.create_extra(self, name)
end

function Tile:remove_extra(name)
  edit.remove_extra(self, name)
end

function Tile:change_terrain(terrain)
  edit.change_terrain(self, terrain)
end

function Tile:unleash_barbarians()
  return edit.unleash_barbarians(self)
end

function Tile:place_partisans(player, count, sq_radius)
  edit.place_partisans(self, player, count, sq_radius)
end

function Tile:set_label(label)
  edit.tile_set_label(self, label)
end
)";
} // namespace

void setup_server_edit(sol::state_view lua)
{
  auto edit = lua["edit"].get_or_create<sol::table>();
  edit["climate_change"] = edit_climate_change;
  edit["create_unit"] = edit_create_unit;
  edit["create_unit_full"] = edit_create_unit_full;
  edit["unit_teleport"] = edit_unit_teleport;
  edit["unit_kill"] = edit_unit_kill;
  edit["change_terrain"] = edit_change_terrain;
  edit["create_city"] = edit_create_city;
  edit["create_owned_extra"] = edit_create_owned_extra;
  edit["create_extra"] = edit_create_extra;
  edit["remove_extra"] = edit_remove_extra;
  edit["tile_set_label"] = edit_tile_set_label;
  edit["create_player"] = edit_create_player;
  edit["change_gold"] = edit_change_gold;
  /* cost:
   *     0 or above - The exact cost % to apply
   *    -1          - Apply freecost
   *    -2          - Apply conquercost
   *    -3          - Apply diplbulbcost */
  edit["give_tech"] = edit_give_technology;
  edit["trait_mod"] = edit_trait_mod_set;
  edit["unleash_barbarians"] = unleash_barbarians;
  edit["place_partisans"] = edit_place_partisans;
  edit["civil_war"] = edit_civil_war;
  edit["unit_turn"] = edit_unit_turn;
  edit["player_victory"] = edit_player_victory;
  edit["unit_move"] = edit_unit_move;
  edit["movement_disallow"] = edit_movement_disallow;
  edit["movement_allow"] = edit_movement_allow;
  edit["add_city_history"] = edit_add_city_history;
  edit["add_player_history"] = edit_add_player_history;

  lua.script(setup_lua);
}
