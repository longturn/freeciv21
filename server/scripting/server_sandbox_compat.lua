-- SPDX-License-Identifier: GPL-3.0-or-later
-- SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
-- SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

-- Adds safe implementations of functions to the global scope for the sandboxed
-- server script environment for compatibility with externally imported modules.

do
  env = env or {}
  os.getenv = function(var_name)
    return env[var_name]
  end

  os.exit = function(status_code)
    if status_code then
      error(string.format("Fail with exit code %d.", status_code))
    else
      print("Success on exit.")
    end
  end

  local BufferedWriter = {}
  BufferedWriter.prefix = ""
  BufferedWriter.buffer = ""

  function BufferedWriter:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
  end

  function BufferedWriter:write(...)
    self.buffer = self.buffer .. table.concat{...}
    while true do
      local eol = self.buffer:find("\n", 1, true)
      if not eol then break end
      print(self.prefix .. self.buffer:sub(1, eol - 1))
      self.buffer = self.buffer:sub(eol + 1)
    end
  end

  function BufferedWriter:flush()
    if self.buffer ~= "" then
      print(self.prefix .. self.buffer)
      self.buffer = ""
    end
  end

  function BufferedWriter:close()
    self:flush()
    print(self.prefix .. "Closed")
  end

  io = {}
  io.stdout = BufferedWriter:new()
  io.stderr = BufferedWriter:new{prefix = "Error: "}
  io.open = function(name, mode)
    if mode == "w" then
      return BufferedWriter:new{prefix = "File " .. name .. ": "}
    else
      error("Unsupported file mode " .. mode)
    end
  end
end
