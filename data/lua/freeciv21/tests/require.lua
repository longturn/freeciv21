-- SPDX-License-Identifier: GPL-3.0-or-later
-- SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
-- SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

-- Tests for the require function
-- See: https://luaunit.readthedocs.io/en/luaunit_v3_5/

local lu = require("luaunit")

local M = {}

function M.testCannotNavigateToParent()
  lu.assertErrorMsgContains("disallowed for security reasons",
      require, "../parent_module_test")
end

function M.testCannotUseAbsolutePath()
  lu.assertErrorMsgContains("disallowed for security reasons",
      require, "/usr/share/lua/module")
end

function M.testCannotOpenSOFiles()
  lu.assertErrorMsgContains("No Freeciv21 script found",
      require, "freeciv21/tests/something_scary.so")
end

return M

