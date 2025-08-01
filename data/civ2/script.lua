-- Copyright (c) 2007-2020 Freeciv21 and Freeciv contributors. This file is
--   part of Freeciv21. Freeciv21 is free software: you can redistribute it
--   and/or modify it under the terms of the GNU General Public License
--   as published by the Free Software Foundation, either version 3
--   of the License,  or (at your option) any later version.
--   You should have received a copy of the GNU General Public License
--   along with Freeciv21. If not, see https://www.gnu.org/licenses/.

-- This file is for lua-functionality that is specific to a given
-- ruleset. When freeciv loads a ruleset, it also loads script
-- file called 'default.lua'. The one loaded if your ruleset
-- does not provide an override is default/default.lua.

-- 
-- Trireme lost at sea mechanic
--
function is_on_coast(tile)
  for adj in tile:square_iterate(1) do
    if adj.terrain:class_name() ~= "Oceanic" then
      return true
    end
  end
  return false
end

lighthouse = find.building_type("Lighthouse")
seafaring = find.tech_type("Seafaring")
navigation = find.tech_type("Navigation")

function unsafe_away_from_coast(unit)
  if not unit.utype:has_flag("Coast") then return end
  if not unit.owner:is_human() then return end
  if unit.owner:has_wonder(lighthouse) then return end
  if is_on_coast(unit.tile) then return end
  log.debug("%s #%d is away from coast", unit.utype:rule_name(), unit.id)
  local odds = 2
  if unit.owner:knows_tech(seafaring) then odds = odds + 1 end
  if unit.owner:knows_tech(navigation) then odds = odds + 1 end
  log.debug("Survival odds 1 in %d", odds)
  local roll = random(1, odds)
  log.debug("Rolled a %d", roll)
  if roll == 1 then
    log.debug("Lost at sea")
    notify.event(unit.owner, unit.tile, E.UNIT_LOST_MISC, 
      "Your %s has been lost at sea. This kind of vessel may disappear " .. 
      "forever if not adjacent to land at the end of a turn.", 
      unit.utype:name_translation())
    unit:kill("hp_loss")
  else
    log.debug("Survived")
    notify.event(unit.owner, unit.tile, E.UNIT_ORDERS, "Your %s was lucky " ..
      "and managed to weather the rough seas this time. This kind of vessel " ..
      "may disappear forever if not adjacent to land at the end of a turn.",
      unit:link_text())
  end
end

function unsafe_terrain_check_phase()
  for player in players_iterate() do
    for unit in player:units_iterate() do
      unsafe_away_from_coast(unit)
    end
  end
end

-- TODO: This should really occur as soon as the unit makes its final move, 
-- however we need to be able to get the unit's current MP to see if this is
-- the case. See: https://github.com/longturn/freeciv21/issues/972
signal.connect("turn_begin", "unsafe_terrain_check_phase")

--
-- End of Trireme lost at sea mechanic
--
