-- Copyright (c) Freeciv21 and Freeciv contributors. This file is
--   part of Freeciv21. Freeciv21 is free software: you can redistribute it
--   and/or modify it under the terms of the GNU General Public License
--   as published by the Free Software Foundation, either version 3
--   of the License,  or (at your option) any later version.
--   You should have received a copy of the GNU General Public License
--   along with Freeciv21. If not, see https://www.gnu.org/licenses/.

-- This file is the Freeciv21 server`s interface to the database backend
-- when authentication is enabled.
-- See https://longturn.readthedocs.io/en/latest/Coding/fcdb.html.

local md5 = require "md5"
local dbh = nil

-- Machinery for debug logging of options
local seen_options
local function options_init()
  seen_options = {}
end
local function option_log(name, val, is_sensitive, source)
  if not seen_options[name] then
    seen_options[name] = true
    if is_sensitive then
      log.debug('Database option \'%s\': %s', name, source)
    else
      log.debug('Database option \'%s\': %s: value \'%s\'', name, source, val)
    end
  end
end

-- Get an option from configuration file, falling back to sensible
-- defaults where they exist
local function get_option(name, is_sensitive)
  local defaults = {
    backend    = "sqlite",
    table_user = "fcdb_auth",
    table_log  = "fcdb_log"
  }
  local val = fcdb.option(name)
  if val then
    option_log(name, val, is_sensitive, 'read from file')
  else
    val = defaults[name]
    if val then
      option_log(name, val, is_sensitive, 'using default')
    end
  end
  if not val then
    log.error('Database option \'%s\' not specified in configuration file',
              name)
  end
  return val
end

-- open a SQLite database (or raise an error)
local function sqlite_connect()
  if dbh then
    dbh:close()
  end

  local sql = require "luasql.sqlite3".sqlite3()

  -- Load the database parameters.
  local database = get_option("database")

  dbh = assert(sql:connect(database))
end

function sql_time()
  local backend = get_option("backend")
  if backend == 'sqlite' then
    return os.time()
  else
    log.error('Don\'t know how to do timestamps for database backend \'%s\'', backend)
    return 'ERROR'
  end
end

-- Set up tables for an SQLite database.
function sqlite_createdb()
  local query

  if get_option("backend") ~= 'sqlite' then
    error("'backend' in configuration file must be 'sqlite'")
  end

  local table_user = get_option("table_user")
  local table_log  = get_option("table_log")

  if not dbh then
    error("Missing database connection")
  end

  query = string.format([[
CREATE TABLE %s (
  id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  name VARCHAR(48) default NULL UNIQUE,
  password VARCHAR(32) default NULL,
  email VARCHAR default NULL,
  createtime INTEGER default NULL,
  accesstime INTEGER default NULL,
  address VARCHAR default NULL,
  createaddress VARCHAR default NULL,
  logincount INTEGER default '0'
);
]], table_user)
  assert(dbh:execute(query))

  query = string.format([[
CREATE TABLE %s (
  id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  name VARCHAR(48) default NULL,
  logintime INTEGER default NULL,
  address VARCHAR default NULL,
  succeed TEXT default 'S'
);]], table_log)
  assert(dbh:execute(query))
end

-- Check if user exists.
function user_exists(conn)
  local res     -- result handle

  local table_user = get_option("table_user")

  if not dbh then
    error("Missing database connection...")
  end

  local username = dbh:escape(auth.get_username(conn))

  query = string.format([[SELECT count(*) FROM %s WHERE name = '%s']],
                        table_user, username)
  res = assert(dbh:execute(query))

  local count = res:fetch()
  res:close()

  return count == 1
end

-- Check user password.
function user_verify(conn, plaintext)
  local res     -- result handle
  local row     -- one row of the sql result
  local query   -- sql query

  local fields = 'password'

  local table_user = get_option("table_user")

  if not dbh then
    error("Missing database connection...")
  end

  local username = dbh:escape(auth.get_username(conn))

  -- get the password for this user
  query = string.format([[SELECT %s FROM %s WHERE name = '%s']],
                        fields, table_user, username)
  res = assert(dbh:execute(query))

  row = res:fetch({}, 'a')
  if not row then
    -- No match
    res:close()
    return nil
  end

  -- There should be only one result
  if res:fetch() then
    res:close()
    error(string.format('Multiple entries (%d) for user: %s',
                        numrows, username))
  end

  res:close()

  return row.password == md5.sum(plaintext)
end

-- save a user to the database
function user_save(conn, password)
  local table_user = get_option("table_user")

  if not dbh then
    error("Missing database connection...")
  end

  local username = dbh:escape(auth.get_username(conn))
  local ipaddr = auth.get_ipaddr(conn)

  -- insert the user
  local query = string.format([[INSERT INTO %s VALUES (NULL, '%s', '%s',
                                NULL, %s, %s, '%s', '%s', 0)]],
                              table_user, username, md5.sum(password),
                              sql_time(), sql_time(),
                              ipaddr, ipaddr)
  assert(dbh:execute(query))

  user_log(conn, true)
end

-- log the session
function user_log(conn, success)
  local query   -- sql query

  if not dbh then
    error("Missing database connection...")
  end

  local table_user = get_option("table_user")
  local table_log  = get_option("table_log")

  local username = dbh:escape(auth.get_username(conn))
  local ipaddr = auth.get_ipaddr(conn)
  local success_str = success and 'S' or 'F'

  -- update user data
  query = string.format([[UPDATE %s SET accesstime = %s, address = '%s',
                          logincount = logincount + 1
                          WHERE name = '%s']], table_user, sql_time(),
                          ipaddr, username)
  assert(dbh:execute(query))

  -- insert the log row for this user
  query = string.format([[INSERT INTO %s (name, logintime, address, succeed)
                          VALUES ('%s', %s, '%s', '%s')]],
                        table_log, username, sql_time(), ipaddr, success_str)
  assert(dbh:execute(query))
end

-- **************************************************************************
-- freeciv21 database entry functions
-- **************************************************************************

-- test and initialise the database connection
function database_init()
  options_init()

  local backend = get_option("backend")

  if backend == 'sqlite' then
    log.verbose('Opening SQLite database connection...')
    return sqlite_connect()
  end

  error(string.format(
    'Database backend \'%s\' not supported by database.lua',
    backend))
end

-- free the database connection
function database_free()
  log.verbose('Closing database connection...')

  if dbh then
    dbh:close()
  end
end
