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

// Sol
#include "sol/sol.hpp"

extern "C" {
/* dependencies/lua */
#include "lua.h"
#include "lualib.h"
}

// utility
#include "log.h"

// common
#include "featured_text.h"

/* common/scriptcore */
#include "api_game_methods.h"
#include "api_game_specenum.h"
#include "api_signal_base.h"
#include "luascript.h"
#include "luascript_signal.h"

// client
#include "chatline_common.h"
#include "luaconsole_common.h"

/* client/luascript */
#include "script_client.h"

/*****************************************************************************
  Optional game script code (useful for scenarios).
*****************************************************************************/
static char *script_client_code = nullptr;

static void script_client_vars_init();
static void script_client_vars_free();
static void script_client_vars_load(struct section_file *file);
static void script_client_vars_save(struct section_file *file);
static void script_client_code_init();
static void script_client_code_free();
static void script_client_code_load(struct section_file *file);
static void script_client_code_save(struct section_file *file);

static void script_client_output(struct fc_lua *fcl, QtMsgType level,
                                 const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

/*****************************************************************************
  Lua virtual machine state.
*****************************************************************************/

fc_lua *client_lua()
{
  static fc_lua *main = luascript_new(script_client_output, true);
  return main;
}

/**
   Parse and execute the script in str
 */
bool script_client_do_string(const char *str)
{
  int status = luascript_do_string(client_lua(), str, "cmd");

  return (status == 0);
}

/**
   Parse and execute the script at filename.
 */
bool script_client_do_file(const char *filename)
{
  int status = luascript_do_file(client_lua(), filename);

  return (status == 0);
}

/**
   Initialize the game script variables.
 */
static void script_client_vars_init()
{ // nothing
}

/**
   Free the game script variables.
 */
static void script_client_vars_free()
{ // nothing
}

/**
   Load the game script variables in file.
 */
static void script_client_vars_load(struct section_file *file)
{
  luascript_vars_load(client_lua(), file, "script.vars");
}

/**
   Save the game script variables to file.
 */
static void script_client_vars_save(struct section_file *file)
{
  luascript_vars_save(client_lua(), file, "script.vars");
}

/**
   Initialize the optional game script code (useful for scenarios).
 */
static void script_client_code_init() { script_client_code = nullptr; }

/**
   Free the optional game script code (useful for scenarios).
 */
static void script_client_code_free() { NFCN_FREE(script_client_code); }

/**
   Load the optional game script code from file (useful for scenarios).
 */
static void script_client_code_load(struct section_file *file)
{
  if (!script_client_code) {
    const char *code;
    const char *section = "script.code";

    code = secfile_lookup_str_default(file, "", "%s", section);
    script_client_code = fc_strdup(code);
    luascript_do_string(client_lua(), script_client_code, section);
  }
}

/**
   Save the optional game script code to file (useful for scenarios).
 */
static void script_client_code_save(struct section_file *file)
{
  if (script_client_code) {
    secfile_insert_str_noescape(file, script_client_code, "script.code");
  }
}

namespace {

void chat_base(const char *msg)
{
  output_window_printf(ftc_chat_luaconsole, "%s", msg);
}

void setup_client(sol::state_view lua)
{
  auto chat = lua["chat"].get_or_create<sol::table>();
  chat.set("base", chat_base);

  lua.script(R"(
function chat.msg(fmt, ...)
  chat.base(string.format(fmt, ...))
end
  )");
}

} // namespace

/**
   Initialize the scripting state.
 */
bool script_client_init()
{
  auto main_fcl = client_lua();

  sol::state_view main_view(main_fcl->state);
  setup_game_methods(main_view);
  setup_lua_signal(main_fcl->state);

  setup_client(main_view);

  luascript_common_a(main_fcl->state);
  api_specenum_open(main_fcl->state);
  setup_lua_signal(main_fcl->state);

  luascript_common_z(main_fcl->state);

  script_client_code_init();
  script_client_vars_init();

  return true;
}

/**
   Ouput a message on the client lua console.
 */
static void script_client_output(struct fc_lua *fcl, QtMsgType level,
                                 const char *format, ...)
{
  Q_UNUSED(fcl)
  va_list args;
  struct ft_color ftc_luaconsole = ftc_luaconsole_error;

  switch (level) {
  case LOG_FATAL:
    // Special case - will quit the client.
    {
      char buf[1024];

      va_start(args, format);
      fc_vsnprintf(buf, sizeof(buf), format, args);
      va_end(args);

      qFatal("%s", buf);
    }
    break;
  case LOG_ERROR:
    ftc_luaconsole = ftc_luaconsole_error;
    break;
  case LOG_WARN:
    ftc_luaconsole = ftc_luaconsole_warn;
    break;
  case LOG_NORMAL:
    ftc_luaconsole = ftc_luaconsole_normal;
    break;
  case LOG_VERBOSE:
    ftc_luaconsole = ftc_luaconsole_verbose;
    break;
  }

  va_start(args, format);
  luaconsole_vprintf(ftc_luaconsole, format, args);
  va_end(args);
}

/**
   Free the scripting data.
 */
void script_client_free()
{
  auto main_fcl = client_lua();
  if (main_fcl != nullptr) {
    script_client_code_free();
    script_client_vars_free();

    luascript_destroy(main_fcl);
  }
}

/**
   Load the scripting state from file.
 */
void script_client_state_load(struct section_file *file)
{
  script_client_code_load(file);

  /* Variables must be loaded after code is loaded and executed,
   * so we restore their saved state properly */
  script_client_vars_load(file);
}

/**
   Save the scripting state to file.
 */
void script_client_state_save(struct section_file *file)
{
  script_client_code_save(file);
  script_client_vars_save(file);
}
