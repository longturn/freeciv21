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

--- Signals are emitted by the server when certain events occur. 
---
--- See :lua:obj:`Events` for a list of specific signals. 
---
--- Signal emission invokes all associated callbacks in the order they were
--- connected. A callback can stop the current signal emission, preventing
--- callbacks connected after it from being invoked by returning true.
---
--- !doctype table
--- @class signal
signal = {}


--- Register this Lua function to receive the given signal.
---
--- @param signal_name string The :lua:obj:`Events` signal name to receive callbacks for.
--- @param callback_name string The global Lua function to call when this signal is emitted
function signal.connect(signal_name, callback_name) end


--- Remove an existing registration of this Lua function to receive the given
--- signal.
---
--- @param signal_name string The :lua:obj:`Events` signal name it should no longer receive callbacks for.
--- @param callback_name string The global Lua function that should no longer receive this signal callback.
function signal.remove(signal_name, callback_name) end


--- Check for an existing registration of this Lua function to receive the
--- given signal.
---
--- @param signal_name string The :lua:obj:`Events` signal name to check if it receives callbacks for.
--- @param callback_name string The global Lua function to check if it receives this signal callback.
--- @return boolean defined True, if this signal handler is defined.
function signal.defined(signal_name, callback_name) end

--- List all signals as well as any callbacks, via :lua:func:`log.normal`.
--- Intended for debugging.
function signal.list() end


--- Refreshes an existing registration of this Lua function to receive the
--- given signal. This is intended for debugging purposes, and can be used for
--- modifying a callback handler function while the server is running.
---
--- @param signal_name string The :lua:obj:`Events` signal name to receive callbacks for.
--- @param callback_name string The global Lua function to call when this signal is emitted.
function signal.replace(signal_name, callback_name) end

--- Functions in this module are used to acquire objects for further
--- manipulation, given various kinds of identifying information. Functions are
--- overloaded so that a given object can be identified in several ways.
---
--- !doctype table
--- @class find
find = {}


--- Can be used to iterate over all defined signals (until nil is returned).
---
--- @return string signal The :lua:obj:`Events` signal name.
function find.signal(index) end

--- Can be used to iterate over all callbacks currently associated with a given
--- signal. 
---
--- @return string func The name of the global Lua function that receives the callback for this signal.
function find.signal_callback(signal_name, index) end

