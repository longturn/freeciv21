-- SPDX-License-Identifier: GPL-3.0-or-later
-- SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
-- SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

local lu = require("luaunit")

local M = {}

function M.run(...)
  print("Running all unit test modules...")
  os.exit(lu.LuaUnit.new():runSuiteByInstances({
    {"TestRequire", require("freeciv21.tests.require")}
  }, ...))
end

return M

