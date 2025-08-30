---@meta

-- SPDX-License-Identifier: GPL-3.0-or-later
-- SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
-- SPDX-FileCopyrightText: Freeciv Wiki contributors <https://freeciv.fandom.com/wiki/Lua_reference_manual?action=history>
-- SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

--  WARNING: do not attempt to change the name of the API functions.
--  They may be in use in Lua scripts in savefiles, so once released, the
--  name and signature cannot change shape even in new major versions of
--  Freeciv21, until the relevant save format version can no longer be loaded.
--  If you really like to change a function name, be sure to keep also the
--  old one running.

-- Usage references:
-- https://longturn.readthedocs.io/en/latest/Contributing/style-guide.html
-- https://luals.github.io/wiki/definition-files
-- https://luals.github.io/wiki/annotations/#documenting-types
-- https://taminomara.github.io/sphinx-lua-ls/index.html#autodoc-directives
-- https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-primer

--- Represents a nation in the game controlled by a specific player.
---
--- .. note::
---    In clients, functions that need full information about other players
---    consider technologies status of which for specific player is not known
---    to be unknown. Research information about players to which the client
---    has no direct or indirect embassy access is initialized with zeros and
---    nils. However, if embassy data source for some reason ceases to exist
---    during playing session, currently the latest data get stuck, see
---    `#2770 <https://github.com/longturn/freeciv21/issues/2770>`_
---
--- @class Player
--- @field name string The player's name.
--- @field nation Nation_Type The nation from the nation list.
--- @field is_alive boolean If the nation is currently considered alive based on the ruleset.
--- @field id int The unique ID for this player.
--- @field team Team The player's team.
Player = {}

--- 
--- @return int cities The number of cities currently owned by this player.
function Player:num_cities() end

--- 
--- @return int units The number of units currently owned by this player.
function Player:num_units() end

--- 
--- @param building Building_Type The wonder to check for.
--- @return boolean present True if the player has this wonder in any city.
function Player:has_wonder(building) end

--- 
--- @return int gold The amount of gold currently in this player's treasury.
function Player:gold() end

--- 
--- @param tech Tech_Type The advance to check for.
--- @return boolean known True if the player has this advance.
function Player:knows_tech(tech) end

--- 
--- @param player Player The other player to check against
--- @return boolean shares True if this player shares research with the given player
function Player:shares_research(tech) end

--- This will either be the name of this player or the name of their team if
--- :ref:`team_pooled_research <server-option-team-pooled-research>` is enabled.
---
--- @return string name The name of the research owner. 
function Player:research_rule_name() end

--- 
--- @return string name As :lua:func:`Player:research_rule_name` but translated.
function Player:research_name_translation() end

--- 
--- @return int culture The player's current culture score.
function Player:culture() end

--- .. list-table:: Player Flags
---    :header-rows: 1
---
---    * - Flag
---      - Description
---    * - "ai"
---      - Same as :lua:func:`~Player:ai_controlled`.
---    * - "ScenarioReserved"
---      - Not listed on game starting screen.
---
--- @param flag string The flag to check for.
--- @return boolean present True if the player has this flag.
function Player:has_flag() end

--- 
--- @return boolean human True if this nation is controlled by a human player.
function Player:is_human()
  return not self.has_flag(self, "AI");
end

--- 
--- @return boolean exists True if this nation (still) exists in the game.
function Player:exists()
  return true
end

--- Safe iteration over each :lua:class:`Unit` that belongs to a :lua:class:`Player`.
function Player:units_iterate()
  return safe_iterate_list(private.Player.unit_list_head(self))
end

--- Safe iteration over each :lua:class:`City` that belongs to a :lua:class:`Player`.
function Player:cities_iterate()
  return safe_iterate_list(private.Player.city_list_head(self))
end

--- Represents a Team of players that are locked together as allies.
---
--- @class Team
--- @field id int The unique ID for this team
--- @field name string The name of the team
Team = {}

--- Safe iteration over each :lua:class:`Player` in the team.
function Team:members_iterate() end

--- Represents a city on the world map.
---
--- @class City
--- @field name string The name of the city.
--- @field owner Player The current owner of the city.
--- @field original Player The original founder of the city.
--- @field id int The unique ID for the city.
--- @field size int The number of citizens in this city.
--- @field tile Tile The tile this city is on.
City = {}

--- 
--- @param building Building_Type The building type to check.
--- @return boolean present True if the given building type is in this city.
function City:has_building(building) end

--- 
--- @return int sq-radius The square radius of the city working area.
function City:map_sq_radius() end

--- Returns whether the city would produce partisans if conquered from player
--- inspirer, taking into account original owner, nationality, and the
--- :ref:`Inspire_Partisans <effect-inspire-partisans>` effect.
--- The default :lua:func:`_deflua_make_partisans_callback` handler treats the
--- return from this as boolean (greater than zero gives partisans), but a
--- custom handler and ruleset could give different behaviour with different
--- values of the 'Inspire_Partisans' effect.
---
--- @return int partisans Positive if it would inspire partisans.
function City:inspire_partisans(inspirer) end

--- 
--- @return int culture Current total culture score for this city.
function City:culture() end

--- .. note::
---    A city can be both happy and celebrating at the same time.
---
--- @return boolean happy True if the city has more happy citizens than content and no unhappy or angry citizens.
function City:is_happy() end

--- .. note::
---    A city can be both happy and celebrating at the same time.
---
--- @return boolean celebrating True if the city is currently celebrating due to being happy for more than one turn.
function City:is_celebrating() end

--- 
--- @return boolean unhappy True if the city has fewer happy citizens than unhappy, with angry citizens counting as two unhappy.
function City:is_unhappy() end

--- 
--- @return boolean gov-center True if the city is a center of government for the purpose of distance to government center calculations.
function City:is_gov_center() end

--- 
--- @return boolean capital True if the city is the nation's capital.
function City:is_capital() end

--- 
--- @return boolean exists True if the city (still) exists.
function City:exists() end

--- Represents a player's current connection to the game server.
---
--- @class Connection
--- @field id int The unique ID of the connection.
Connection = {}

--- Represents a unit on the world map.
---
--- @class Unit
--- @field utype Unit_Type The type of unit this is.
--- @field owner Player The player who owns the unit.
--- @field id int The unique ID for this unit.
--- @field tile Tile The tile the unit is currently on.
--- @field hp int The number of hitpoints the unit currently has left.
Unit = {}

--- 
--- @return Unit unit The unit that this one is loaded on, or nil.
function Unit:transporter() end

--- 
--- @return boolean settleable True, if a settler unit can build a city here.
function Unit:is_on_possible_city_tile() end

--- 
--- @return Direction direction The current unit orientation.
function Unit:facing() end

--- .. attention::
---    Warning: If unit is going to be removed, you must use 
---    :lua:func:`Unit:tile_link_text` instead.
---
--- @return string link-text Link string fragment to add to messages sent to client.
function Unit:link_text() end

--- 
--- @return string link-text Link string fragment to add to messages sent to client. 
function Unit:tile_link_text() end

--- 
--- @return boolean exists True, if the unit (still) exists.
function Unit:exists()
  return true
end

--- 
--- @return City home The home city of this unit, or nil.
function Unit:get_homecity()
  return find.city(self.owner, self.homecity)
end

--- 
--- @return Number hp-pct The percentage of the unit's maximum hitpoints that remain.
function Unit:hp_pct()
  return self.hp * 100 / self.utype.hp
end

--- Safe iteration over the units transported by this unit. Only iterates over
--- units directly transported by this one, if any. Does not include nested
--- units.
function Unit:cargo_iterate()
  return safe_iterate_list(private.Unit.cargo_list_head(self))
end

--- Represents a tile on the world map.
---
--- .. note::
---    The native coordinate system represents the internal coordinates for a
---    tile on the server. The map coordinate system is how it appears in the
---    client based on the chosen topology. You can see both in the tile info
---    panel.
---
--- @class Tile
--- @field terrain Terrain The terrain type of this tile.
--- @field owner Player The player who owns this tile, or nil.
--- @field id int The unique ID of this tile.
--- @field continent int The ID of the continent this tile is on.
--- @field nat_x int The X position of this tile on the native coordinate system.
--- @field nat_y int The Y position of this tile on the native coordinate system.
--- @field x int The X position of this tile on the map coordinate system.
--- @field y int The Y position of this tile on the map coordinate system.
--- @field link_text string Link string fragment to add to messages sent to client.
Tile = {}

--- 
--- @return City city The city that is on this tile, or nil.
function Tile:city() end

--- 
--- @param center boolean True, if the center tile should be checked also.
--- @return boolean exists True if there is a city within the maximum radius a city map can ever have in Freeciv21 (not necessarily possible in the current ruleset) -- currently within 5 tiles. 
function Tile:city_exists_within_max_city_map(center) end

--- 
--- @param name boolean The ruleset name of any type of terrain extra.
--- @return boolean present True, if the extra is present on this tile.
function Tile:has_extra(name) end

--- 
--- @param name boolean The ruleset name of any base-type extra.
--- @return boolean True, if the base is present on this tile.
function Tile:has_base(name) end

--- 
--- @param name boolean The ruleset name of any road-type extra.
--- @return boolean True, if the road-type is present on this tile.
function Tile:has_road(name) end

--- The primary tile resource persists even if the terrain is changed to an
--- incompatible type, and can be recovered if the terrain is changed back.
--- This will report the name of the hidden resource in this case. Whereas
--- :lua:func:`~Tile:has_extra` will only report whether the resource is
--- currently visible.
--- @return string name The ruleset name of the primary tile resource, or nil.
function Tile:primary_resource_name() end

--- 
--- @param against Player The player to check against.
--- @return boolean enemy True, if the given player has enemies on this tile.
function Tile:is_enemy(against) end

--- 
--- @return int units The total number of units on this tile.
function Tile:num_units() end

--- 
--- @param other Tile A different tile to check against.
--- @return int dist The squared distance between these two tiles.
function Tile:sq_distance(other) end

--- Safe iteration over each :lua:class:`Unit` on the tile.
function Tile:units_iterate()
  return safe_iterate_list(private.Tile.unit_list_head(self))
end

--- This function returns an iterator that yields tiles in a square pattern,
--- expanding outward from the current tile up to the specified radius.
---
--- In a square topology, square_iterate(1) returns 9 tiles.
---
--- @param number radius The distance from the current tile to iterate over.
--- @return function iterator An iterator function.
function Tile:square_iterate(radius) end

--- This function returns an iterator that yields tiles in a circular pattern,
--- expanding outward from the current tile up to the specified squared radius.
---
--- In a square topology, circle_iterate(1) returns 5 tiles.
---
--- @param number sq_radius The squared distance from the current tile to iterate over.
--- @return function iterator An iterator function.
function Tile:circle_iterate(sq_radius) end


--- Represents a government type for this ruleset.
---
--- @class Government
--- @field id int The unique ID of this government type.
Government = {}

--- 
--- @return string name The name given by the ruleset.
function Government:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Government:name_translation() end

--- Represents a nation type for this ruleset.
---
--- @class Nation_Type
--- @field id int The unique ID of this nation type.
Nation_Type = {}

--- 
--- @return string name The name given by the ruleset.
function Nation_Type:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Nation_Type:name_translation() end

--- 
--- @return string name The translation of the name in plural form.
function Nation_Type:plural_translation() end

--- Represents a type of building that can be constructed in a city in this
--- ruleset.
---
--- @class Building_Type
--- @field build_cost int The number of shields required to construct.
--- @field id int The unique ID of the building type.
Building_Type = {}

--- 
--- @return boolean wonder True, if this can be built once in the world or once per nation.
function Building_Type:is_wonder() end

--- 
--- @return boolean great-wonder True, if this can be built once in the world.
function Building_Type:is_great_wonder() end

--- 
--- @return boolean small-wonder True, if this can be built once per nation.
function Building_Type:is_small_wonder() end

--- 
--- @return boolean improvement True, if this can be built multiple times by a nation.
function Building_Type:is_improvement() end

--- 
--- @return string name The name given by the ruleset.
function Building_Type:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Building_Type:name_translation() end

--- 
--- @return int cost The number of shields needed to construct this building.
function Building_Type:build_shield_cost()
  return self.build_cost
end

--- Represents a type of unit available in this ruleset.
---
--- @class Unit_Type
--- @field build_cost int The number of shields required to make.
--- @field pop_cost int The number of citizens consumed to make.
--- @field hp int The maximum number of hitpoints.
--- @field id int The unique ID of the unit type.
Unit_Type = {}

--- 
--- @param flag_name string The name of the unit type flag to check for.
--- @return boolean present True, if this unit type has the given flag.
function Unit_Type:has_flag(flag_name) end

--- 
--- @param role_name string The name of the unit type role to check for.
--- @return boolean present True, if this unit type has the given role.
function Unit_Type:has_role(role_name) end

--- 
--- @return string name The name given by the ruleset.
function Unit_Type:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Unit_Type:name_translation() end

--- 
--- @param tile Tile The tile to check.
--- @return boolean safe True, if the unit type can normally exist on the given tile.
function Unit_Type:can_exist_at_tile(tile) end

--- 
--- @return int cost The total shield cost to build this unit type. Same as :lua:obj:`Unit_Type.build_cost`.
function Unit_Type:build_shield_cost()
  return self.build_cost
end

--- Represents a technological or cultural advance that a nation can research.
---
--- @class Tech_Type
--- @field id int The unique ID of the advance.
Tech_Type = {}

--- 
--- @return string name The name given by the ruleset.
function Tech_Type:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Tech_Type:name_translation() end

--- Represents a type of terrain that may appear on the map in this ruleset.
---
--- @class Terrain
--- @field id int The unique ID of the terrain type.
Terrain = {}

--- 
--- @return string name The name given by the ruleset.
function Terrain:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Terrain:name_translation() end

--- 
--- @return string class The localised translation of the name.
function Terrain:class_name() end

--- Represents a type of disaster that can occur for a city in this ruleset.
---
--- @class Disaster
--- @field id int The unique ID of the disaster type.
Disaster = {}

--- 
--- @return string name The name given by the ruleset.
function Disaster:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Disaster:name_translation() end

--- Represents an achievement that a player can unlock in this ruleset.
---
--- @class Achievement
--- @field id int The unique ID of the achievement.
Achievement = {}

--- 
--- @return string name The name given by the ruleset.
function Achievement:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Achievement:name_translation() end

--- Represents a type of action that a unit can perform in this ruleset.
---
--- @class Action
--- @field id int The unique ID of the action type.
Action = {}

--- 
--- @return string name The name given by the ruleset.
function Action:rule_name() end

--- 
--- @return string name The localised translation of the name.
function Action:name_translation() end

--- This module is used for game related information. 
---
--- !doctype table
--- @class game
game = {}

--- 
--- @return int turn The current turn number, starting from 1.
function game.current_turn() end

--- 
--- @return int year The current game year.
function game.current_year() end

--- 
--- @return int year-fragment The current game year fragment. E.g. 8 = August.
function game.current_fragment() end

--- 
--- @return string year-text Textual representation of current calendar time.
function game.current_year_text() end

--- 
--- @return string ruleset-name Unique name of the ruleset.
function game.rulesetdir() end

--- 
--- @return string ruleset-name Human readable name of the ruleset.
function game.ruleset_name() end

--- Functions in this module are used to acquire objects for further
--- manipulation, given various kinds of identifying information. Functions are
--- overloaded so that a given object can be identified in several ways. 
---
--- !doctype table
--- @class find
find = {}

--- 
--- @param player_id_or_name int|string The unique ID or name of the player to search for.
--- @return Player player The player with the given ID or name if they exist, or nil.
function find.player(player_id_or_name) end

--- 
--- @param team_id_or_name int|string The unique ID or name of the player to search for.
--- @return Team team The team with the given ID or name if they exist, or nil.
function find.team(team_id_or_name) end

--- Safe iteration over each :lua:class:`Team` in the game.
function find.teams_iterate() end

--- 
--- @param player Player The player who owns the city, or nil for any player
--- @param city_id int The unique ID for the city to search for
--- @return City city The city if it exists and belongs to player, or nil
function find.city(player, city_id) end

--- 
--- @param player Player The player who owns the unit, or nil for any player
--- @param unit_id int The unique ID for the unit to search for
--- @return Unit unit The unit if it exists and belongs to player, or nil
function find.unit(player, unit_id) end

--- Looks for an existing unit that can transport unit_type at tile. Intended
--- to be used with create_unit_full, to spawn it directly into the hold of the
--- transport.
---
--- @param player Player The player who owns the transport unit
--- @param unit_type Unit_Type The unit type to search for a transport for
--- @param tile Tile The tile to search in
--- @return Unit unit The transport unit found, or nil
function find.transport_unit(player, unit_type, tile) end

--- Finds a tile based on natural coordinates.
--- @param nat_x int The natural x-coordinate.
--- @param nat_y int The natural y-coordinate.
--- @return Tile tile The found tile.
function find.tile(nat_x, nat_y) end

--- Finds a tile by its index.
--- @param tindex int The index of the tile.
--- @return Tile tile The found tile.
function find.tile(tindex) end

--- Finds a government by its name.
--- @param name string The ruleset name of the government.
--- @return Government government The found government.
function find.government(name) end

--- Finds a government by its ID.
--- @param government_id int The ID of the government.
--- @return Government government The found government.
function find.government(government_id) end

--- Finds a nation type by its name.
--- @param name string The ruleset name of the nation type.
--- @return Nation_Type nation The found nation type.
function find.nation_type(name) end

--- Finds a nation type by its ID.
--- @param nation_type_id int The ID of the nation type.
--- @return Nation_Type nation The found nation type.
function find.nation_type(nation_type_id) end

--- Finds an action by its rule name.
--- @param name string The ruleset name of the action.
--- @return Action action The found action.
function find.action(name) end

--- Finds an action by its ID. For use in loops etc. 
---
--- .. attention::
---    An action's ID may change after saving and reloading the game.
---
--- @param action_type_id int The ID of the action.
--- @return Action action The found action.
function find.action(action_type_id) end

--- Finds a building type by its name.
--- @param name string The ruleset name of the building type.
--- @return Building_Type building The found building type.
function find.building_type(name) end

--- Finds a building type by its ID.
--- @param building_type_id int The ID of the building type.
--- @return Building_Type building The found building type.
function find.building_type(building_type_id) end

--- Finds a unit type by its name.
--- @param name string The ruleset name of the unit type.
--- @return Unit_Type unit-type The found unit type.
function find.unit_type(name) end

--- Finds a unit type by its ID.
--- @param unit_type_id int The ID of the unit type.
--- @return Unit_Type unit_type The found unit type.
function find.unit_type(unit_type_id) end

--- If player is not nil, role_unit_type returns the best suitable unit this
--- player can build. If player is nil, returns first unit for that role.
--- @param role_name string The name of the role or unit type flag.
--- @param pplayer Player The owner of the unit, or nil for any player.
--- @return Unit_Type unit-type The found role unit type.
function find.role_unit_type(role_name, pplayer) end

--- Finds a tech type by its name.
--- @param name string The ruleset name of the tech type.
--- @return Tech_Type tech The found tech type.
function find.tech_type(name) end

--- Finds a tech type by its ID.
--- @param tech_type_id int The ID of the tech type.
--- @return Tech_Type tech The found tech type.
function find.tech_type(tech_type_id) end

--- Finds a terrain by its name.
--- @param name string The ruleset name of the terrain.
--- @return Terrain terrain The found terrain.
function find.terrain(name) end

--- Finds a terrain by its ID.
--- @param terrain_id int The ID of the terrain.
--- @return Terrain terrain The found terrain.
function find.terrain(terrain_id) end


--- Calculate the current value of a 
--- :ref:`ruleset effect <modding-rulesets-effects>`. Effect names are the same
--- as in rulesets, e.g., "``Upkeep_Free``". 
---
--- !doctype table
--- @class effects
effects = {}

--- Calculates a world effect bonus.
--- @param effect_type Effect_Type The type of effect to calculate.
--- @return int value The value of the world effect.
function effects.world_bonus(effect_type) end

--- Calculates a player effect bonus.
--- @param pplayer Player The player to calculate the bonus for.
--- @param effect_type Effect_Type The type of effect to calculate.
--- @return int value The value of the player effect.
function effects.player_bonus(pplayer, effect_type) end

--- Calculates a city effect bonus.
--- @param pcity City The city to calculate the bonus for.
--- @param effect_type Effect_Type The type of effect to calculate.
--- @return int value The value of the city effect.
function effects.city_bonus(pcity, effect_type) end


--- !doctype table
--- @class direction
direction = {}

--- Turns a string like "north", "southwest", etc., into a Direction object.
--- @param str string The string representation of the direction.
--- @return Direction direction The corresponding Direction object.
function direction.str2dir(str) end

--- Returns the next counterclockwise direction in the current topology.
--- @param direction Direction The current Direction.
--- @return Direction direction The next counterclockwise Direction.
function direction.next_ccw(direction) end

--- Returns the next clockwise direction in the current topology.
--- @param direction Direction The current Direction.
--- @return Direction direction The next clockwise Direction.
function direction.next_cw(direction) end

--- Returns the opposite direction.
--- @param direction Direction The current Direction.
--- @return Direction direction The opposite Direction.
function direction.opposite(direction) end


--- Iterate over each :lua:obj:`Player` in the game.
function players_iterate()
  return index_iterate(find.player)
end

--- Iterate over each :lua:obj:`Tile` on the map.
function whole_map_iterate()
  return index_iterate(find.tile)
end

