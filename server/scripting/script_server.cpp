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
#include <sys/stat.h>

/* dependencies/lua */
extern "C" {
#include "lua.h"
#include "lualib.h"
}

// Sol
#include "sol/sol.hpp"

// utility
#include "log.h"
#include "registry.h"

/* common/scriptcore */
#include "api_game_find.h"
#include "api_game_methods.h"
#include "api_game_specenum.h"
#include "api_signal_base.h"
#include "luascript.h"
#include "luascript_signal.h"

// server
#include "console.h"
#include "stdinhand.h"

/* server/scripting */
#include "api_server_edit.h"
#include "api_server_game_methods.h"
#include "api_server_notify.h"

#include "script_server.h"

/**
   Lua virtual machine states.
 */
static struct fc_lua *fcl_unsafe = nullptr;

fc_lua *server_fcl_main()
{
  static fc_lua *fcl_main = luascript_new(nullptr, true);
  return fcl_main;
}

/**
   Optional game script code (useful for scenarios).
 */
static char *script_server_code = nullptr;

static void script_server_vars_init();
static void script_server_vars_free();
static void script_server_vars_load(struct section_file *file);
static void script_server_vars_save(struct section_file *file);
static void script_server_code_init();
static void script_server_code_free();
static void script_server_code_load(struct section_file *file);
static void script_server_code_save(struct section_file *file);

static void script_server_cmd_reply(struct fc_lua *fcl, QtMsgType level,
                                    const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

/**
   Parse and execute the script in str in the context of the specified
   instance.
 */
static bool script_server_do_string_shared(struct fc_lua *fcl,
                                           struct connection *caller,
                                           const char *str)
{
  int status;
  struct connection *save_caller;
  luascript_log_func_t save_output_fct;

  /* Set a log callback function which allows to send the results of the
   * command to the clients. */
  save_caller = fcl->caller;
  save_output_fct = fcl->output_fct;
  fcl->output_fct = script_server_cmd_reply;
  fcl->caller = caller;

  status = luascript_do_string(fcl, str, "cmd");

  // Reset the changes.
  fcl->caller = save_caller;
  fcl->output_fct = save_output_fct;

  return (status == 0);
}

/**
   Parse and execute the script in str in the same instance as the ruleset
 */
bool script_server_do_string(struct connection *caller, const char *str)
{
  return script_server_do_string_shared(server_fcl_main(), caller, str);
}

/**
   Parse and execute the script in str in an unsafe instance
 */
bool script_server_unsafe_do_string(struct connection *caller,
                                    const char *str)
{
  return script_server_do_string_shared(fcl_unsafe, caller, str);
}

/**
   Load script to a buffer
 */
bool script_server_load_file(const char *filename, char **buf)
{
  FILE *ffile;
  struct stat stats;
  char *buffer;

  fc_stat(filename, &stats);
  ffile = fc_fopen(filename, "r");

  if (ffile != nullptr) {
    int len;

    buffer = new char[stats.st_size + 1];
    len = fread(buffer, 1, stats.st_size, ffile);

    if (len == stats.st_size) {
      buffer[len] = '\0';

      *buf = buffer;
    } else {
      delete[] buffer;
    }
    fclose(ffile);
  }

  return true;
}

/**
   Parse and execute the script at filename in the context of the specified
   instance.
 */
static bool script_server_do_file_shared(struct fc_lua *fcl,
                                         struct connection *caller,
                                         const char *filename)
{
  int status = 1;
  struct connection *save_caller;
  luascript_log_func_t save_output_fct;

  /* Set a log callback function which allows to send the results of the
   * command to the clients. */
  save_caller = fcl->caller;
  save_output_fct = fcl->output_fct;
  fcl->output_fct = script_server_cmd_reply;
  fcl->caller = caller;

  status = luascript_do_file(fcl, filename);

  // Reset the changes.
  fcl->caller = save_caller;
  fcl->output_fct = save_output_fct;

  return (status == 0);
}

/**
   Parse and execute the script at filename in the same instance as the
   ruleset.
 */
bool script_server_do_file(struct connection *caller, const char *filename)
{
  return script_server_do_file_shared(server_fcl_main(), caller, filename);
}

/**
   Parse and execute the script at filename in an unsafe instance.
 */
bool script_server_unsafe_do_file(struct connection *caller,
                                  const char *filename)
{
  return script_server_do_file_shared(fcl_unsafe, caller, filename);
}

/**
   Initialize the game script variables.
 */
static void script_server_vars_init()
{ // nothing
}

/**
   Free the game script variables.
 */
static void script_server_vars_free()
{ // nothing
}

/**
   Load the game script variables in file.
 */
static void script_server_vars_load(struct section_file *file)
{
  luascript_vars_load(server_fcl_main(), file, "script.vars");
}

/**
   Save the game script variables to file.
 */
static void script_server_vars_save(struct section_file *file)
{
  luascript_vars_save(server_fcl_main(), file, "script.vars");
}

/**
   Initialize the optional game script code (useful for scenarios).
 */
static void script_server_code_init() { script_server_code = nullptr; }

/**
   Free the optional game script code (useful for scenarios).
 */
static void script_server_code_free()
{
  if (script_server_code) {
    FCPP_FREE(script_server_code);
  }
}

/**
   Load the optional game script code from file (useful for scenarios).
 */
static void script_server_code_load(struct section_file *file)
{
  if (!script_server_code) {
    const char *code;
    const char *section = "script.code";

    code = secfile_lookup_str_default(file, "", "%s", section);
    script_server_code = fc_strdup(code);
    luascript_do_string(server_fcl_main(), script_server_code, section);
  }
}

/**
   Save the optional game script code to file (useful for scenarios).
 */
static void script_server_code_save(struct section_file *file)
{
  if (script_server_code) {
    secfile_insert_str_noescape(file, script_server_code, "script.code");
  }
}

/**
   Initialize the scripting state.
 */
bool script_server_init()
{
  auto fcl_main = server_fcl_main();

  sol::state_view main_view(fcl_main->state);
  setup_game_methods(main_view);
  setup_server_methods(main_view);

  luascript_common_a(fcl_main->state);
  api_specenum_open(fcl_main->state);
  setup_lua_find(main_view);

  setup_lua_signal(fcl_main->state);

  luascript_common_z(fcl_main->state);

  script_server_code_init();
  script_server_vars_init();

  setup_server_edit(main_view);
  setup_server_notify(main_view);

  // Add the unsafe instance.
  fcl_unsafe = luascript_new(nullptr, false);
  if (fcl_unsafe == nullptr) {
    luascript_destroy(fcl_unsafe);
    fcl_unsafe = nullptr;

    return false;
  }

  sol::state_view unsafe_view(fcl_unsafe->state);
  setup_game_methods(unsafe_view);
  setup_server_methods(unsafe_view);

  luascript_common_a(fcl_unsafe->state);
  api_specenum_open(fcl_unsafe->state);
  setup_lua_find(unsafe_view);

  luascript_common_z(fcl_unsafe->state);

  setup_server_edit(unsafe_view);
  setup_server_notify(unsafe_view);

  return true;
}

/**
   Free the scripting data.
 */
void script_server_free()
{
  auto fcl_main = server_fcl_main();
  if (fcl_main != nullptr) {
    script_server_code_free();
    script_server_vars_free();
  }

  if (fcl_unsafe != nullptr) {
    // luascript_signal_free() is called by luascript_destroy().
    luascript_destroy(fcl_unsafe);
    fcl_unsafe = nullptr;
  }
}

/**
   Load the scripting state from file.
 */
void script_server_state_load(struct section_file *file)
{
  script_server_code_load(file);

  /* Variables must be loaded after code is loaded and executed,
   * so we restore their saved state properly */
  script_server_vars_load(file);
}

/**
   Save the scripting state to file.
 */
void script_server_state_save(struct section_file *file)
{
  script_server_code_save(file);
  script_server_vars_save(file);
}

/**
   Send the message via cmd_reply().
 */
static void script_server_cmd_reply(struct fc_lua *fcl, QtMsgType level,
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
  case LOG_VERBOSE:
    rfc_status = C_LOG_BASE;
    break;
  }

  cmd_reply(CMD_LUA, fcl->caller, rfc_status, "%s", buf);
}
