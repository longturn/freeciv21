/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdarg>
#include <cstdlib>
#include <ctime>

// Qt
#include <QStandardPaths>

/* dependencies/lua */
#include "lua.h"
#include "lualib.h"

/* dependencies/tolua */
#include "tolua.h"

/* dependencies/luasql */
#ifdef HAVE_FCDB_MYSQL
#include "ls_mysql.h"
#endif
#ifdef HAVE_FCDB_ODBC
#include "ls_odbc.h"
#endif
#ifdef HAVE_FCDB_POSTGRES
#include "ls_postgres.h"
#endif
#ifdef HAVE_FCDB_SQLITE3
#include "ls_sqlite3.h"
#endif

#include <QCryptographicHash>

// utility
#include "log.h"
#include "registry.h"

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"
#include "tolua_common_a_gen.h"
#include "tolua_common_z_gen.h"
#include "tolua_game_gen.h"

// server
#include "console.h"
#include "stdinhand.h"

/* server/scripting */
#include "tolua_fcdb_gen.h"

#include "script_fcdb.h"

#define SCRIPT_FCDB_LUA_FILE "freeciv21/database.lua"

static void script_fcdb_functions_define(void);
static bool script_fcdb_functions_check(const char *fcdb_luafile);

static void script_fcdb_cmd_reply(struct fc_lua *lfcl, QtMsgType level,
                                  const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

/**
   Lua virtual machine state.
 */
static struct fc_lua *fcl = NULL;

/**
   Add fcdb callback functions; these must be defined in the lua script
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
   user_log(Connection pconn, Bool success):
     - check if the login attempt was successful logged.
   user_delegate_to(Connection pconn, Player pplayer, String delegate):
     - returns Bool, whether pconn is allowed to delegate player to delegate.
   user_take(Connection requester, Connection taker, Player pplayer,
             Bool observer):
     - returns Bool, whether requester is allowed to attach taker to pplayer.

   If an error occurred, the functions return a non-NULL string error message
   as the last return value.
 */
static void script_fcdb_functions_define(void)
{
  luascript_func_add(fcl, "database_init", true, 0, 0);
  luascript_func_add(fcl, "database_free", true, 0, 0);

  luascript_func_add(fcl, "user_exists", true, 1, 1, API_TYPE_CONNECTION,
                     API_TYPE_BOOL);
  luascript_func_add(fcl, "user_verify", true, 2, 1, API_TYPE_CONNECTION,
                     API_TYPE_STRING, API_TYPE_BOOL);
  luascript_func_add(fcl, "user_save", false, 2, 0, API_TYPE_CONNECTION,
                     API_TYPE_STRING);
  luascript_func_add(fcl, "user_log", true, 2, 0, API_TYPE_CONNECTION,
                     API_TYPE_BOOL);
  luascript_func_add(fcl, "user_delegate_to", false, 3, 1,
                     API_TYPE_CONNECTION, API_TYPE_PLAYER, API_TYPE_STRING,
                     API_TYPE_BOOL);
  luascript_func_add(fcl, "user_take", false, 4, 1, API_TYPE_CONNECTION,
                     API_TYPE_CONNECTION, API_TYPE_PLAYER, API_TYPE_BOOL,
                     API_TYPE_BOOL);
}

/**
   Check the existence of all needed functions.
 */
static bool script_fcdb_functions_check(const char *fcdb_luafile)
{
  bool ret = true;
  QVector<QString> *missing_func_required = new QVector<QString>;
  QVector<QString> *missing_func_optional = new QVector<QString>;

  if (!luascript_func_check(fcl, missing_func_required,
                            missing_func_optional)) {
    for (auto func_name : *missing_func_required) {
      qCritical("Database script '%s' does not define the required function "
                "'%s'.",
                fcdb_luafile, qUtf8Printable(func_name));
      ret = false;
    }
    for (auto func_name : *missing_func_optional) {
      qDebug("Database script '%s' does not define the optional "
             "function '%s'.",
             fcdb_luafile, qUtf8Printable(func_name));
    }
  }

  delete missing_func_required;
  delete missing_func_optional;

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
   Initialize the scripting state. Returns the status of the freeciv database
   lua state.
 */
bool script_fcdb_init(const QString &fcdb_luafile)
{
  if (fcl != NULL) {
    fc_assert_ret_val(fcl->state != NULL, false);

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

  fcl = luascript_new(NULL, false);
  if (fcl == NULL) {
    qCritical("Error loading the Freeciv database lua definition.");
    return false;
  }

  tolua_common_a_open(fcl->state);
  tolua_game_open(fcl->state);
  tolua_fcdb_open(fcl->state);
#ifdef HAVE_FCDB_MYSQL
  luaL_requiref(fcl->state, "ls_mysql", luaopen_luasql_mysql, 1);
  lua_pop(fcl->state, 1);
#endif
#ifdef HAVE_FCDB_ODBC
  luaL_requiref(fcl->state, "ls_odbc", luaopen_luasql_odbc, 1);
  lua_pop(fcl->state, 1);
#endif
#ifdef HAVE_FCDB_POSTGRES
  luaL_requiref(fcl->state, "ls_postgres", luaopen_luasql_postgres, 1);
  lua_pop(fcl->state, 1);
#endif
#ifdef HAVE_FCDB_SQLITE3
  luaL_requiref(fcl->state, "ls_sqlite3", luaopen_luasql_sqlite3, 1);
  lua_pop(fcl->state, 1);
#endif
  tolua_common_z_open(fcl->state);

  luascript_func_init(fcl);

  // Define the prototypes for the needed lua functions.
  script_fcdb_functions_define();

  if (luascript_do_file(fcl, qUtf8Printable(fcdb_luafile_resolved))
      || !script_fcdb_functions_check(
          qUtf8Printable(fcdb_luafile_resolved))) {
    qCritical("Error loading the Freeciv database lua script '%s'.",
              qUtf8Printable(fcdb_luafile_resolved));
    script_fcdb_free();
    return false;
  }

  if (!script_fcdb_call("database_init")) {
    qCritical("Error connecting to the database");
    script_fcdb_free();
    return false;
  }
  return true;
}

/**
   Call a lua function.

   Example call to the lua function 'user_load()':
     success = script_fcdb_call("user_load", pconn);
 */
bool script_fcdb_call(const char *func_name, ...)
{
  bool success = true;

  va_list args;
  va_start(args, func_name);

  success = luascript_func_call_valist(fcl, func_name, args);
  va_end(args);

  return success;
}

/**
   Free the scripting data.
 */
void script_fcdb_free()
{
  if (!script_fcdb_call("database_free", 0)) {
    qCritical("Error closing the database connection. Continuing anyway...");
  }

  if (fcl) {
    // luascript_func_free() is called by luascript_destroy().
    luascript_destroy(fcl);
    fcl = NULL;
  }
}

/**
   Parse and execute the script in str in the lua instance for the freeciv
   database.
 */
bool script_fcdb_do_string(struct connection *caller, const char *str)
{
  int status;
  struct connection *save_caller;
  luascript_log_func_t save_output_fct;

  /* Set a log callback function which allows to send the results of the
   * command to the clients. */
  save_caller = fcl->caller;
  save_output_fct = fcl->output_fct;
  fcl->output_fct = script_fcdb_cmd_reply;
  fcl->caller = caller;

  status = luascript_do_string(fcl, str, "cmd");

  // Reset the changes.
  fcl->caller = save_caller;
  fcl->output_fct = save_output_fct;

  return (status == 0);
}
