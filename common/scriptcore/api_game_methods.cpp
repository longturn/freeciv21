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
#include "deprecations.h"
#include "fcintl.h"

// common
#include "achievements.h"
#include "actions.h"
#include "calendar.h"
#include "citizens.h"
#include "culture.h"
#include "game.h"
#include "government.h"
#include "improvement.h"
#include "map.h"
#include "movement.h"
#include "nation.h"
#include "research.h"
#include "tech.h"
#include "terrain.h"
#include "tile.h"
#include "unitlist.h"
#include "unittype.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_game_methods.h"

namespace sol {

namespace stack {
template <> struct unqualified_pusher<::QString> {
  static int push(lua_State *L, const ::QString &str)
  {
    return stack::push(L, qUtf8Printable(str));
  }
};

template <> struct unqualified_getter<::QString> {
  static ::QString get(lua_State *L, int index)
  {
    return QString(stack::get<const char *>(L, index));
  }
};

} // namespace stack

} // namespace sol

struct unit_list_link {
  struct unit_list_link *next, *prev;
  void *dataptr;
};

struct city_list_link {
  struct unit_list_link *next, *prev;
  void *dataptr;
};

namespace {

/**
   Return the current turn.
 */
int game_turn() { return game.info.turn; }

/**
   Return the current year.
 */
int game_year() { return game.info.year; }

/**
   Return the current year fragment.
 */
int game_year_fragment() { return game.info.fragment_count; }

/**
   Return name of the current ruleset.
 */
const char *game_rulesetdir() { return game.server.rulesetdir; }

/**
   Return name of the current ruleset.
 */
const char *game_ruleset_name() { return game.control.name; }

/**
   Register game table.
 */
sol::table register_game(sol::state_view lua)
{
  auto game = lua["game"].get_or_create<sol::table>();

  game.set("turn", game_turn);
  game.set("current_year", game_year);
  game.set("year_text", calendar_text);
  game.set("current_fragment", game_year_fragment);
  game.set("rulesetdir", game_rulesetdir);
  game.set("ruleset_name", game_ruleset_name);

  return game;
}

/**
   How much city inspires partisans for a player.
 */
int city_inspire_partisans(city *self, player *inspirer)
{
  bool inspired = false;

  if (!game.info.citizen_nationality) {
    if (self->original == inspirer) {
      inspired = true;
    }
  } else {
    if (game.info.citizen_partisans_pct > 0) {
      if (is_server()) {
        int own = citizens_nation_get(self, inspirer->slot);
        int total = 0;

        /* Not citizens_foreign_iterate() as city has already changed hands.
         * old owner would be considered foreign and new owner not. */
        citizens_iterate(self, pslot, nat) { total += nat; }
        citizens_iterate_end;

        if (total != 0
            && ((own * 100 / total) >= game.info.citizen_partisans_pct)) {
          inspired = true;
        }
      }
      // else is_client() -> don't consider inspired by default.
    } else if (self->original == inspirer) {
      inspired = true;
    }
  }

  if (inspired) {
    /* Cannot use get_city_bonus() as it would use city's current owner
     * instead of inspirer. */
    return get_target_bonus_effects(
        nullptr, inspirer, nullptr, self, nullptr, city_tile(self), nullptr,
        nullptr, nullptr, nullptr, nullptr, EFT_INSPIRE_PARTISANS);
  }

  return 0;
}

/**
   Return TRUE iff city happy
 */
bool city_is_happy(city *pcity)
{
  // Note: if clients ever have virtual cities or sth, needs amending
  return is_server() ? city_happy(pcity) : pcity->client.happy;
}

/**
   Return TRUE iff city is unhappy
 */
bool city_is_unhappy(city *pcity)
{
  // Note: if clients ever have virtual cities or sth, needs amending
  return is_server() ? city_unhappy(pcity) : pcity->client.unhappy;
}

/**
   Register city metatable.
 */
sol::usertype<city> register_city(sol::state_view lua)
{
  auto city = lua.new_usertype<::city>("City", sol::no_constructor);
  city.set("name", sol::readonly(&city::name));
  city.set("owner", sol::readonly(&city::owner));
  city.set("original", sol::readonly(&city::original));
  city.set("id", sol::readonly(&city::id));

  city.set("map_sq_radius", sol::property(city_map_radius_sq_get));
  city.set("size", sol::property(city_size_get));
  city.set("tile", sol::property(city_tile));
  city.set("culture", sol::property(city_culture));
  city.set("is_capital", sol::property(is_capital));
  city.set("is_gov_center", sol::property(is_gov_center));
  city.set("is_celebrating", sol::property(city_celebrating));
  city.set("is_unhappy", sol::property(city_is_unhappy));
  city.set("is_happy", sol::property(city_is_happy));

  city.set("inspire_partisans", city_inspire_partisans);
  city.set("has_building", city_has_building);

  return city;
}

/**
   Register government metatable.
 */
sol::usertype<government> register_government(sol::state_view lua)
{
  auto government =
      lua.new_usertype<::government>("Government", sol::no_constructor);
  government.set("id", sol::readonly(&government::item_number));

  government.set("rule_name", sol::property(government_rule_name));
  government.set("name_translation",
                 sol::property(government_name_translation));

  return government;
}

/**
   Register nation metatable.
 */
sol::usertype<nation_type> register_nation(sol::state_view lua)
{
  auto nation =
      lua.new_usertype<nation_type>("Nation_Type", sol::no_constructor);
  nation.set("id", sol::readonly(&nation_type::item_number));
  nation.set("rule_name", sol::property(nation_rule_name));
  nation.set("name_translation",
             sol::property(nation_adjective_translation));
  nation.set("plural_translation", sol::property(nation_plural_translation));

  return nation;
}

/**
   Return gui type string of the controlling connection.
 */
const char *player_controlling_gui(player *pplayer)
{
  static bool warned = false;
  if (!warned) {
    qWarning() << "player:controlling_gui is deprecated";
    warned = true;
  }
  return "Qt";
}

/**
   Return TRUE iff player has wonder
 */
bool player_has_wonder(sol::this_state s, player *pplayer,
                       impr_type *building)
{
  return wonder_is_built(pplayer, building);
}

/**
   Return the number of cities pplayer has.
 */
int player_num_cities(player *pplayer)
{
  return city_list_size(pplayer->cities);
}

/**
   Return the number of units pplayer has.
 */
int player_num_units(player *pplayer)
{
  return unit_list_size(pplayer->units);
}

/**
   Return gold for Player
 */
int player_gold(player *pplayer) { return pplayer->economic.gold; }

/**
   Return TRUE if Player knows advance ptech.
 */
bool player_knows_tech(player *pplayer, advance *ptech)
{
  return research_invention_state(research_get(pplayer),
                                  advance_number(ptech))
         == TECH_KNOWN;
}

/**
   Does player have flag set?
 */
bool player_has_flag(player *pplayer, const char *flag)
{
  enum plr_flag_id flag_val;

  flag_val = plr_flag_id_by_name(flag, fc_strcasecmp);

  if (plr_flag_id_is_valid(flag_val)) {
    return player_has_flag(pplayer, flag_val);
  }

  return false;
}

/**
   Return TRUE if players share research.
 */
bool player_shares_research(player *pplayer, player *aplayer)
{
  return research_get(pplayer) == research_get(aplayer);
}

/**
   Return name of the research group player belongs to.
 */
const char *player_research_rule_name(player *pplayer)
{
  return research_rule_name(research_get(pplayer));
}

/**
   Return name of the research group player belongs to.
 */
const char *player_research_name_translation(player *pplayer)
{
  static char buf[MAX_LEN_MSG];

  (void) research_pretty_name(research_get(pplayer), buf, ARRAY_SIZE(buf));

  return buf;
}

/**
   Return list head for unit list for Player
 */
unit_list_link *player_unit_list_head(player *pplayer)
{
  return unit_list_head(pplayer->units);
}

/**
   Register player metatable.
 */
sol::usertype<player> register_player(sol::state_view lua)
{
  auto player = lua.new_usertype<::player>("Player", sol::no_constructor);

  player.set("name", sol::readonly(&player::name));
  player.set("nation", sol::readonly(&player::nation));
  player.set("is_alive", sol::readonly(&player::is_alive));

  player.set("id", sol::property(player_number));
  player.set("controlling_gui", sol::property(player_controlling_gui));
  player.set("num_cities", sol::property(player_num_cities));
  player.set("num_units", sol::property(player_num_units));
  player.set("gold", sol::property(player_gold));
  player.set("culture", sol::property(player_culture));

  player.set("has_wonder", player_has_wonder);
  player.set("knows_tech", player_knows_tech);
  player.set("shares_research", player_shares_research);
  player.set("research_rule_name", player_research_rule_name);
  player.set("shares_research", player_shares_research);
  player.set("research_name_translation", player_research_name_translation);
  player.set("has_flag", player_has_flag);

  lua.script(R"(
  function Player:is_human()
    return not self.has_flag(self, "AI");
  end
  )");
  return player;
}

/**
   Register unit list metatable.
 */
sol::usertype<unit_list_link> register_unit_list_link(sol::state_view lua)
{
  auto unit_list = lua.new_usertype<unit_list_link>("Unit_List_Link",
                                                    sol::no_constructor);
  unit_list.set("data", unit_list_link_data);
  unit_list.set("next", unit_list_link_next);

  return unit_list;
}

/**
   Return list head for city list for Player
 */
city_list_link *player_city_list_head(player *pplayer)
{
  return city_list_head(pplayer->cities);
}

/**
   Register city list metatable.
 */
sol::usertype<city_list_link> register_city_list_link(sol::state_view lua)
{
  auto city_list = lua.new_usertype<city_list_link>("City_List_Link",
                                                    sol::no_constructor);
  city_list.set("data", city_list_link_data);
  city_list.set("next", city_list_link_next);

  return city_list;
}

/**
   Register advance metatable.
 */
sol::usertype<advance> register_advance(sol::state_view lua)
{
  auto tech = lua.new_usertype<::advance>("Tech_Type", sol::no_constructor);
  tech.set("id", sol::readonly(&advance::item_number));
  tech.set("rule_name", advance_rule_name);
  tech.set("name_translation", advance_name_translation);

  return tech;
}

/**
   Return name of the terrain's class
 */
const char *class_name(terrain *pterrain)
{
  return terrain_class_name(terrain_type_terrain_class(pterrain));
}

/**
   Register terrain metatable.
 */
sol::usertype<terrain> register_terrain(sol::state_view lua)
{
  auto terrain = lua.new_usertype<::terrain>("Terrain", sol::no_constructor);
  terrain.set("id", sol::readonly(&terrain::item_number));
  terrain.set("name_translation", terrain_name_translation);
  terrain.set("rule_name", terrain_rule_name);
  terrain.set("class_name", class_name);

  return terrain;
}

/**
   Register disaster metatable.
 */
sol::usertype<disaster_type> register_disaster(sol::state_view lua)
{
  auto disaster =
      lua.new_usertype<disaster_type>("Disaster", sol::no_constructor);
  disaster.set("id", sol::readonly(&disaster_type::id));
  disaster.set("rule_name", disaster_rule_name);
  disaster.set("name_translation", disaster_name_translation);

  return disaster;
}

/**
   Register achievement metatable.
 */
sol::usertype<achievement> register_achievement(sol::state_view lua)
{
  auto achievement =
      lua.new_usertype<::achievement>("Achievement", sol::no_constructor);
  achievement.set("id", sol::readonly(&achievement::id));
  achievement.set("rule_name", achievement_rule_name);
  achievement.set("name_translation", achievement_name_translation);

  return achievement;
}

/**
   Return rule name for Action
 */
const char *action_rule_name(action *pact)
{
  return action_id_rule_name(pact->id);
}

/**
   Return translated name for Action
 */
const QString action_name_translation(action *pact)
{
  return action_id_name_translation(pact->id);
}

/**
   Register action metatable.
 */
sol::usertype<action> register_action(sol::state_view lua)
{
  auto action = lua.new_usertype<::action>("Action", sol::no_constructor);
  action.set("id", sol::readonly(&action::id));
  action.set("rule_name", action_rule_name);
  action.set("name_translation", action_name_translation);

  return action;
}

/**
   Return the native x coordinate of the tile.
 */
int tile_nat_x(tile *ptile)
{
  return index_to_native_pos_x(tile_index(ptile));
}

/**
   Return the native y coordinate of the tile.
 */
int tile_nat_y(tile *ptile)
{
  return index_to_native_pos_y(tile_index(ptile));
}

/**
   Return the map x coordinate of the tile.
 */
int tile_map_x(tile *ptile) { return index_to_map_pos_x(tile_index(ptile)); }

/**
   Return the map y coordinate of the tile.
 */
int tile_map_y(tile *ptile) { return index_to_map_pos_y(tile_index(ptile)); }

/**
   Return TRUE if there is a extra with rule name name on ptile.
   If no name is specified return true if there is a extra on ptile.
 */
bool has_extra(tile *ptile, const char *name)
{
  if (!name) {
    extra_type_iterate(pextra)
    {
      if (tile_has_extra(ptile, pextra)) {
        return true;
      }
    }
    extra_type_iterate_end;

    return false;
  } else {
    struct extra_type *pextra = extra_type_by_rule_name(name);

    return (nullptr != pextra && tile_has_extra(ptile, pextra));
  }
}

/**
   Return TRUE if there is a base with rule name name on ptile.
   If no name is specified return true if there is any base on ptile.
 */
bool tile_has_base(tile *ptile, const char *name)
{
  if (!name) {
    extra_type_by_cause_iterate(EC_BASE, pextra)
    {
      if (tile_has_extra(ptile, pextra)) {
        return true;
      }
    }
    extra_type_by_cause_iterate_end;

    return false;
  } else {
    struct extra_type *pextra;

    pextra = extra_type_by_rule_name(name);

    return (nullptr != pextra && is_extra_caused_by(pextra, EC_BASE)
            && tile_has_extra(ptile, pextra));
  }
}

/**
   Return TRUE if there is a road with rule name name on ptile.
   If no name is specified return true if there is any road on ptile.
 */
bool tile_has_road(tile *ptile, const char *name)
{
  if (!name) {
    extra_type_by_cause_iterate(EC_ROAD, pextra)
    {
      if (tile_has_extra(ptile, pextra)) {
        return true;
      }
    }
    extra_type_by_cause_iterate_end;

    return false;
  } else {
    struct extra_type *pextra;

    pextra = extra_type_by_rule_name(name);

    return (nullptr != pextra && is_extra_caused_by(pextra, EC_ROAD)
            && tile_has_extra(ptile, pextra));
  }
}

/**
   Is tile occupied by enemies
 */
bool enemy_tile(tile *ptile, player *against)
{
  struct city *pcity;

  if (is_non_allied_unit_tile(ptile, against)) {
    return true;
  }

  pcity = tile_city(ptile);
  return pcity != nullptr && !pplayers_allied(against, city_owner(pcity));
}

/**
   Return number of units on tile
 */
int tile_num_units(tile *ptile) { return unit_list_size(ptile->units); }

/**
   Return list head for unit list for Tile
 */
unit_list_link *tile_unit_list_head(tile *ptile)
{
  return unit_list_head(ptile->units);
}

/**
   Return nth tile iteration index (for internal use)
   Will return the next index, or an index < 0 when done
 */
int tile_next_outward_index(tile *pstart, int tindex, int max_dist)
{
  int dx, dy;
  int newx, newy;
  int startx, starty;

  if (tindex < 0) {
    return 0;
  }

  index_to_map_pos(&startx, &starty, tile_index(pstart));

  tindex++;
  while (tindex < wld.map.num_iterate_outwards_indices) {
    if (wld.map.iterate_outwards_indices[tindex].dist > max_dist) {
      return -1;
    }
    dx = wld.map.iterate_outwards_indices[tindex].dx;
    dy = wld.map.iterate_outwards_indices[tindex].dy;
    newx = dx + startx;
    newy = dy + starty;
    if (!normalize_map_pos(&(wld.map), &newx, &newy)) {
      tindex++;
      continue;
    }

    return tindex;
  }

  return -1;
}

/**
   Return tile for nth iteration index (for internal use)
 */
tile *tile_for_outward_index(sol::this_state s, tile *pstart, int tindex)
{
  int newx, newy;
  LUASCRIPT_CHECK_ARG(
      s, tindex >= 0 && tindex < wld.map.num_iterate_outwards_indices, 3,
      "index out of bounds", nullptr);

  index_to_map_pos(&newx, &newy, tile_index(pstart));
  newx += wld.map.iterate_outwards_indices[tindex].dx;
  newy += wld.map.iterate_outwards_indices[tindex].dy;

  if (!normalize_map_pos(&(wld.map), &newx, &newy)) {
    return nullptr;
  }

  return map_pos_to_tile(&(wld.map), newx, newy);
}

/**
   Register tile metatable.
 */
sol::usertype<tile> register_tile(sol::state_view lua)
{
  auto tile = lua.new_usertype<::tile>("Tile", sol::no_constructor);
  tile.set("id", sol::readonly(&tile::index));
  tile.set("terrain", sol::readonly(&tile::terrain));
  tile.set("owner", sol::readonly(&tile::owner));

  tile.set("nat_x", sol::property(tile_nat_x));
  tile.set("nat_y", sol::property(tile_nat_y));
  tile.set("map_x", sol::property(tile_map_x));
  tile.set("map_y", sol::property(tile_map_y));

  tile.set("city", tile_city);
  tile.set("sq_distance", sq_map_distance);
  tile.set("num_units", tile_num_units);
  tile.set("is_enemy", enemy_tile);
  tile.set("has_road", tile_has_road);
  tile.set("has_base", tile_has_base);
  tile.set("has_extra", has_extra);
  tile.set("city_exists_within_max_city_map",
           city_exists_within_max_city_map);

  return tile;
}

/**
   Can punit found a city on its tile?
 */
bool unit_city_can_be_built_here(unit *punit)
{
  return city_can_be_built_here(unit_tile(punit), punit);
}

/**
   Get unit orientation
 */
const Direction *unit_facing(unit *punit)
{
  return luascript_dir(punit->facing);
}

/**
   Register unit metatable.
 */
sol::usertype<unit> register_unit(sol::state_view lua)
{
  auto unit = lua.new_usertype<::unit>("Unit", sol::no_constructor);
  unit.set("id", sol::readonly(&unit::id));
  unit.set("tile", sol::readonly(&unit::tile));
  unit.set("utype", sol::readonly(&unit::utype));
  unit.set("owner", sol::readonly(&unit::owner));
  unit.set("homecity", sol::readonly(&unit::homecity));
  unit.set("transporter", sol::readonly(&unit::transporter));

  unit.set("facing", sol::property(unit_facing));

  unit.set("is_on_possible_city_tile", unit_city_can_be_built_here);

  return unit;
}

/**
   Return list head for cargo list for Unit
 */
unit_list_link *unit_cargo_list_head(unit *punit)
{
  return unit_list_head(punit->transporting);
}

/**
   Return TRUE if punit_type has flag.
 */
bool unit_type_has_flag(sol::this_state s, unit_type *punit_type,
                        const char *flag)
{
  enum unit_type_flag_id id = unit_type_flag_id_by_name(flag, fc_strcasecmp);
  if (unit_type_flag_id_is_valid(id)) {
    return utype_has_flag(punit_type, id);
  } else {
    luascript_error(s, "Unit type flag \"%s\" does not exist", flag);
    return false;
  }
}

/**
   Return TRUE if punit_type has role.
 */
bool unit_type_has_role(sol::this_state s, unit_type *punit_type,
                        const char *role)
{
  enum unit_role_id id = unit_role_id_by_name(role, fc_strcasecmp);
  if (unit_role_id_is_valid(id)) {
    return utype_has_role(punit_type, id);
  } else {
    luascript_error(s, "Unit role \"%s\" does not exist", role);
    return false;
  }
}

/**
   Return TRUE iff the unit type can exist on the tile.
 */
bool unit_type_can_exist_at_tile(unit_type *punit_type, tile *ptile)
{
  return can_exist_at_tile(&(wld.map), punit_type, ptile);
}

/**
   Register unit type metatable.
 */
sol::usertype<unit_type> register_unit_type(sol::state_view lua)
{
  auto utype = lua.new_usertype<unit_type>("Unit_Type", sol::no_constructor);
  utype.set("name_translation", sol::property(utype_name_translation));
  utype.set("rule_name", sol::property(utype_rule_name));

  utype.set("can_exist_at_tile", unit_type_can_exist_at_tile);
  utype.set("has_role", unit_type_has_role);
  utype.set("has_flag", unit_type_has_flag);

  return utype;
}

/**
   Register building metatable.
 */
sol::usertype<impr_type> register_building(sol::state_view lua)
{
  auto building =
      lua.new_usertype<impr_type>("Building_Type", sol::no_constructor);
  building.set("is_wonder", is_wonder);
  building.set("is_great_wonder", sol::property(is_great_wonder));
  building.set("is_small_wonder", sol::property(is_small_wonder));
  building.set("is_improvement", sol::property(is_improvement));
  building.set("rule_name", sol::property(improvement_rule_name));
  building.set("name_translation",
               sol::property(improvement_name_translation));

  return building;
}

const std::string private_script = R"(
-- ***************************************************************************
-- Player and Tile: cities_iterate and units_iterate methods
-- ***************************************************************************
do
  local private = methods_private

  -- Iterate over the values of 'array' in order:
  -- array[1], array[2], array[3], etc.
  local function value_iterator(array)
    local i = 0
    local function iterator()
      i = i + 1
      return array[i]
    end
    return iterator
  end

  -- use a copy of the list for safe iteration
  local function safe_iterate_list(link)
    local objs = {}
    while link do
      objs[#objs + 1] = link:data()
      link = link:next()
    end
    return value_iterator(objs)
  end

  -- Safe iteration over all units that belong to Player
  function Player:units_iterate()
    return safe_iterate_list(private.Player.unit_list_head(self))
  end

  -- Safe iteration over all cities that belong to Player
  function Player:cities_iterate()
    return safe_iterate_list(private.Player.city_list_head(self))
  end

  -- Safe iteration over the units on Tile
  function Tile:units_iterate()
    return safe_iterate_list(private.Tile.unit_list_head(self))
  end

  -- Safe iteration over the units transported by Unit
  function Unit:cargo_iterate()
    return safe_iterate_list(private.Unit.cargo_list_head(self))
  end
end

-- ***************************************************************************
-- Tile: square_iterate, circle_iterate
-- ***************************************************************************
do
  local next_outward_index = methods_private.Tile.next_outward_index
  local tile_for_outward_index = methods_private.Tile.tile_for_outward_index

  -- iterate over tiles at distance 'radius'
  function Tile:square_iterate(radius)
    local index = -1
    local function iterator()
      index = next_outward_index(self, index, radius)
      if index < 0 then
        return nil
      else
        return tile_for_outward_index(self, index)
      end
    end
    return iterator
  end

  -- iterate over tiles at squared distance 'sq_radius'
  function Tile:circle_iterate(sq_radius)
    local cr_radius = math.floor(math.sqrt(sq_radius))
    local sq_iter = self:square_iterate(cr_radius)
    local function iterator()
      local tile = nil
      repeat
        tile = sq_iter()
      until not tile or self:sq_distance(tile) <= sq_radius
      return tile
    end
    return iterator
  end
end

-- ***************************************************************************
-- Iteration constructs for game-global objects
-- ***************************************************************************
do
  -- iterate over the values returned by lookup
  -- until nil is returned:
  -- lookup(0), lookup(1), lookup(2), etc
  local function index_iterate(lookup)
    local index = -1
    local function iterator()
      index = index + 1
      return lookup(index)
    end
    return iterator
  end

  -- Iterate over all players of the game
  function players_iterate()
    return index_iterate(find.player)
  end

  -- Iterate over all tiles of the game
  function whole_map_iterate()
    return index_iterate(find.tile)
  end

  -- NOTE: Identical further definitions can be made for
  -- governments, tech_types, building_types etc
end

)";

/**
   Register private methods.
 */
sol::table register_private(sol::state_view lua)
{
  auto priv = lua["methods_private"].get_or_create<sol::table>();
  auto player = priv["Player"].get_or_create<sol::table>();
  player.set("unit_list_head", player_unit_list_head);
  player.set("city_list_head", player_city_list_head);

  auto unit = priv["Unit"].get_or_create<sol::table>();
  unit.set("cargo_list_head", unit_cargo_list_head);

  auto tile = priv["Tile"].get_or_create<sol::table>();
  tile.set("next_outward_index", tile_next_outward_index);
  tile.set("tile_for_outward_index", tile_for_outward_index);
  tile.set("unit_list_head", tile_unit_list_head);

  lua.script(private_script);
  return priv;
}

/**
   Register connection metatable.
 */
sol::usertype<connection> register_connection(sol::state_view lua)
{
  auto conn =
      lua.new_usertype<connection>("Connection", sol::no_constructor);
  conn.set("id", sol::readonly(&connection::id));

  return conn;
}

} // namespace

/**
   Register game methods.
 */
void setup_game_methods(sol::state_view lua)
{
  register_game(lua);
  register_nation(lua);
  register_player(lua);
  register_city(lua);
  register_city_list_link(lua);
  register_building(lua);
  register_government(lua);
  register_unit(lua);
  register_unit_type(lua);
  register_unit_list_link(lua);
  register_achievement(lua);
  register_action(lua);
  register_disaster(lua);
  register_advance(lua);
  register_terrain(lua);
  register_tile(lua);
  register_connection(lua);
  register_private(lua);
}
