/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

#include <cstdarg>
#include <ctime>

extern "C" {
/* dependencies/lua */
#include "lua.h"

/* dependencies/tolua */
#include "tolua.h"
}
// utility
#include "log.h"
#include "section_file.h"

// common
#include "featured_text.h"

/* common/scriptcore */
#include "api_game_specenum.h"
#include "luascript.h"

#include "tolua_game_gen.h"
#include "tolua_signal_gen.h"

// client
#include "luaconsole_common.h"

/* client/luascript */
#include "tolua_client_gen.h"

#include "script_client.h"

/**
  Lua virtual machine state.
 */
static struct fc_lua *main_fcl = nullptr;

/**
  Optional game script code (useful for scenarios).
 */
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

static void script_client_signal_create();

/**
   Parse and execute the script in str
 */
bool script_client_do_string(const char *str)
{
  int status = luascript_do_string(main_fcl, str, "cmd");

  return (status == 0);
}

/**
   Parse and execute the script at filename.
 */
bool script_client_do_file(const char *filename)
{
  int status = luascript_do_file(main_fcl, filename);

  return (status == 0);
}

/**
   Invoke the 'callback_name' Lua function.
 */
bool script_client_callback_invoke(const char *callback_name, int nargs,
                                   enum api_types *parg_types, va_list args)
{
  return luascript_callback_invoke(main_fcl, callback_name, nargs,
                                   parg_types, args);
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
  luascript_vars_load(main_fcl, file, "script.vars");
}

/**
   Save the game script variables to file.
 */
static void script_client_vars_save(struct section_file *file)
{
  luascript_vars_save(main_fcl, file, "script.vars");
}

/**
   Initialize the optional game script code (useful for scenarios).
 */
static void script_client_code_init() { script_client_code = nullptr; }

/**
   Free the optional game script code (useful for scenarios).
 */
static void script_client_code_free()
{
  delete[] script_client_code;
  script_client_code = nullptr;
}

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
    luascript_do_string(main_fcl, script_client_code, section);
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

/**
   Initialize the scripting state.
 */
bool script_client_init()
{
  if (main_fcl != nullptr) {
    fc_assert_ret_val(main_fcl->state != nullptr, false);

    return true;
  }

  main_fcl = luascript_new(script_client_output, true);
  if (main_fcl == nullptr) {
    luascript_destroy(main_fcl); // TODO: main_fcl is nullptr here...
    main_fcl = nullptr;

    return false;
  }

  luascript_common_a(main_fcl->state);
  api_specenum_open(main_fcl->state);
  tolua_game_open(main_fcl->state);
  tolua_signal_open(main_fcl->state);

#ifdef MESON_BUILD
  /* Tolua adds 'tolua_' prefix to _open() function names,
   * and we can't pass it a basename where the original
   * 'tolua_' has been stripped when generating from meson. */
  tolua_tolua_client_open(main_fcl->state);
#else  // MESON_BUILD
  tolua_client_open(main_fcl->state);
#endif // MESON_BUILD

  luascript_common_z(main_fcl->state);

  script_client_code_init();
  script_client_vars_init();

  luascript_signal_init(main_fcl);
  script_client_signal_create();

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
  if (main_fcl != nullptr) {
    script_client_code_free();
    script_client_vars_free();

    luascript_signal_free(main_fcl);

    luascript_destroy(main_fcl);
    main_fcl = nullptr;
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

/**
   Invoke all the callback functions attached to a given signal.
 */
void script_client_signal_emit(const char *signal_name, ...)
{
  va_list args;

  va_start(args, signal_name);
  luascript_signal_emit_valist(main_fcl, signal_name, args);
  va_end(args);
}

/**
   Declare any new signal types you need here.
 */
static void script_client_signal_create()
{
  luascript_signal_create(main_fcl, "new_tech", 0);
}
