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


--- Connect to an event signal using
---
--- .. code-block:: lua
---
---    signal.connect(signal_name, callback_name)
---
--- Currently, signals are only sent to ruleset scripts running on the server.
--- The set of signals is defined in :file:`server/scripting/script_server.cpp`
--- (search for "``script_server_signals_create``").
---
--- If false is returned from a signal handler function, the signal will not
--- propagate to any subsequent signal handlers.
---
--- !doctype table
--- @class Events
Events = {}


--- A signal for the beginning of a turn.
--- @param turn Number The turn number, starting at 1.
--- @param year Number The calendar year number
function Events.turn_begin(turn, year) end

--- A signal for when a unit has moved. Emitted at the end of a unit_move().
--- @param unit Unit The unit that moved.
--- @param tile_from Tile The tile the unit moved from.
--- @param tile_to Tile The tile the unit moved to.
function Events.unit_moved(unit, tile_from, tile_to) end

--- A signal for when a city is built.
--- @param city City The city that was built.
function Events.city_built(city) end

--- A signal for changes in city size.
---
--- Reason is one of: "unit_added", "growth", "poison", "incited",
--- "upkeep_failure", "famine", "unit_built", "migration_from", "migration_to",
--- "disaster", "plague", "conquest", "nuke", "bombard", "attack", or a
--- user-defined value set in :lua:func:`edit.resize_city`
---
--- @param city City The city whose size changed.
--- @param size Number The new size of the city.
--- @param reason String The reason for the size change.
function Events.city_size_change(city, size, reason) end

--- A signal for when a unit is built in a city.
--- @param unit Unit The unit that was built.
--- @param city City The city where the unit was built.
function Events.unit_built(unit, city) end

--- A signal for when a building is built in a city.
--- @param building Building_Type The type of building that was built.
--- @param city City The city where the building was built.
function Events.building_built(building, city) end

--- A signal for when a unit cannot be built.
---
--- The reason is one of: "need_tech", "need_techflag", "need_building",
--- "have_building", "need_building_genus", "have_building_genus",
--- "need_government", "have_government", "need_achievement", "need_extra",
--- "have_extra", "need_good", "have_good", "need_terrain", "have_terrain",
--- "need_nation", "have_nation", "need_nationgroup", "have_nationgroup",
--- "need_style", "have_style", "need_nationality", "have_nationality",
--- "need_diplrel", "have_diplrel", "need_minsize", "have_minsize",
--- "need_minculture", "need_minforeignpct", "have_minforeignpct",
--- "need_mintechs", "need_tileunits", "have_tileunits", "need_terrainclass",
--- "have_terrainclass", "need_terrainflag", "have_terrainflag",
--- "need_baseflag", "have_baseflag", "need_roadflag", "have_roadflag",
--- "need_extraflag", "have_extraflag", "need_minyear", "need_mincalfrag",
--- "have_mincalfrag", "need_topo", "need_setting", "need_age", "never",
--- "unavailable", or "pop_cost"
---
--- .. note::
---    At the moment, ruleset doesn't allow full requirements for units, but
---    as soon as it does a lot of these will automatically come into play.
---
--- @param unit_type Unit_Type The type of unit that cannot be built.
--- @param city City The city where the unit cannot be built.
--- @param reason String The reason why the unit cannot be built.
function Events.unit_cant_be_built(unit_type, city, reason) end

--- A signal for when a building cannot be built.
---
--- The reason is one of: "need_tech", "need_techflag", "need_building",
--- "have_building", "need_building_genus", "have_building_genus",
--- "need_government", "have_government", "need_achievement", "need_extra",
--- "have_extra", "need_good", "have_good", "need_terrain", "have_terrain",
--- "need_nation", "have_nation", "need_nationgroup", "have_nationgroup",
--- "need_style", "have_style", "need_nationality", "have_nationality",
--- "need_diplrel", "have_diplrel", "need_minsize", "have_minsize",
--- "need_minculture", "need_minforeignpct", "have_minforeignpct",
--- "need_mintechs", "need_tileunits", "have_tileunits", "need_terrainclass",
--- "have_terrainclass", "need_terrainflag", "have_terrainflag",
--- "need_baseflag", "have_baseflag", "need_roadflag", "have_roadflag",
--- "need_extraflag", "have_extraflag", "need_minyear", "need_mincalfrag",
--- "have_mincalfrag", "need_topo", "need_setting", "need_age", "never", or
--- "unavailable".
---
--- @param building_type Building_Type The type of building that cannot be built.
--- @param city City The city where the building cannot be built.
--- @param reason String The reason why the building cannot be built.
function Events.building_cant_be_built(building_type, city, reason) end

--- A signal for when a building is lost.
---
--- The reason is one of: "razed", "conquered" (applicable for Small Wonders
--- only), "city_destroyed", "disaster", "attacked", "sabotaged", "landlocked",
--- "sold", "obsolete", or "cant_maintain"
---
--- destroyer is applicable for "sabotaged" only. 
---
--- @param city City The city where the building was lost.
--- @param building Building_Type The type of building that was lost.
--- @param reason String The reason for the loss.
--- @param destroyer Unit The unit responsible for the loss (if applicable).
function Events.building_lost(city, building, reason, destroyer) end

--- A signal for when a technology is researched.
---
--- source may be: "researched", "traded", "stolen", "hut", or a user-defined
--- value passed to the :lua:func:`edit.give_tech` function.
---
--- @param tech_type Tech_Type The type of technology that was researched.
--- @param player Player The player who researched the technology.
--- @param source String The source of the research.
function Events.tech_researched(tech_type, player, source) end

--- A signal for when a city is destroyed.
---
--- enemy may be nil if city was disbanded. 
---
--- @param city City The city that was destroyed.
--- @param owner Player The player who owned the city.
--- @param enemy Player The player who destroyed the city.
function Events.city_destroyed(city, owner, enemy) end

--- A signal for when loot is taken from a captured city. This occurs before
--- the city is captured so still belongs to the original owner, however it is
--- after spaceship is lost if it was the capital.
---
--- See :lua:obj:`_deflua_city_conquer_gold_loot` for default implementation.
---
--- @param city City The city from which loot was taken.
--- @param unit Unit The unit that took the loot.
function Events.city_loot(city, unit) end

--- A signal for when a city is transferred to another nation. 
---
--- Reason is one of: "conquest", "trade", "incited", "civil_war",
--- "death-back_to_original", or "death-barbarians_get"
---
--- @param city City The city that was transferred.
--- @param former_owner Player The former owner of the city.
--- @param new_owner Player The new owner of the city.
--- @param reason String The reason for the transfer.
function Events.city_transferred(city, former_owner, new_owner, reason) end

--- A signal for when a unit enters a hut. Generated for any extra of category
--- "Bonus" after it is removed if some extra on the tile has cause "Hut".
---
--- @param unit Unit The unit that entered the hut.
--- @param extra String The rule name of the hut extra.
function Events.hut_enter(unit, extra) end

--- A signal for when a hut is frightened by a unit. Generated for any extra of
--- category "Bonus" after it is removed if some extra on the tile has cause
--- "Hut", and the unit's hut behaviour is "frighten".
---
--- @param unit Unit The unit that entered the hut.
--- @param extra String The rule name of the hut extra.
function Events.hut_frighten(unit, extra) end

--- A signal for when a unit is lost.
---
--- See :ref:`Unit Loss Reasons <script-unit-loss-reasons>`.
---
--- @param unit Unit The unit that was lost.
--- @param player Player The player who lost the unit.
--- @param reason String The reason for the loss.
function Events.unit_lost(unit, player, reason) end

--- A signal for when a disaster occurs.
--- @param disaster Disaster The type of disaster that occurred.
--- @param city City The city affected by the disaster.
function Events.disaster_occurred(disaster, city) end

--- A signal for when a nuclear explosion occurs.
--- @param tile Tile The tile where the explosion occurred.
--- @param player Player The player responsible for the explosion.
function Events.nuke_exploded(tile, player) end

--- A signal for when an achievement is gained.
--- @param achievement Achievement The type of achievement gained.
--- @param player Player The player who gained the achievement.
--- @param first Boolean Indicates whether player is first one to reach the achievement.
function Events.achievement_gained(achievement, player, first) end

--- A signal for when a map is generated.
function Events.map_generated() end

--- A signal for a pulse event. Pulses occur at approximately one second
--- intervals. The time between pulses is not constant --- this cannot be used
--- to measure time.
function Events.pulse() end

--- A signal for when a unit performs an act on another single unit.
---
--- For valid actions, see 
--- :ref:`Actions Done by a Unit Against Another Unit <modding-ruleset-actions-vs-unit>`
---
--- @param action Action The action that is starting.
--- @param actor Unit The unit that is performing the action.
--- @param target Unit The unit the action is being performed on.
function Events.action_started_unit_unit(action, actor, target) end

--- A signal for when a unit performs an act on a stack of units.
---
--- For valid actions, see
--- :ref:`Actions Done by a Unit Against All Units at a Tile <modding-ruleset-actions-vs-stack>`
---
--- @param action Action The action that is starting.
--- @param actor Unit The unit that is performing the action.
--- @param tile Tile The tile where the unit stack is located.
function Events.action_started_unit_units(action, actor, tile) end

--- A signal for when a unit performs an act on a city.
---
--- For valid actions, see 
--- :ref:`Actions Done by a Unit Against a City <modding-ruleset-actions-vs-city>`
---
--- @param action Action The action that is starting.
--- @param actor Unit The unit that is performing the action.
--- @param city City The city involved in the action.
function Events.action_started_unit_city(action, actor, city) end

--- A signal for when a unit performs an act on a tile.
---
--- For valid actions, see
--- :ref:`Actions Done by a Unit Against a Tile <modding-ruleset-actions-vs-tile>`
---
--- @param action Action The action that is starting.
--- @param actor Unit The unit that is performing the action.
--- @param tile Tile The tile involved in the action.
function Events.action_started_unit_tile(action, actor, tile) end

--- A signal for when a unit performs an act on itself.
---
--- For valid actions, see
--- :ref:`Actions Done by a Unit to Itself <modding-ruleset-actions-vs-self>`
---
--- @param action Action The action that is starting.
--- @param actor Unit The unit that is performing the action.
function Events.action_started_unit_self(action, actor) end

