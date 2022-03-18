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

// common
#include "idex.h"
#include "map.h"
#include "movement.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_game_find.h"

namespace {

/**
   Return a player city with the given city_id.
 */
city *find_city(player *pplayer, int city_id)
{
  if (pplayer) {
    return player_city_by_number(pplayer, city_id);
  } else {
    return idex_lookup_city(&wld, city_id);
  }
}

/**
   Return a player unit with the given unit_id.
 */
unit *find_unit(player *pplayer, int unit_id)
{
  if (pplayer) {
    return player_unit_by_number(pplayer, unit_id);
  } else {
    return idex_lookup_unit(&wld, unit_id);
  }
}

/**
   Return a unit that can transport ptype at a given ptile.
 */
unit *find_transport_unit(player *pplayer, unit_type *ptype, tile *ptile)
{
  struct unit *ptransport;
  struct unit *pvirt = unit_virtual_create(pplayer, nullptr, ptype, 0);
  unit_tile_set(pvirt, ptile);
  pvirt->homecity = 0;
  ptransport = transporter_for_unit(pvirt);
  unit_virtual_destroy(pvirt);
  return ptransport;
}

/**
   Return a unit type for given role or flag.
   (Prior to 2.6.0, this worked only for roles.)
 */
unit_type *find_role_unit_type(const char *role_name, player *pplayer)
{
  int role_or_flag = unit_role_id_by_name(role_name, fc_strcasecmp);

  if (!unit_role_id_is_valid(unit_role_id(role_or_flag))) {
    role_or_flag = unit_type_flag_id_by_name(role_name, fc_strcasecmp);
    if (!unit_type_flag_id_is_valid(unit_type_flag_id(role_or_flag))) {
      return nullptr;
    }
  }

  if (pplayer) {
    return best_role_unit_for_player(pplayer, role_or_flag);
  } else if (num_role_units(role_or_flag) > 0) {
    return get_role_unit(role_or_flag, 0);
  } else {
    return nullptr;
  }
}

/**
   Return the tile at the given native coordinates.
 */
tile *find_tile(int nat_x, int nat_y)
{
  return native_pos_to_tile(&(wld.map), nat_x, nat_y);
}

/**
   Return the tile at the given index.
 */
tile *find_tile_by_index(int tindex)
{
  return index_to_tile(&(wld.map), tindex);
}

} // namespace

void setup_lua_find(sol::state_view lua)
{
  auto find = lua["find"].get_or_create<sol::table>();
  find.set("city", find_city);
  find.set("player", player_by_number);
  find.set("unit", find_unit);
  find.set("transport_unit", find_transport_unit);
  find.set("role_unit_type", find_role_unit_type);
  find.set("tile", sol::overload(find_tile, find_tile_by_index));
  find.set("government",
           sol::overload(government_by_number, government_by_rule_name));
  find.set("nation_type",
           sol::overload(nation_by_number, nation_by_rule_name));
  find.set("action", sol::overload(action_by_number, action_by_rule_name));
  find.set("building_type",
           sol::overload(improvement_by_number, improvement_by_rule_name));
  find.set("unit_type",
           sol::overload(utype_by_number, unit_type_by_rule_name));
  find.set("terrain",
           sol::overload(terrain_by_number, terrain_by_rule_name));
  find.set("tech_type",
           sol::overload(advance_by_number, advance_by_rule_name));
}
