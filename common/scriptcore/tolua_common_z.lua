-- Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
-- part of Freeciv21. Freeciv21 is free software: you can redistribute it
-- and/or modify it under the terms of the GNU  General Public License  as
-- published by the Free Software Foundation, either version 3 of the
-- License,  or (at your option) any later version. You should have received
-- a copy of the GNU General Public License along with Freeciv21. If not,
-- see https://www.gnu.org/licenses/.

--  WARNING: do not attempt to change the name of the API functions.
--  They may be in use in Lua scripts in savefiles, so once released, the
--  name and signature cannot change shape even in new major versions of
--  Freeciv, until the relevant save format version can no longer be loaded.
--  If you really like to change a function name, be sure to keep also the
--  old one running.

-- ***************************************************************************
-- API Types Special Methods
-- check also luascript_types.h
-- ***************************************************************************
do
  local api_types = {
    "Building_Type",
    "City",
    "City_List_Link",
    "Connection",
    "Government",
    "Nation_Type",
    "Player",
    "Tech_Type",
    "Terrain",
    "Tile",
    "Unit",
    "Unit_List_Link",
    "Unit_Type",
    "Disaster",
    "Achievement",
    "Action"
  }

  local function id_eq (o1, o2)
    return o1.id == o2.id and (o1.id ~= nil)
  end

  -- define string representation for tostring
  local function string_rep(self)
    local id = self.id
    local name = self.rule_name and self:rule_name() or self.name
    if name and id then
      return string.format('<%s #%d %s>', self.__type.name, id, name)
    elseif id then
      return string.format('<%s #%d>', self.__type.name, id)
    else
      return string.format('<%s>', self.__type.name)
    end
  end

  for index, typename in ipairs(api_types) do
    local api_type = _G[typename]

    api_type[".eq"] = id_eq
    api_type.__tostring = string_rep
  end
  -- End (API Types Special Methods)
end

-- ***************************************************************************
-- API Lockdown
-- ***************************************************************************

-- Hide all private methods
methods_private = nil
