/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// utility
#include "deprecations.h"
#include "fcintl.h"

// common
#include "achievements.h"
#include "actions.h"
#include "calendar.h"
#include "citizens.h"
#include "culture.h"
#include "featured_text.h"
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

/**
   Return the current turn.
 */
int api_methods_game_turn(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, 0);

  return game.info.turn;
}

/**
   Return the current year.
 */
int api_methods_game_year(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, 0);

  return game.info.year;
}

/**
   Return the current year fragment.
 */
int api_methods_game_year_fragment(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, 0);

  return game.info.fragment_count;
}

/**
   Return the current year fragment.
 */
const char *api_methods_game_year_text(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);

  return calendar_text();
}

/**
   Return the current turn, as if real turns started from 0.
 */
int api_methods_game_turn_deprecated(lua_State *L)
{
  LUASCRIPT_CHECK_STATE(L, 0);

  qCWarning(deprecations_category,
            "Deprecated: lua construct \"game:turn\", deprecated "
            "since \"3.0\", used. "
            "Use \"game:current_turn\" instead.");

  if (game.info.turn > 0) {
    return game.info.turn - 1;
  }

  return game.info.turn;
}

/**
   Return name of the current ruleset.
 */
const char *api_methods_game_rulesetdir(lua_State *L)
{
  Q_UNUSED(L)
  return game.server.rulesetdir;
}

/**
   Return name of the current ruleset.
 */
const char *api_methods_game_ruleset_name(lua_State *L)
{
  Q_UNUSED(L)
  return game.control.name;
}

/**
   Return TRUE if pbuilding is a wonder.
 */
bool api_methods_building_type_is_wonder(lua_State *L,
                                         Building_Type *pbuilding)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pbuilding, false);

  return is_wonder(pbuilding);
}

/**
   Return TRUE if pbuilding is a great wonder.
 */
bool api_methods_building_type_is_great_wonder(lua_State *L,
                                               Building_Type *pbuilding)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pbuilding, false);

  return is_great_wonder(pbuilding);
}

/**
   Return TRUE if pbuilding is a small wonder.
 */
bool api_methods_building_type_is_small_wonder(lua_State *L,
                                               Building_Type *pbuilding)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pbuilding, false);

  return is_small_wonder(pbuilding);
}

/**
   Return TRUE if pbuilding is a building.
 */
bool api_methods_building_type_is_improvement(lua_State *L,
                                              Building_Type *pbuilding)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pbuilding, false);

  return is_improvement(pbuilding);
}

/**
   Return rule name for Building_Type
 */
const char *api_methods_building_type_rule_name(lua_State *L,
                                                Building_Type *pbuilding)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pbuilding, nullptr);

  return improvement_rule_name(pbuilding);
}

/**
   Return translated name for Building_Type
 */
const char *
api_methods_building_type_name_translation(lua_State *L,
                                           Building_Type *pbuilding)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pbuilding, nullptr);

  return improvement_name_translation(pbuilding);
}

/**
   Return TRUE iff city has building
 */
bool api_methods_city_has_building(lua_State *L, City *pcity,
                                   Building_Type *building)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pcity, false);
  LUASCRIPT_CHECK_ARG_NIL(L, building, 3, Building_Type, false);

  return city_has_building(pcity, building);
}

/**
   Return the square raduis of the city map.
 */
int api_methods_city_map_sq_radius(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pcity, 0);

  return city_map_radius_sq_get(pcity);
}

/**
   Return the size of the city.
 */
int api_methods_city_size_get(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, 1);
  LUASCRIPT_CHECK_SELF(L, pcity, 1);

  return city_size_get(pcity);
}

/**
   Return the tile of the city.
 */
Tile *api_methods_city_tile_get(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pcity, nullptr);

  return pcity->tile;
}

/**
   How much city inspires partisans for a player.
 */
int api_methods_city_inspire_partisans(lua_State *L, City *self,
                                       Player *inspirer)
{
  Q_UNUSED(L)
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
   How much culture city has?
 */
int api_methods_city_culture_get(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pcity, 0);

  return city_culture(pcity);
}

/**
   Return TRUE iff city happy
 */
bool api_methods_is_city_happy(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pcity, false);

  // Note: if clients ever have virtual cities or sth, needs amending
  return is_server() ? city_happy(pcity) : pcity->client.happy;
}

/**
   Return TRUE iff city is unhappy
 */
bool api_methods_is_city_unhappy(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pcity, false);

  // Note: if clients ever have virtual cities or sth, needs amending
  return is_server() ? city_unhappy(pcity) : pcity->client.unhappy;
}

/**
   Return TRUE iff city is celebrating
 */
bool api_methods_is_city_celebrating(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pcity, false);

  return city_celebrating(pcity);
}

/**
   Return TRUE iff city is government center
 */
bool api_methods_is_gov_center(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pcity, false);

  return is_gov_center(pcity);
}

/**
   Return TRUE if city is capital
 */
bool api_methods_is_capital(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pcity, false);

  return is_capital(pcity);
}

/**
    Return rule name for Government
 */
const char *api_methods_government_rule_name(lua_State *L,
                                             Government *pgovernment)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pgovernment, nullptr);

  return government_rule_name(pgovernment);
}

/**
   Return translated name for Government
 */
const char *api_methods_government_name_translation(lua_State *L,
                                                    Government *pgovernment)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pgovernment, nullptr);

  return government_name_translation(pgovernment);
}

/**
   Return rule name for Nation_Type
 */
const char *api_methods_nation_type_rule_name(lua_State *L,
                                              Nation_Type *pnation)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pnation, nullptr);

  return nation_rule_name(pnation);
}

/**
   Return translated adjective for Nation_Type
 */
const char *api_methods_nation_type_name_translation(lua_State *L,
                                                     Nation_Type *pnation)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pnation, nullptr);

  return nation_adjective_translation(pnation);
}

/**
   Return translated plural noun for Nation_Type
 */
const char *api_methods_nation_type_plural_translation(lua_State *L,
                                                       Nation_Type *pnation)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pnation, nullptr);

  return nation_plural_translation(pnation);
}

/**
   Return gui type string of the controlling connection.
 */
const char *api_methods_player_controlling_gui(lua_State *L, Player *pplayer)
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
bool api_methods_player_has_wonder(lua_State *L, Player *pplayer,
                                   Building_Type *building)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pplayer, false);
  LUASCRIPT_CHECK_ARG_NIL(L, building, 3, Building_Type, false);

  return wonder_is_built(pplayer, building);
}

/**
   Return player number
 */
int api_methods_player_number(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, pplayer, -1);

  return player_number(pplayer);
}

/**
   Return the number of cities pplayer has.
 */
int api_methods_player_num_cities(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  return city_list_size(pplayer->cities);
}

/**
   Return the number of units pplayer has.
 */
int api_methods_player_num_units(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  return unit_list_size(pplayer->units);
}

/**
   Return gold for Player
 */
int api_methods_player_gold(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  return pplayer->economic.gold;
}

/**
   Return TRUE if Player knows advance ptech.
 */
bool api_methods_player_knows_tech(lua_State *L, Player *pplayer,
                                   Tech_Type *ptech)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pplayer, false);
  LUASCRIPT_CHECK_ARG_NIL(L, ptech, 3, Tech_Type, false);

  return research_invention_state(research_get(pplayer),
                                  advance_number(ptech))
         == TECH_KNOWN;
}

/**
   How much culture player has?
 */
int api_methods_player_culture_get(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  return player_culture(pplayer);
}

/**
   Does player have flag set?
 */
bool api_methods_player_has_flag(lua_State *L, Player *pplayer,
                                 const char *flag)
{
  enum plr_flag_id flag_val;

  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pplayer, 0);

  flag_val = plr_flag_id_by_name(flag, fc_strcasecmp);

  if (plr_flag_id_is_valid(flag_val)) {
    return player_has_flag(pplayer, flag_val);
  }

  return false;
}

/**
   Return TRUE if players share research.
 */
bool api_methods_player_shares_research(lua_State *L, Player *pplayer,
                                        Player *aplayer)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, pplayer, false);
  LUASCRIPT_CHECK_ARG_NIL(L, aplayer, 3, Player, false);

  return research_get(pplayer) == research_get(aplayer);
}

/**
   Return name of the research group player belongs to.
 */
const char *api_methods_research_rule_name(lua_State *L, Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pplayer, nullptr);

  return research_rule_name(research_get(pplayer));
}

/**
   Return name of the research group player belongs to.
 */
const char *api_methods_research_name_translation(lua_State *L,
                                                  Player *pplayer)
{
  static char buf[MAX_LEN_MSG];

  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pplayer, nullptr);

  (void) research_pretty_name(research_get(pplayer), buf, ARRAY_SIZE(buf));

  return buf;
}

/**
   Return list head for unit list for Player
 */
Unit_List_Link *api_methods_private_player_unit_list_head(lua_State *L,
                                                          Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pplayer, nullptr);
  return unit_list_head(pplayer->units);
}

/**
   Return list head for city list for Player
 */
City_List_Link *api_methods_private_player_city_list_head(lua_State *L,
                                                          Player *pplayer)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pplayer, nullptr);

  return city_list_head(pplayer->cities);
}

/**
   Return rule name for Tech_Type
 */
const char *api_methods_tech_type_rule_name(lua_State *L, Tech_Type *ptech)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, ptech, nullptr);

  return advance_rule_name(ptech);
}

/**
   Return translated name for Tech_Type
 */
const char *api_methods_tech_type_name_translation(lua_State *L,
                                                   Tech_Type *ptech)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, ptech, nullptr);

  return advance_name_translation(ptech);
}

/**
   Return rule name for Terrain
 */
const char *api_methods_terrain_rule_name(lua_State *L, Terrain *pterrain)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pterrain, nullptr);

  return terrain_rule_name(pterrain);
}

/**
   Return translated name for Terrain
 */
const char *api_methods_terrain_name_translation(lua_State *L,
                                                 Terrain *pterrain)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pterrain, nullptr);

  return terrain_name_translation(pterrain);
}

/**
   Return name of the terrain's class
 */
const char *api_methods_terrain_class_name(lua_State *L, Terrain *pterrain)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pterrain, nullptr);

  return terrain_class_name(terrain_type_terrain_class(pterrain));
}

/**
   Return rule name for Disaster
 */
const char *api_methods_disaster_rule_name(lua_State *L, Disaster *pdis)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pdis, nullptr);

  return disaster_rule_name(pdis);
}

/**
   Return translated name for Disaster
 */
const char *api_methods_disaster_name_translation(lua_State *L,
                                                  Disaster *pdis)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pdis, nullptr);

  return disaster_name_translation(pdis);
}

/**
   Return rule name for Achievement
 */
const char *api_methods_achievement_rule_name(lua_State *L,
                                              Achievement *pach)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pach, nullptr);

  return achievement_rule_name(pach);
}

/**
   Return translated name for Achievement
 */
const char *api_methods_achievement_name_translation(lua_State *L,
                                                     Achievement *pach)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pach, nullptr);

  return achievement_name_translation(pach);
}

/**
   Return rule name for Action
 */
const char *api_methods_action_rule_name(lua_State *L, Action *pact)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pact, nullptr);

  return action_id_rule_name(pact->id);
}

/**
   Return translated name for Action
 */
const char *api_methods_action_name_translation(lua_State *L, Action *pact)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pact, nullptr);

  return qUtf8Printable(action_id_name_translation(pact->id));
}

/**
   Return the native x coordinate of the tile.
 */
int api_methods_tile_nat_x(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, ptile, -1);

  return index_to_native_pos_x(tile_index(ptile));
}

/**
   Return the native y coordinate of the tile.
 */
int api_methods_tile_nat_y(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, ptile, -1);

  return index_to_native_pos_y(tile_index(ptile));
}

/**
   Return the map x coordinate of the tile.
 */
int api_methods_tile_map_x(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, ptile, -1);

  return index_to_map_pos_x(tile_index(ptile));
}

/**
   Return the map y coordinate of the tile.
 */
int api_methods_tile_map_y(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, -1);
  LUASCRIPT_CHECK_SELF(L, ptile, -1);

  return index_to_map_pos_y(tile_index(ptile));
}

/**
   Return City on ptile, else nullptr
 */
City *api_methods_tile_city(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, ptile, nullptr);

  return tile_city(ptile);
}

/**
   Return TRUE if there is a city inside the maximum city radius from ptile.
 */
bool api_methods_tile_city_exists_within_max_city_map(lua_State *L,
                                                      Tile *ptile,
                                                      bool may_be_on_center)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, ptile, false);

  return city_exists_within_max_city_map(ptile, may_be_on_center);
}

/**
   Return TRUE if there is a extra with rule name name on ptile.
   If no name is specified return true if there is a extra on ptile.
 */
bool api_methods_tile_has_extra(lua_State *L, Tile *ptile, const char *name)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, ptile, false);

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
    struct extra_type *pextra;

    pextra = extra_type_by_rule_name(name);

    return (nullptr != pextra && tile_has_extra(ptile, pextra));
  }
}

/**
   Return TRUE if there is a base with rule name name on ptile.
   If no name is specified return true if there is any base on ptile.
 */
bool api_methods_tile_has_base(lua_State *L, Tile *ptile, const char *name)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, ptile, false);

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
bool api_methods_tile_has_road(lua_State *L, Tile *ptile, const char *name)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, ptile, false);

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
bool api_methods_enemy_tile(lua_State *L, Tile *ptile, Player *against)
{
  struct city *pcity;

  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, ptile, false);

  if (is_non_allied_unit_tile(ptile, against)) {
    return true;
  }

  pcity = tile_city(ptile);
  return pcity != nullptr && !pplayers_allied(against, city_owner(pcity));
}

/**
   Return number of units on tile
 */
int api_methods_tile_num_units(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, ptile, 0);

  return unit_list_size(ptile->units);
}

/**
   Return list head for unit list for Tile
 */
Unit_List_Link *api_methods_private_tile_unit_list_head(lua_State *L,
                                                        Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, ptile, nullptr);

  return unit_list_head(ptile->units);
}

/**
   Return nth tile iteration index (for internal use)
   Will return the next index, or an index < 0 when done
 */
int api_methods_private_tile_next_outward_index(lua_State *L, Tile *pstart,
                                                int tindex, int max_dist)
{
  int dx, dy;
  int newx, newy;
  int startx, starty;

  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, pstart, 0);

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
Tile *api_methods_private_tile_for_outward_index(lua_State *L, Tile *pstart,
                                                 int tindex)
{
  int newx, newy;

  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, pstart, nullptr);
  LUASCRIPT_CHECK_ARG(
      L, tindex >= 0 && tindex < wld.map.num_iterate_outwards_indices, 3,
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
   Return squared distance between tiles 1 and 2
 */
int api_methods_tile_sq_distance(lua_State *L, Tile *ptile1, Tile *ptile2)
{
  LUASCRIPT_CHECK_STATE(L, 0);
  LUASCRIPT_CHECK_SELF(L, ptile1, 0);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile2, 3, Tile, 0);

  return sq_map_distance(ptile1, ptile2);
}

/**
   Can punit found a city on its tile?
 */
bool api_methods_unit_city_can_be_built_here(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, punit, false);

  return city_can_be_built_here(unit_tile(punit), punit);
}

/**
   Return the tile of the unit.
 */
Tile *api_methods_unit_tile_get(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, punit, nullptr);

  return unit_tile(punit);
}

/**
   Get unit orientation
 */
const Direction *api_methods_unit_orientation_get(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_ARG_NIL(L, punit, 2, Unit, nullptr);

  return luascript_dir(punit->facing);
}

/**
   Return Unit that transports punit, if any.
 */
Unit *api_methods_unit_transporter(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, punit, nullptr);

  return punit->transporter;
}

/**
   Return list head for cargo list for Unit
 */
Unit_List_Link *api_methods_private_unit_cargo_list_head(lua_State *L,
                                                         Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, punit, nullptr);
  return unit_list_head(punit->transporting);
}

/**
   Return TRUE if punit_type has flag.
 */
bool api_methods_unit_type_has_flag(lua_State *L, Unit_Type *punit_type,
                                    const char *flag)
{
  enum unit_type_flag_id id;

  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, punit_type, false);
  LUASCRIPT_CHECK_ARG_NIL(L, flag, 3, string, false);

  id = unit_type_flag_id_by_name(flag, fc_strcasecmp);
  if (unit_type_flag_id_is_valid(id)) {
    return utype_has_flag(punit_type, id);
  } else {
    luascript_error(L, "Unit type flag \"%s\" does not exist", flag);
    return false;
  }
}

/**
   Return TRUE if punit_type has role.
 */
bool api_methods_unit_type_has_role(lua_State *L, Unit_Type *punit_type,
                                    const char *role)
{
  enum unit_role_id id;

  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, punit_type, false);
  LUASCRIPT_CHECK_ARG_NIL(L, role, 3, string, false);

  id = unit_role_id_by_name(role, fc_strcasecmp);
  if (unit_role_id_is_valid(id)) {
    return utype_has_role(punit_type, id);
  } else {
    luascript_error(L, "Unit role \"%s\" does not exist", role);
    return false;
  }
}

/**
   Return TRUE iff the unit type can exist on the tile.
 */
bool api_methods_unit_type_can_exist_at_tile(lua_State *L,
                                             Unit_Type *punit_type,
                                             Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, false);
  LUASCRIPT_CHECK_SELF(L, punit_type, false);
  LUASCRIPT_CHECK_ARG_NIL(L, ptile, 3, Tile, false);

  return can_exist_at_tile(&(wld.map), punit_type, ptile);
}

/**
   Return rule name for Unit_Type
 */
const char *api_methods_unit_type_rule_name(lua_State *L,
                                            Unit_Type *punit_type)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, punit_type, nullptr);

  return utype_rule_name(punit_type);
}

/**
   Return translated name for Unit_Type
 */
const char *api_methods_unit_type_name_translation(lua_State *L,
                                                   Unit_Type *punit_type)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);
  LUASCRIPT_CHECK_SELF(L, punit_type, nullptr);

  return utype_name_translation(punit_type);
}

/**
   Return Unit for list link
 */
Unit *api_methods_unit_list_link_data(lua_State *L, Unit_List_Link *ul_link)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);

  return unit_list_link_data(ul_link);
}

/**
   Return next list link or nullptr when link is the last link
 */
Unit_List_Link *api_methods_unit_list_next_link(lua_State *L,
                                                Unit_List_Link *ul_link)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);

  return unit_list_link_next(ul_link);
}

/**
   Return City for list link
 */
City *api_methods_city_list_link_data(lua_State *L, City_List_Link *cl_link)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);

  return city_list_link_data(cl_link);
}

/**
   Return next list link or nullptr when link is the last link
 */
City_List_Link *api_methods_city_list_next_link(lua_State *L,
                                                City_List_Link *cl_link)
{
  LUASCRIPT_CHECK_STATE(L, nullptr);

  return city_list_link_next(cl_link);
}

/**
 * Return featured text link of the tile.
 */
const char *api_methods_tile_link(lua_State *L, Tile *ptile)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, ptile, NULL);
  return tile_link(ptile);
}

/**
 * Return featured text link of the unit tile.
 */
const char *api_methods_unit_tile_link(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, punit, NULL);
  return unit_tile_link(punit);
}

/**
 * Return featured text link of the city tile.
 */
const char *api_methods_city_tile_link(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, pcity, NULL);
  return city_tile_link(pcity);
}

/**
 * Return featured text link of the unit.
 */
const char *api_methods_unit_link(lua_State *L, Unit *punit)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, punit, NULL);
  return unit_link(punit);
}

/**
 * Return featured text link of the city.
 */
const char *api_methods_city_link(lua_State *L, City *pcity)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_SELF(L, pcity, NULL);
  return city_link(pcity);
}
