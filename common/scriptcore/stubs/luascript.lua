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

--- @param text String The raw text to translate.
--- @return String translated The translated text.
function _(text) end

--- @param text String The raw text to mark for translation.
--- @return String raw-text The raw text again.
function N_(text) end

--- @param text String The raw text to disambiguate.
--- @return String translated The translated text.
function Q_(text) end

--- @param singular String The raw text in singular form.
--- @param plural String The raw text in plural form.
--- @param count int The quantity to determine which form is used.
--- @return String translated The translated text for use with :lua:func:`string.format`.
function PL_(singular, plural, count) end

--- Generates a random number using Freeciv21's random number generator and
--- random seeds. This allows reproducible games given the same initial
--- conditions and inputs. Use this rather than Lua's :lua:func:`math.random`. 
--- @param min int The minimum roll value (inclusive).
--- @param max int The maximum roll value (inclusive).
--- @return Number roll The random number.
function random(min, max) end

--- @return String version The current version of the project.
function fc_version() end

