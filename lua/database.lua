-- Freeciv - Copyright (C) 2011 - The Freeciv Project
--   This program is free software; you can redistribute it and/or modify
--   it under the terms of the GNU General Public License as published by
--   the Free Software Foundation; either version 2, or (at your option)
--   any later version.
--
--   This program is distributed in the hope that it will be useful,
--   but WITHOUT ANY WARRANTY; without even the implied warranty of
--   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--   GNU General Public License for more details.

-- This file is the Freeciv server`s interface to the database backend
-- when authentication is enabled. See doc/README.fcdb.

-- Lua Base64 License

-- Copyright (c) 2012, Daniel Lindsley
-- All rights reserved.
-- 
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions are met:
-- 
-- * Redistributions of source code must retain the above copyright notice, this
--   list of conditions and the following disclaimer.
-- * Redistributions in binary form must reproduce the above copyright notice,
--   this list of conditions and the following disclaimer in the documentation
--   and/or other materials provided with the distribution.
-- * Neither the name of the base64 nor the names of its contributors may be
--   used to endorse or promote products derived from this software without
--   specific prior written permission.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
-- WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
-- DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
-- FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
-- DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
-- SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
-- CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
-- OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
-- OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

local index_table = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

local function to_binary(integer)
    local remaining = tonumber(integer)
    local bin_bits = ''

    for i = 7, 0, -1 do
        local current_power = 2 ^ i

        if remaining >= current_power then
            bin_bits = bin_bits .. '1'
            remaining = remaining - current_power
        else
            bin_bits = bin_bits .. '0'
        end
    end

    return bin_bits
end

local function from_binary(bin_bits)
    return tonumber(bin_bits, 2)
end

local function from_base64(to_decode)
    local padded = to_decode:gsub("%s", "")
    local unpadded = padded:gsub("=", "")
    local bit_pattern = ''
    local decoded = ''

    for i = 1, string.len(unpadded) do
        local char = string.sub(to_decode, i, i)
        local offset, _ = string.find(index_table, char)
        if offset == nil then
             error("Invalid character '" .. char .. "' found.")
        end

        bit_pattern = bit_pattern .. string.sub(to_binary(offset-1), 3)
    end

    for i = 1, string.len(bit_pattern), 8 do
        local byte = string.sub(bit_pattern, i, i+7)
        decoded = decoded .. string.char(from_binary(byte))
    end

    local padding_length = padded:len()-unpadded:len()

    if (padding_length == 1 or padding_length == 2) then
        decoded = decoded:sub(1,-2)
    end
    return decoded
end

local dbh = nil

function check_password(hash, plaintext)
  local c, salt, hash = string.match(hash, 'pbkdf2_sha256$(%d+)$([^$]+)$(.*)')
  local got = pbkdf2_sha256(plaintext, salt, tonumber(c), 256)
  return from_base64(hash) == got
end

-- **************************************************************************
-- freeciv user auth functions
-- **************************************************************************

-- Check if user exists.
function user_exists(conn)
  if not dbh then
    error("Missing database connection...")
  end

  local username = dbh:escape(auth.get_username(conn))

  local query = string.format([[SELECT count(*) FROM auth_user WHERE username = '%s']], username)
  local res = assert(dbh:execute(query))

  local row = res:fetch()
  res:close()

  return tonumber(row) == 1
end

-- Check user password.
function user_verify(conn, plaintext)
  if not dbh then
    error("Missing database connection...")
  end

  local username = dbh:escape(auth.get_username(conn))

  -- get the password for this user
  local query = string.format([[SELECT password FROM auth_user WHERE username = '%s']], username)
  local res = assert(dbh:execute(query))

  local row = res:fetch({}, 'a')
  if not row then
    -- No match
    res:close()
    return nil
  end

  local ok = check_password(row.password, plaintext)

  -- There should be only one result
  if res:fetch() then
    res:close()
    error(string.format('Multiple entries (%d) for user: %s',
                        numrows, username))
  end

  res:close()

  return ok
end

-- save a user to the database
function user_save(conn, password)
end

-- log the session
function user_log(conn, success)
end

-- **************************************************************************
-- freeciv database entry functions
-- **************************************************************************

-- test and initialise the database connection
function database_init()
  local sql = ls_postgres.postgres()

  dbh = assert(sql:connect("longturn", "longturn"))
end

-- free the database connection
function database_free()
  log.verbose('Closing database connection...')

  if dbh then
    dbh:close()
  end
end
