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

-- Nonexistent methods.
function Nonexistent:exists()
  return false
end

-- Log module implementation.

function log.fatal(fmt, ...)
  log.base(log.level.FATAL, string.format(fmt, ...))
end

function log.error(fmt, ...)
  log.base(log.level.ERROR, string.format(fmt, ...))
end

function log.warn(fmt, ...)
  log.base(log.level.WARN, string.format(fmt, ...))
end

function log.normal(fmt, ...)
  log.base(log.level.NORMAL, string.format(fmt, ...))
end

function log.verbose(fmt, ...)
  log.base(log.level.VERBOSE, string.format(fmt, ...))
end

function log.debug(fmt, ...)
  log.base(log.level.DEBUG, string.format(fmt, ...))
end

-- ***************************************************************************
-- Old logging functions
-- Deprecated. New logging functions are listed above.
-- ***************************************************************************
function error_log(msg)
  log.error(msg)
end

function debug_log(msg)
  log.debug(msg)
end

-- ***************************************************************************
-- Dump the state of user scalar variables to a Lua code string.
-- ***************************************************************************
function _freeciv_state_dump()
  local res = ''

  for k, v in pairs(_G) do
    if k == '_VERSION' then
      -- ignore _VERSION variable.
    elseif type(v) == 'boolean'
        or type(v) == 'number' then
      -- FIXME: depending on locale, the number formatting can use a
      -- comma as radix point, whereas we always want '.'. HRM #722300
      local rvalue = tostring(v)

      res = res .. k .. '=' .. rvalue .. '\n'
    elseif type(v) == 'string' then
      local rvalue = string.format('%q', v)

      res = res .. k .. '=' .. rvalue .. '\n'
    elseif type(v) == 'userdata' then
      local method = string.lower(tolua.type(v))

      res = res .. k .. '=find.' .. method
      if method == 'city' or method == 'unit' then
        res = res .. '(nil,' .. string.format("%d", v.id) .. ')'
      elseif v.id then
        res = res .. '(' .. string.format("%d", v.id) .. ')'
      else
        res = res .. '()'
      end
      res = res .. '\n'
    end
  end

  return res
end

-- ***************************************************************************
-- List all defined lua variables (functions, tables)
-- Source http://www.wellho.net/resources/ex.php4?item=u112/basics
-- ***************************************************************************
function listenv()
  -- helper function for listenv
  local function _listenv_loop(offset, data)
    local name
    local value

    for name,value in pairs(data) do
      if name ~= "loaded" and name ~= "_G" and name:sub(0,2) ~= "__" then
        log.normal("%s- %s: %s", offset, type(value), name)
        if type(value) == "table" then
          _listenv_loop(offset .. "    ", value)
        end
      end
    end
  end

  _listenv_loop("", _G)
end

-- ***************************************************************************
-- Flexible "constant" implementation
-- source: http://developer.anscamobile.com/code/\
--                universal-constants-module-very-easy-usage
-- written in 2010 by Hans Raaf - use as you wish!
-- ***************************************************************************

const = {}
local data = {}
const_mt = {
  __newindex = function(a,b,c)
    if data[b] == nil then
      if type(c) == 'table' then
        -- make that table readonly
        local proxy = {}
        -- create metatable
        local mt = {
          __index = c,
          __newindex = function (t,k,v)
            log.error([["Attempt to update read-only table '%s' index '%s' "
                        "with '%s'."]], b, tostring(k), tostring(v))
          end
        }
        setmetatable(proxy, mt)
        data[b] = proxy
      else
        data[b] = c
      end
    else
      log.error("Illegal assignment to constant '%s'.", tostring(b))
    end
  end,

  __index = function(a,b)
    return data[b]
  end
}
setmetatable(const, const_mt)
