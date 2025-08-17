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

-- For code documentation, see:
-- https://luals.github.io/wiki/annotations/#documenting-types
-- https://taminomara.github.io/sphinx-lua-ls/index.html#autodoc-directives
-- https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#rst-primer

--- Checks if the object (still) exists.
---
--- @return boolean
function Nonexistent:exists()
  return false
end

--- Logging facilities from Lua script. This is the preferred way to emit
--- textual output from Freeciv21 Lua scripts. Messages emitted with these
--- functions will be sent to an appropriate place, which will differ depending
--- on the context. 
---
--- !doctype table
--- @class log

--- Log message at Fatal level with printf-like formatting (Lua's 
--- string.format). Note that Fatal level warnings DO NOT abort the game.
---
--- @param fmt string
function log.fatal(fmt, ...)
  log.base(log.level.FATAL, string.format(fmt, ...))
end

--- Log message at Error level with printf-like formatting (Lua's 
--- string.format).
---
--- @param fmt string
function log.error(fmt, ...)
  log.base(log.level.ERROR, string.format(fmt, ...))
end

--- Log message at Warn level with printf-like formatting (Lua's string.format).
---
--- @param fmt string
function log.warn(fmt, ...)
  log.base(log.level.WARN, string.format(fmt, ...))
end

--- Log message at Normal level with printf-like formatting (Lua's 
--- string.format).
---
--- @param fmt string
function log.normal(fmt, ...)
  log.base(log.level.NORMAL, string.format(fmt, ...))
end

--- Log message at Verbose level with printf-like formatting (Lua's 
--- string.format).
---
--- @param fmt string
function log.verbose(fmt, ...)
  log.base(log.level.VERBOSE, string.format(fmt, ...))
end

--- Log message at Debug level with printf-like formatting (Lua's 
--- string.format).
---
--- @param fmt string
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

--- Dump the state of user scalar variables to a Lua code string.
--- @return string state
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

--- List all defined lua variables (functions, tables)
function listenv()
  -- Source http://www.wellho.net/resources/ex.php4?item=u112/basics
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

--- Flexible "constant" implementation. Define your constants once and they
--- then become unchangeable. E.g.
---
--- .. code-block:: lua
---
---     const.test = 'Hello World!'
---     log.normal("%s", const.test) -- prints "Hello World!"
---
---     const.test = 'Not with me Dude!' -- prints error for that line
---
---     const.inner_test = {'Hello','World'}
---     log.normal("%s %s!", const.inner_test[1], const.inner_test[2])
---
---     const.inner_test[2] = 'Superman' -- prints error for that line
---
--- !doctype table
--- @class const

-- source: https://web.archive.org/web/20120202141634/http://developer.anscamobile.com/code/universal-constants-module-very-easy-usage
-- written in 2010 by Hans Raaf - use as you wish!
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
