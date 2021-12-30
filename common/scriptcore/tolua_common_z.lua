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
    "Nonexistent",
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
      return string.format('<%s #%d %s>', tolua.type(self), id, name)
    elseif id then
      return string.format('<%s #%d>', tolua.type(self), id)
    else
      return string.format('<%s>', tolua.type(self))
    end
  end

  for index, typename in ipairs(api_types) do
    local api_type = _G[typename]

    api_type[".eq"] = id_eq
    api_type.__tostring = string_rep

    -- Object field resolution
    -- 1) Check properties defined in our API
    --    (Properties are fields that call an accessor to get their value)
    -- 2) Delegate to tolua's __index if name is without _ or . prefix
    --    (metamethods and tolua fields give access to unprotected C
    --     functions in a pointer-unsafe way).
    -- otherwise, return nil
    local api_type_index = api_type.__index
    local properties = api_type.properties
    -- Prevent tampering with the notion of equality
    local rawequal = rawequal
    local string_sub = string.sub

    local function field_getter(self, field)
      local getter = properties and properties[field]
      if getter then
        return getter(self)
      else
        local pfx = string_sub(field, 1, 1)
        if rawequal(pfx, '.') or rawequal(pfx, '_') then
          return nil
        else
          return api_type_index(self, field)
        end
      end
    end
    api_type.__index = field_getter

    -- Delete '.set' table to disallow direct writing of struct fields
    api_type[".set"] = nil
    -- Hide the metatable and hide the class from global namespace
    api_type.__metatable = false
    _G[typename] = nil
  end
  -- End (API Types Special Methods)
end

-- ***************************************************************************
-- API Lockdown
-- ***************************************************************************

-- Override global 'tolua' module with a reduced version
tolua = {
  type=tolua.type,
}

-- Hide all private methods
methods_private = nil
