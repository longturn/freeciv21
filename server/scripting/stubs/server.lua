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

--- The server module is for interacting with the server, and getting
--- configuration details and settings.
---
--- !doctype table
--- @class server
server = {}

--- Instructs the server to save the game. As with the
--- :ref:`/save <server-command-save>` command, if filename is empty, the
--- server chooses an appropriate filename.
---
--- @param filename string The filename to use, or nil to use a default.
function server.save(filename) end

--- 
--- @return boolean started True, if the game has been started.
function server.started() end

--- Same as :lua:obj:`Player:civilization_score`
--- @param player Player
--- @return int score
function server.civilization_score(player) end

--- Request player's client to play certain track.
--- @param player Player The player who should play the track.
--- @param tag string The tag of the track to play.
--- @return boolean played True, if it played.
function server.play_music(player, tag) end

--- The server settings module is for querying and modifying server settings.
---
--- !doctype table
--- @class server.setting
server.setting = {}

--- 
--- @param String name The name of the :ref:`server setting <server-options>`.
--- @return String value The value of a server setting as a string. 
function server.setting.get(name) end


--- Send event notifications to players. 
---
--- See the :lua:obj:`E` for a list of possible event
--- types.
---
--- !doctype table
--- @class notify
notify = {}

--- Send all players a message with type :lua:obj:`E.SCRIPT`.
---
--- @param message String A :lua:obj:`string.format`-style message.
--- @param ... any A set of values to insert into the string template.
function notify.all(message, ...) end

--- Send a player a message with type :lua:obj:`E.SCRIPT`.
---
--- @param player Player The player to send the event message to.
--- @param message String A :lua:obj:`string.format`-style message.
--- @param ... any A set of values to insert into the string template.
function notify.player(player, message, ...) end

--- Send a specific player or all players (if player is nil) a message with
--- an arbitrary type, optionally drawing attention to a particular tile.
---
--- @param player Player The player to send the event message to, or nil for all players.
--- @param tile Tile The tile to highlight, or nil for no tile.
--- @param event E The event category.
--- @param message String A :lua:obj:`string.format`-style message.
--- @param ... any A set of values to insert into the string template.
function notify.event(player, tile, event, message, ...) end

--- Send a message to all players who have an embassy with the given player.
---
--- @param player Player The player to send the event message about.
--- @param tile Tile The tile to highlight, or nil for no tile.
--- @param event E The event category.
--- @param message String A :lua:obj:`string.format`-style message.
--- @param ... any A set of values to insert into the string template.
function notify.embassies(player, ptile, event, message, ...) end

--- Send all players sharing research with specific player a message with an
--- arbitrary type.
---
--- Setting selfmsg to false can be useful if you want to send different
--- message to one actually getting the tech for the team.
---
--- @param player Player The player to send the event message about.
--- @param selfmsg boolean True, if the player should also receive the event.
--- @param event E The event category.
--- @param message String A :lua:obj:`string.format`-style message.
--- @param ... any A set of values to insert into the string template.
function notify.research(player, selfmsg, event, message, ...) end

--- Send a message to all players who have an embassy with someone sharing
--- research with player, that is; to everyone who sees someone gaining a tech
--- when player gains it.
---
--- @param player Player The player to send the event message about.
--- @param event E The event category.
--- @param message String A :lua:obj:`string.format`-style message.
--- @param ... any A set of values to insert into the string template.
function notify.research_embassies(player, event, message, ...) end

--- The edit module contains all of the functions for modifying game state.
---
--- !doctype table
--- @class edit
edit = {}

--- Creates a unit.
--- @param player Player The player who will own the unit.
--- @param tile Tile The tile where the unit will be created.
--- @param unit_type Unit_Type The type of the unit.
--- @param veteran_level int The veteran level of the unit.
--- @param home_city City The city that the unit will belong to, or nil for unhomed.
--- @param moves_left int The initial number of moves left for the unit. -1 for max.
--- @return Unit unit The created unit.
function edit.create_unit(player, tile, unit_type, veteran_level, home_city, moves_left) end

--- Creates a unit with full parameters.
--- @param player Player The player who will own the unit.
--- @param tile Tile The tile where the unit is created.
--- @param unit_type Unit_Type The type of the unit.
--- @param veteran_level int The veteran level of the unit.
--- @param home_city City The city that the unit belongs to, or nil for unhomed.
--- @param moves_left int The number of moves left for the unit. -1 for max.
--- @param hp_left int The current hit points of the unit. -1 for max.
--- @param transport Unit The transport unit it starts in, or nil for not in a transport.
--- @return Unit unit The created unit.
function edit.create_unit_full(player, tile, unit_type, veteran_level, home_city, moves_left, hp_left, transport) end

--- Teleports a unit to a destination tile without consuming move points. If it
--- lands in inhospitable terrain or among non-friendly units, it dies.
--- @param unit Unit The unit to be teleported.
--- @param dest Tile The destination tile.
--- @return bool True if the teleportation was successful, false if it died.
function edit.unit_teleport(unit, dest) end

--- Kills a unit with a :ref:`reason <script-unit-loss-reasons>`. This affects
--- how scoring is applied for the death.
--- @param unit Unit The unit to be killed.
--- @param reason string The reason for killing the unit.
--- @param killer Player The player who killed the unit, or nil.
function edit.unit_kill(unit, reason, killer) end

--- Damages a unit's HP. Automatically killing the unit if it drops to zero or
--- below. See :lua:obj:`edit.unit_kill` for loss reasons.
--- @param unit Unit The unit to be damaged.
--- @param amount int The amount of damage to apply.
--- @param reason string The reason for the damage. See :ref:`reason <script-unit-loss-reasons>`
--- @param killer Player The player responsible for the damage, or nil.
--- @return bool surived True if the unit survived.
function edit.unit_damage_hp(unit, amount, reason, killer) end

--- Recovers a unit's HP, capped at the max.
--- @param unit Unit The unit to recover HP for.
--- @param amount int The amount of HP to recover.
function edit.unit_recover_hp(unit, amount) end

--- Sets a unit's HP. Capped between 1 and the max for the unit type.
--- @param unit Unit The unit whose HP is to be set.
--- @param amount int The new HP amount.
function edit.unit_set_hp(unit, amount) end

--- .. note::
---    If this is set above the required number of work points for the current
---    activity, this will complete the activity immediately.
---
--- See :lua:obj:`Unit.activity_count_set` and :lua:obj:`Unit.activity_count_add`
---
--- @param unit Unit The unit to modify.
--- @param work_points int The total number of work points that have been put into the current activity.
function edit.unit_activity_count_set(unit, work_points) end

--- See :lua:obj:`Unit.nationality_set`
---
--- @param unit Unit The unit to modify.
--- @param nationality Player The nationality of the people in the unit.
function edit.unit_nationality_set(unit, nationality) end

--- .. note::
---    This may exceed the max :lua:obj:`Unit_Type.move_rate`,
---    but cannot be lower then zero. 
---
--- See :lua:obj:`Unit.moves_left_set` and :lua:obj:`Unit.moves_left_add`
---
--- @param unit Unit The unit to modify.
--- @param moves_left int The number of move fragments remaining.
function edit.unit_moves_left_set(unit, moves_left) end

--- .. note::
---    This cannot be less than zero or exceed 
---    :lua:obj:`Unit_Type.max_veteran_level`.
---
--- .. note::
---    This does not automatically :lua:obj:`notify` the :lua:obj:`Player`
---    who owns the unit.
---
--- See :lua:obj:`Unit.veteran_level_set` and :lua:obj:`Unit.veteran_level_add`
---
--- @param unit Unit The unit to modify.
--- @param veteran int The veterancy level of the unit.
function edit.unit_veteran_level_set(unit, veteran) end

--- .. note::
---    This cannot exceed the max :lua:obj:`Unit_Type.fuel`, and cannot be
---    lower then zero.
---
--- See :lua:obj:`Unit.fuel_set` and :lua:obj:`Unit.fuel_add` 
---
--- @param unit Unit The unit to modify.
--- @param fuel int The amount of fuel remaining.
function edit.unit_fuel_set(unit, fuel) end

--- Changes the terrain of a tile. If the terrain change would destroy a city,
--- the change fails. You need to :lua:obj:`edit.remove_city` first to change
--- it anyway.
--- @param tile Tile The tile whose terrain is to be changed.
--- @param terrain Terrain The new terrain type.
--- @return bool success True if the terrain change was successful, false otherwise.
function edit.change_terrain(tile, terrain) end

--- Creates a new city at size 1.
--- @param player Player The player who owns the city.
--- @param tile Tile The tile where the city is created.
--- @param name string The name of the city, or nil for random.
function edit.create_city(player, tile, name) end

--- Resizes a city. The reason is a user-defined value that will show up in
--- :lua:obj:`~Event.city_size_change` signal events.
--- @param city City The city to resize.
--- @param size int The new size of the city.
--- @param reason string The reason for resizing the city.
function edit.resize_city(city, size, reason) end

--- Removes a city.
--- @param city City The city to be removed.
function edit.remove_city(city) end

--- Creates an owned extra on a tile. This will claim ownership of any other
--- ownable extras on the tile too.
--- @param tile Tile The tile where the extra is created.
--- @param name string The name of the extra.
--- @param player Player The player who owns the extra.
function edit.create_owned_extra(tile, name, player) end

--- Creates an extra on a tile. Note that this does not affect the primary
--- resource of a tile. If you want to change the primary resource, use 
--- :lua:obj:`edit.set_primary_resource` instead.
--- @param tile Tile The tile where the extra is created.
--- @param name string The name of the extra.
function edit.create_extra(tile, name) end

--- Removes an extra from a tile. Note that this does not affect the primary
--- resource of a tile. If you want to change the primary resource, use 
--- :lua:obj:`edit.set_primary_resource` instead.
--- @param tile Tile The tile from which the extra is removed.
--- @param name string The name of the extra to remove.
function edit.remove_extra(tile, name) end

--- Changes the primary resource for a tile. The primary resource persists even
--- if the terrain changes to an incompatible type, and can be recovered if the
--- terrain is changed to a compatible type again. Unlike resources created by
--- :lua:obj:`~edit.create_extra` which disappear forever.
--- @param tile Tile The tile to set the primary resource for.
--- @param name string The name of the resource, or nil to remove.
function edit.tile_primary_resource_set(tile, name) end

--- Sets a textual label for a tile.
--- @param tile Tile The tile to label.
--- @param label string The label to set for the tile.
function edit.tile_set_label(tile, label) end

--- Creates a player. The new player has no units or cities.
--- @param username string The username of the player.
--- @param nation Nation_Type The nation type of the player.
--- @param ai string The AI type for the player, or nil for human.
--- @return Player player The created player.
function edit.create_player(username, nation, ai) end

--- Adds the given amount to the player's treasury.
--- @param player Player The player whose gold amount is to be changed.
--- @param amount int The amount to add (can be positive or negative).
function edit.change_gold(player, amount) end

--- Gives a technology to a player. 
---
--- Cost is a percentage of the tech's cost to be deducted from the bulb store,
--- or '-1' for normal :ref:`freecost <server-option-free-cost>`, 
--- '-2' for :ref:`conquercost <server-option-conquer-cost>`, 
--- '-3' for :ref:`diplbulbcost <server-option-diplomacy-bulb-cost>`
---
--- The reason is a user-defined value that is passed to the
--- :lua:obj:`~Events.tech_researched` signal event.
---
--- @param player Player The player receiving the technology.
--- @param tech Tech_Type The technology to give, or nil for random.
--- @param cost int The cost to apply.
--- @param notify bool True to notify all parties as normal.
--- @param reason string The reason for giving the technology.
--- @return Tech_Type tech The technology that was given, or nil if they already had the tech.
function edit.give_tech(player, tech, cost, notify, reason) end

--- Modifies an AI player's personality trait value. This is a modifier for the
--- trait value that the player was created with. Subsequent calls override the
--- modifier, they are not additive.
---
--- The trait_name can be one of: "Expansionist", "Trader", "Aggressive",
--- or "Builder".
---
--- @param player Player The AI player whose trait is to be modified.
--- @param trait_name string The name of the trait to modify.
--- @param mod int The modification value to apply to the trait.
--- @return bool success True if the modification was successful, false otherwise.
function edit.trait_mod(player, trait_name, mod) end

--- Unleashes barbarians on a tile.
--- @param tile Tile The tile where barbarians are unleashed.
--- @return bool survived False if any units on the tile were killed (or would have been killed, if there were no units). 
function edit.unleash_barbarians(tile) end

--- Places partisans in a radius around a tile.
--- @param tile Tile The tile where partisans are placed near.
--- @param player Player The player who owns the partisans.
--- @param count int The number of partisans to place.
--- @param sq_radius int The square radius around the tile to place partisans.
function edit.place_partisans(tile, player, count, sq_radius) end

--- Enum for climate change types.
---
--- .. list-table:: Climate Change Types
---    :header-rows: 1
---
---    * - Enum
---      - Description
---    * - GLOBAL_WARMING
---      - Global Warming is caused by the accumulation of pollution.
---    * - NUCLEAR_WINTER
---      - Nuclear Winter is caused by the accumulation of nuclear fallout.
edit.climate_change_type = {
    GLOBAL_WARMING = "GLOBAL_WARMING",
    NUCLEAR_WINTER = "NUCLEAR_WINTER"
}

--- Applies a wave of global climate change effects. Unlike regular climate
--- change, this does not automatically generate any messages or affect the
--- counters for pollution/fallout-related climate change. It will trigger even
--- if :ref:`global_warming <server-option-global-warming>` or 
--- :ref:`nuclear_winter <server-option-nuclear-winter>` options are disabled.
---
--- @param cctype edit.climate_change_type The type of climate change to apply.
--- @param effect int The magnitude of the effect (approximately the number of tiles affected.)
function edit.climate_change(cctype, effect) end

--- Possibly throw a player into civil war. If the chance is not met, nothing
--- happens. If the chance is met, civil war happens as usual, with appropriate
--- messages sent to the players.
---
--- Probability is the percentage chance of the civil war occurring (use 100
--- for certainty), or if it is zero, the normal calculation is used (affected
--- by government, happiness, etc).
---
--- Takes no account of the :ref:`server-option-civil-war-size` server option
--- (but the player must have at least two cities).
---
--- @param player Player The player who may experience a civil war.
--- @param probability int The probability of civil war occurring.
--- @return Player rebel The new rebel AI player, or nil if it failed.
function edit.civil_war(player, probability) end

--- Change unit orientation to face in direction. 
--- @param unit Unit The unit to change the orientation of.
--- @param direction Direction The direction in which the unit will face.
function edit.unit_turn(unit, direction) end

--- Marks a player as victorious and ends the game.
--- @param player Player The player to mark as the winner.
function edit.player_victory(player) end

--- Moves a unit to a new tile and deducts the cost from the units move points.
--- If the tile is inhospitable terrain, or there are non-friendly units
--- present, the unit dies. Very few checks are applied. Be careful!
--- @param unit Unit The unit to be moved.
--- @param move_to Tile The tile to move the unit to.
--- @param move_cost int The cost of the move (in move fragments).
--- @return bool survived True if the move was successful, false it died.
function edit.unit_move(unit, move_to, move_cost) end

--- Prohibits the unit from moving from the spot it is at.
--- @param unit Unit The unit to disallow movement for.
function edit.movement_disallow(unit) end

--- Allows movement for a unit again following a 
--- :lua:obj:`edit.movement_disallow` call.
--- @param unit Unit The unit to allow movement for.
function edit.movement_allow(unit) end

--- Adds history a.k.a. culture to a city. This affects civilisation score.
--- @param city City The city to which history points are to be added.
--- @param amount int The amount of history points to add.
function edit.add_city_history(city, amount) end

--- Adds history :term:`a.k.a.` culture to a player. This affects civilisation score.
--- @param player Player The player to which history points are to be added.
--- @param amount int The amount of history points to add.
function edit.add_player_history(player, amount) end

--- See :lua:obj:`edit.create_unit`
--- @param tile Tile
--- @param unit_type Unit_Type
--- @param veteran_level int
--- @param home_city City
--- @param moves_left int
--- @return Unit unit
function Player:create_unit(tile, utype, veteran_level, homecity, moves_left)
  return edit.create_unit(self, tile, utype, veteran_level, homecity,
                          moves_left)
end

--- See :lua:obj:`edit.create_unit_full`
--- @param tile Tile
--- @param unit_type Unit_Type
--- @param veteran_level int
--- @param home_city City
--- @param moves_left int
--- @param hp_left int
--- @param transport Unit
--- @return Unit unit
function Player:create_unit_full(tile, utype, veteran_level, homecity,
                                 moves_left, hp_left, ptransport)
  return edit.create_unit_full(self, tile, utype, veteran_level, homecity,
                               moves_left, hp_left, ptransport)
end

--- 
--- @return int score The player's current score.
function Player:civilization_score()
  return server.civilization_score(self)
end

--- See :lua:obj:`edit.create_city`
--- @param tile Tile
--- @param name string
function Player:create_city(tile, name)
  edit.create_city(self, tile, name)
end

--- See :lua:obj:`edit.change_gold`
--- @param amount int
function Player:change_gold(amount)
  edit.change_gold(self, amount)
end

--- See :lua:obj:`edit.give_tech`
--- @param tech Tech_Type
--- @param cost int
--- @param notify bool
--- @param reason string
--- @return Tech_Type tech
function Player:give_tech(tech, cost, notify, reason)
  return edit.give_tech(self, tech, cost, notify, reason)
end

--- See :lua:obj:`edit.trait_mod`
--- @param trait_name string
--- @param mod int
function Player:trait_mod(trait, mod)
  return edit.trait_mod(self, trait, mod)
end

--- See :lua:obj:`edit.civil_war`
--- @param probability int
--- @return Player rebel
function Player:civil_war(probability)
  return edit.civil_war(self, probability)
end

--- See :lua:obj:`edit.player_victory`
function Player:victory()
  edit.player_victory(self)
end

--- See :lua:obj:`edit.add_player_history`
--- @param amount int
function Player:add_history(amount)
  edit.add_player_history(self, amount)
end

--- See :lua:obj:`edit.resize_city`
--- @param size int
--- @param reason string
function City:resize(size, reason)
  edit.resize_city(self, size, reason)
end

--- See :lua:obj:`edit.remove_city`
function City:remove()
  edit.remove_city(self)
end

--- See :lua:obj:`edit.add_city_history`
--- @param amount int
function City:add_history(amount)
  edit.add_city_history(self, amount)
end

--- 
--- @class Unit
Unit = {}

---
--- @return int turn_number The turn that the unit was first created.
function Unit:birth_turn() end

--- See :lua:obj:`edit.unit_teleport`
--- @param dest Tile
function Unit:teleport(dest)
  return edit.unit_teleport(self, dest)
end

--- See :lua:obj:`edit.unit_turn`
--- @param direction Direction
function Unit:turn(direction)
  edit.unit_turn(self, direction)
end

--- See :lua:obj:`edit.unit_kill`
--- @param reason string
--- @param killer Player
function Unit:kill(reason, killer)
  edit.unit_kill(self, reason, killer)
end

--- See :lua:obj:`edit.unit_damage_hp`
--- @param amount int
--- @param reason string
--- @param killer Player
function Unit:damage_hp(amount, reason, killer)
  return edit.unit_damage_hp(self, amount, reason, killer)
end

--- Damages a unit by a percentage of its max hitpoints.
--- See :lua:obj:`edit.unit_damage_hp`
--- @param amount int
--- @param reason string
--- @param killer Player
function Unit:damage_hp_pct(amount, reason, killer)
  local damage = math.ceil(self.utype.hp * amount / 100)
  return self:damage_hp(damage, reason, killer)
end

--- See :lua:obj:`edit.unit_recover_hp`
--- @param amount int
function Unit:recover_hp(amount)
  edit.unit_recover_hp(self, amount)
end

--- The unit recovers by a percentage of its max hitpoints.
--- See :lua:obj:`edit.unit_recover_hp`
--- @param amount int
function Unit:recover_hp_pct(amount)
  local recover = math.ceil(self.utype.hp * amount / 100)
  return self:recover_hp(recover)
end

--- See :lua:obj:`edit.unit_set_hp`
--- @param amount int
function Unit:set_hp(amount)
  edit.unit_set_hp(self, amount)
end

--- The unit is set to a percentage of its max hitpoints.
--- See :lua:obj:`edit.unit_set_hp`
--- @param amount int
function Unit:set_hp_pct(amount)
--- @param amount int
  local total_hp = math.ceil(self.utype.hp * amount / 100)
  edit.unit_set_hitpoints(self, total_hp)
end

--- See :lua:obj:`edit.unit_move`
--- @param moveto Tile
--- @param movecost int
function Unit:move(moveto, movecost)
  return edit.unit_move(self, moveto, movecost)
end

--- See :lua:obj:`edit.movement_disallow`
function Unit:movement_disallow()
  edit.movement_disallow(self)
end

--- See :lua:obj:`edit.movement_allow`
function Unit:movement_allow()
  edit.movement_allow(self)
end

--- .. note::
---    If this is set above the required number of work points for the current
---    activity, it will complete immediately.
---
--- @param work_points int The total number of work points that have been put into the current activity.
function Unit:activity_count_set(work_points) end

--- .. note::
---    If this is set above the required number of work points for the current
---    activity, it will complete immediately.
---
--- @param work_points int The number of work points to add to the current activity.
function Unit:activity_count_add(work_points) end

---
--- @param nationality Player The nationality of the people in the unit.
function Unit:nationality_set(nationality) end

--- .. note::
---    This may exceed the max for the :lua:obj:`Unit_Type.move_rate` type,
---    but cannot be lower then zero. 
---
--- @param moves_left int The number of move fragments remaining.
function Unit:moves_left_set(moves_left) end

--- .. note::
---    This may exceed the max for the :lua:obj:`Unit_Type.move_rate` type,
---    but gets capped at zero when adding negative amounts. 
---
--- @param moves_left int The number of move fragments to add.
function Unit:moves_left_add(moves_left) end

--- .. note::
---    This cannot be less than zero or exceed 
---    :lua:obj:`Unit_Type.max_veteran_level`.
---
--- .. note::
---    This does not automatically :lua:obj:`notify` the :lua:obj:`Player`
---    who owns the unit.
---
--- @param veteran int The veterancy level of the unit.
function Unit:veteran_level_set(veteran) end

--- .. note::
---    This will be capped between zero and 
---    :lua:obj:`Unit_Type:max_veteran_level`.
---
--- .. note::
---    This does not automatically :lua:obj:`notify` the :lua:obj:`Player`
---    who owns the unit.
---
--- @param veteran int The number of veterancy levels to add to the unit.
--- @return boolean changed True, if the veteran level changed as a result.
function Unit:veteran_level_add(veteran) end

--- .. note::
---    This cannot exceed the max :lua:obj:`Unit_Type.fuel`, and cannot be
---    lower then zero. 
---
--- @param fuel int The amount of fuel remaining.
function Unit:fuel_set(fuel) end

--- .. note::
---    This will be capped at the max :lua:obj:`Unit_Type.fuel`, and capped
---    at zero for negative amounts.
---
--- @param fuel int The amount of fuel to add.
--- @return boolean changed True, if the fuel level changed as a result.
function Unit:fuel_add(fuel) end

--- See :lua:obj:`edit.create_owned_extra`
--- @param name string
--- @param player Player
function Tile:create_owned_extra(name, player)
  edit.create_owned_extra(self, name, player)
end

--- See :lua:obj:`edit.create_extra`
--- @param name string
function Tile:create_extra(name)
  edit.create_extra(self, name)
end

--- See :lua:obj:`edit.remove_extra`
--- @param name string
function Tile:remove_extra(name)
  edit.remove_extra(self, name)
end

--- See :lua:obj:`edit.change_terrain`
--- @param terrain Terrain_Type
function Tile:change_terrain(terrain)
  edit.change_terrain(self, terrain)
end

--- See :lua:obj:`edit.tile_primary_resource_set`
--- @param resource string
function Tile:set_primary_resource(resource)
  edit.tile_primary_resource_set(self, resource)
end

--- See :lua:obj:`edit.unleash_barbarians`
function Tile:unleash_barbarians()
  return edit.unleash_barbarians(self)
end

--- See :lua:obj:`edit.place_partisans`
--- @param player Player
--- @param count int
--- @param sq_radius int
function Tile:place_partisans(player, count, sq_radius)
  edit.place_partisans(self, player, count, sq_radius)
end

--- See :lua:obj:`edit.tile_set_label`
--- @param label string
function Tile:set_label(label)
  edit.tile_set_label(self, label)
end

--- Gets the current numeric value of AI trait trait in effect for player. This
--- is usually :lua:obj:`~Player.trait_base` + 
--- :lua:obj:`~Player.trait_current_mod`. See :lua:obj:`edit.trait_mod` for
--- traits. 
--- @param trait_name string The name of the trait to retrieve.
--- @return int value The value of the specified trait.
function Player:trait(trait_name) end

--- Gets the base trait value (chosen at game start, before any trait_mod) for
--- a player.
--- @param trait_name string The name of the trait to retrieve.
--- @return int base The base value of the specified trait.
function Player:trait_base(trait_name) end

--- Gets the current modifier of a trait for a player.
--- @param trait_name string The name of the trait to retrieve the current modifier for.
--- @return int modifier The current modification value of the specified trait.
function Player:trait_current_mod(trait_name) end

--- Gets the minimum start value of an AI trait for a nation type.
--- @param trait_name string The name of the AI trait.
--- @return int min The minimum value of the specified trait.
function Nation_Type:trait_min(trait_name) end

--- Gets the maximum start value of an AI trait for a nation type.
--- @param trait_name string The name of the AI trait.
--- @return int max The maximum value of the specified trait.
function Nation_Type:trait_max(trait_name) end

--- Gets the default start value of an AI trait for a nation type. Used when
--- server option :ref:`traitdistribution <server-option-trait-distribution>`
--- is :literal:`FIXED`.
--- @param trait_name string The name of the AI trait.
--- @return int default The default value of the specified trait.
function Nation_Type:trait_default(trait_name) end

