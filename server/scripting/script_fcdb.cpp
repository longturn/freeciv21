/**************************************************************************
 Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

// Qt
#include <QStandardPaths>

// sol2
#include "sol/sol.hpp"

/* dependencies/lua */
#include "lua.h"
#include "lualib.h"

/* dependencies/tolua */
#include "tolua.h"

// utility
#include "log.h"

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"
#include "tolua_common_a_gen.h"
#include "tolua_game_gen.h"

// server
#include "auth.h"
#include "console.h"
#include "stdinhand.h"

#include "script_fcdb.h"

#define SCRIPT_FCDB_LUA_FILE "freeciv21/database.lua"

static bool script_fcdb_functions_check(const char *fcdb_luafile);

static bool script_fcdb_database_init();
static bool script_fcdb_database_free();

static void script_fcdb_cmd_reply(struct fc_lua *lfcl, QtMsgType level,
                                  const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

/// Lua virtual machine state.
static sol::state *fcl = nullptr;

/// Tolua compatibility
static fc_lua fcl_compat = fc_lua();

/**
   fcdb callback functions that must be defined in the lua script
   'database.lua':

   database_init:
     - test and initialise the database.
   database_free:
     - free the database.

   user_exists(Connection pconn):
     - Check if the user exists.
   user_save(Connection pconn, String password):
     - Save a new user.
   user_verify(Connection pconn):
     - Check the credentials of the user.
   user_delegate_to(Connection pconn, Player pplayer, String delegate):
     - returns Bool, whether pconn is allowed to delegate player to delegate.
   user_take(Connection requester, Connection taker, Player pplayer,
             Bool observer):
     - returns Bool, whether requester is allowed to attach taker to pplayer.
 */

/**
   Check the existence of all needed functions.
 */
static bool script_fcdb_functions_check(const char *fcdb_luafile)
{
  // Mandatory functions
  bool ret = true;
  for (const auto &name : {
           "database_init",
           "database_free",
           "user_exists",
           "user_verify",
       }) {
    if (!(*fcl)[name].valid()) {
      qCritical("Database script '%s' does not define the required function "
                "'%s'.",
                fcdb_luafile, name);
      ret = false;
    }
  }

  // Optional functions
  for (const auto &name : {
           "user_save",
           "user_delegate_to",
           "user_take",
       }) {
    if (!(*fcl)[name].valid()) {
      qDebug("Database script '%s' does not define the optional "
             "function '%s'.",
             fcdb_luafile, name);
    }
  }

  return ret;
}

/**
   Send the message via cmd_reply().
 */
static void script_fcdb_cmd_reply(struct fc_lua *lfcl, QtMsgType level,
                                  const char *format, ...)
{
  va_list args;
  enum rfc_status rfc_status = C_OK;
  char buf[1024];

  va_start(args, format);
  fc_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  switch (level) {
  case LOG_FATAL:
    // Special case - will quit the server.
    qFatal("%s", buf);
    break;
  case LOG_ERROR:
  case LOG_WARN:
    rfc_status = C_WARNING;
    break;
  case LOG_NORMAL:
    rfc_status = C_COMMENT;
    break;
  case LOG_DEBUG:
    rfc_status = C_DEBUG;
    break;
  }

  cmd_reply(CMD_FCDB, lfcl->caller, rfc_status, "%s", buf);
}

/**
 * Registers FCDB-related functions in the Lua state
 */
static void script_fcdb_register_functions()
{
  // "auth" table
  (*fcl)["auth"] = fcl->create_table_with("get_ipaddr", auth_get_ipaddr,
                                          "get_username", auth_get_username);
  // "fcdb" table
  (*fcl)["fcdb"] = fcl->create_table_with(
      "option", fcdb_option_get,
      // Definitions for backward compatibility with Freeciv 2.4.
      // Old database.lua scripts might pass fcdb.param.USER etc to
      // fcdb.option(), but it's deprecated in favour of literal strings, and
      // the strings listed here are only conventional.
      // clang-format off
      "param", fcl->create_table_with("HOST", "host",
                                      "USER", "user",
                                      "PORT", "port",
                                      "PASSWORD", "password",
                                      "DATABASE", "database",
                                      "TABLE_USER", "table_user",
                                      "TABLE_LOG", "table_log",
                                      "BACKEND", "backend")
      // clang-format on
  );
}

/**
   Initialize the scripting state. Returns the status of the freeciv database
   lua state.
 */
bool script_fcdb_init(const QString &fcdb_luafile)
{
  if (fcl != nullptr) {
    return true;
  }

  auto fcdb_luafile_resolved =
      fcdb_luafile.isEmpty() ?
                             // Use default freeciv database lua file.
          QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                 SCRIPT_FCDB_LUA_FILE)
                             : fcdb_luafile;
  if (fcdb_luafile_resolved.isEmpty()) {
    qCritical() << "Could not find " SCRIPT_FCDB_LUA_FILE
                   " in the following directories:"
                << QStandardPaths::standardLocations(
                       QStandardPaths::GenericConfigLocation);
    return false;
  }

  try {
    fcl = new sol::state();
    fcl->open_libraries();

    fcl_compat.state = fcl->lua_state();
    fcl_compat.output_fct = nullptr;
    fcl_compat.caller = nullptr;
    luascript_init(&fcl_compat);

    luascript_common_a(fcl->lua_state());
    tolua_game_open(fcl->lua_state());
    script_fcdb_register_functions();
    luascript_common_z(fcl->lua_state());
  } catch (const std::exception &e) {
    qCritical() << "Error loading the Freeciv21 database lua definition:"
                << e.what();
    script_fcdb_free();
    return false;
  }

  //   luascript_func_init(fcl->lua_state());

  // Define the prototypes for the needed lua functions.
  if (!fcl->safe_script_file(qUtf8Printable(fcdb_luafile_resolved))
           .valid()) {
    qCritical("Error loading the Freeciv21 database lua script '%s'.",
              qUtf8Printable(fcdb_luafile_resolved));
    script_fcdb_free();
    return false;
  }
  script_fcdb_functions_check(qUtf8Printable(fcdb_luafile_resolved));

  if (!script_fcdb_database_init()) {
    qCritical("Error connecting to the database");
    script_fcdb_free();
    return false;
  }
  return true;
}

/**
   Free the scripting data.
 */
void script_fcdb_free()
{
  if (fcl != nullptr) {
    if (!script_fcdb_database_free()) {
      qCritical(
          "Error closing the database connection. Continuing anyway...");
    }

    delete fcl;
    fcl = nullptr;
  }
}

/**
   Parse and execute the script in str in the lua instance for the freeciv
   database.
 */
bool script_fcdb_do_string(struct connection *caller, const char *str)
{
  /* Set a log callback function which allows to send the results of the
   * command to the clients. */
  auto save_caller = fcl_compat.caller;
  auto save_output_fct = fcl_compat.output_fct;
  fcl_compat.output_fct = script_fcdb_cmd_reply;
  fcl_compat.caller = caller;

  return fcl->safe_script(str).valid();

  // Reset the changes.
  fcl_compat.caller = save_caller;
  fcl_compat.output_fct = save_output_fct;

  return true;
}

/**
 * test and initialise the database.
 */
static bool script_fcdb_database_init()
{
  const sol::protected_function database_init = (*fcl)["database_init"];
  auto result = database_init();
  if (result.valid()) {
    return true;
  } else {
    qCritical() << sol::error(result).what();
  }
  return false;
}

/**
 * free the database.
 */
static bool script_fcdb_database_free()
{
  const sol::protected_function database_free = (*fcl)["database_free"];
  auto result = database_free();
  if (result.valid()) {
    return true;
  } else {
    qCritical() << sol::error(result).what();
  }
  return false;
}

/**
 * returns Bool, whether pconn is allowed to delegate player to delegate.
 */
bool script_fcdb_user_delegate_to(connection *pconn, player *pplayer,
                                  const char *delegate, bool &success)
{
  const sol::protected_function user_delegate_to =
      (*fcl)["user_delegate_to"];
  auto result = user_delegate_to(pconn, pplayer, delegate);
  if (result.valid()) {
    success = result;
    return true;
  } else {
    qCritical() << sol::error(result).what();
  }
  return false;
}

/**
 * Check if the user exists.
 */
bool script_fcdb_user_exists(connection *pconn, bool &exists)
{
  const sol::protected_function user_exists = (*fcl)["user_exists"];
  auto result = user_exists(pconn);
  if (result.valid()) {
    exists = result;
    return true;
  } else {
    qCritical() << sol::error(result).what();
  }
  return false;
}

/**
 * Save a new user.
 */
bool script_fcdb_user_save(connection *pconn, const char *password)
{
  const sol::protected_function user_save = (*fcl)["user_save"];
  auto result = user_save(pconn, password);
  if (result.valid()) {
    return true;
  } else {
    qCritical() << sol::error(result).what();
  }
  return false;
}

/**
 * returns Bool, whether requester is allowed to attach taker to pplayer.
 */
bool script_fcdb_user_take(connection *requester, connection *taker,
                           player *player, bool will_observe, bool &success)
{
  const sol::protected_function user_take = (*fcl)["user_take"];
  auto result = user_take(requester, taker, player, will_observe);
  if (result.valid()) {
    success = result;
    return true;
  } else {
    qCritical() << sol::error(result).what();
  }
  return false;
}

/**
 * Check the credentials of the user.
 */
bool script_fcdb_user_verify(connection *pconn, const char *username,
                             bool &success)
{
  const sol::protected_function user_verify = (*fcl)["user_verify"];
  auto result = user_verify(pconn, username);
  if (result.valid()) {
    success = result;
    return true;
  } else {
    qCritical() << sol::error(result).what();
    success = false;
  }
  return false;
}
