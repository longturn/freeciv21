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
#include <QFile>

/* dependencies/lua */
extern "C" {
#include "lua.h"
#include "lualib.h"
}

// Sol
#include "sol/sol.hpp"

// utility
#include "fcintl.h"
#include "log.h"
#include "registry.h"

// common
#include "map.h"

/* common/scriptcore */
#include "api_common_intl.h"
#include "api_common_utilities.h"
#include "luascript_signal.h"

#include "luascript.h"

/*****************************************************************************
  Configuration for script execution time limits. Checkinterval is the
  number of executed lua instructions between checking. Disabled if 0.
*****************************************************************************/
#define LUASCRIPT_MAX_EXECUTION_TIME_SEC 5.0
#define LUASCRIPT_CHECKINTERVAL 10000

// The name used for the freeciv lua struct saved in the lua state.
#define LUASCRIPT_GLOBAL_VAR_NAME "__fcl"

/*****************************************************************************
  Unsafe Lua builtin symbols that we to remove access to.

  If Freeciv's Lua version changes, you have to check how the set of
  unsafe functions and modules changes in the new version. Update the list of
  loaded libraries in luascript_lualibs, then update the unsafe symbols
  blacklist in luascript_unsafe_symbols.

  Once the variables are updated for the new version, update the value of
  LUASCRIPT_SECURE_LUA_VERSION

  In general, unsafe is all functionality that gives access to:
  * Reading files and running processes
  * Loading lua files or libraries
*****************************************************************************/
#define LUASCRIPT_SECURE_LUA_VERSION1 503
#define LUASCRIPT_SECURE_LUA_VERSION2 504

static const char *luascript_unsafe_symbols_secure[] = {"debug", "dofile",
                                                        "loadfile", nullptr};

static const char *luascript_unsafe_symbols_permissive[] = {
    "debug", "dofile", "loadfile", nullptr};

#if LUA_VERSION_NUM != LUASCRIPT_SECURE_LUA_VERSION1                        \
    && LUA_VERSION_NUM != LUASCRIPT_SECURE_LUA_VERSION2
#warning "The script runtime's unsafe symbols information is not up to date."
#warning "This can be a big security hole!"
#endif

/*****************************************************************************
  Lua libraries to load (all default libraries, excluding operating system
  and library loading modules). See linit.c in Lua 5.1 for the default list.
*****************************************************************************/
#if LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 504
static luaL_Reg luascript_lualibs_secure[] = {
    // Using default libraries excluding: package, io, os, and bit32
    {"_G", luaopen_base},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_DBLIBNAME, luaopen_debug},
    {nullptr, nullptr}};
#else // LUA_VERSION_NUM
#error "Unsupported lua version"
#endif // LUA_VERSION_NUM

static void luascript_openlibs(lua_State *L, const luaL_Reg *llib);
static void luascript_blacklist(lua_State *L, const char *lsymbols[]);

/**
   Open lua libraries in the array of library definitions in llib.
 */
static void luascript_openlibs(lua_State *L, const luaL_Reg *llib)
{
  // set results to global table
  for (; llib->func; llib++) {
    luaL_requiref(L, llib->name, llib->func, 1);
    lua_pop(L, 1); // remove lib
  }
}

/**
   Remove global symbols from lua state L
 */
static void luascript_blacklist(lua_State *L, const char *lsymbols[])
{
  int i;

  for (i = 0; lsymbols[i] != nullptr; i++) {
    lua_pushnil(L);
    lua_setglobal(L, lsymbols[i]);
  }
}

/**
   Internal api error function - varg version.
 */
int luascript_error(lua_State *L, const char *format, ...)
{
  va_list vargs;
  int ret;

  va_start(vargs, format);
  ret = luascript_error_vargs(L, format, vargs);
  va_end(vargs);

  return ret;
}

/**
   Internal api error function.
   Invoking this will cause Lua to stop executing the current context and
   throw an exception, so to speak.
 */
int luascript_error_vargs(lua_State *L, const char *format, va_list vargs)
{
  fc_assert_ret_val(L != nullptr, 0);

  luaL_where(L, 1);
  lua_pushvfstring(L, format, vargs);
  lua_concat(L, 2);

  return lua_error(L);
}

/**
   Like script_error, but using a prefix identifying the called lua function:
     bad argument #narg to '<func>': msg
 */
int luascript_arg_error(lua_State *L, int narg, const char *msg)
{
  return luaL_argerror(L, narg, msg);
}

/**
   Initialize the scripting state.
 */
struct fc_lua *luascript_new(luascript_log_func_t output_fct,
                             bool secured_environment)
{
  fc_lua *fcl = new fc_lua[1]();

  fcl->state = luaL_newstate();
  if (!fcl->state) {
    FCPP_FREE(fcl);
    return nullptr;
  }
  fcl->output_fct = output_fct;
  fcl->caller = nullptr;

  if (secured_environment) {
    luascript_openlibs(fcl->state, luascript_lualibs_secure);
    luascript_blacklist(fcl->state, luascript_unsafe_symbols_secure);
  } else {
    luaL_openlibs(fcl->state);
    luascript_blacklist(fcl->state, luascript_unsafe_symbols_permissive);
  }

  luascript_init(fcl);

  return fcl;
}

/**
   Sets the freeciv lua struct for a lua state.
 */
void luascript_init(fc_lua *fcl)
{
  // Save the freeciv lua struct in the lua state.
  lua_pushstring(fcl->state, LUASCRIPT_GLOBAL_VAR_NAME);
  lua_pushlightuserdata(fcl->state, fcl);
  lua_settable(fcl->state, LUA_REGISTRYINDEX);
}

/**
   Get the freeciv lua struct from a lua state.
 */
struct fc_lua *luascript_get_fcl(lua_State *L)
{
  struct fc_lua *fcl;

  fc_assert_ret_val(L, nullptr);

  // Get the freeciv lua struct from the lua state.
  lua_pushstring(L, LUASCRIPT_GLOBAL_VAR_NAME);
  lua_gettable(L, LUA_REGISTRYINDEX);
  fcl = static_cast<fc_lua *>(lua_touserdata(L, -1));

  // This is an error!
  fc_assert_ret_val(fcl != nullptr, nullptr);

  return fcl;
}

/**
   Free the scripting data.
 */
void luascript_destroy(struct fc_lua *fcl)
{
  if (fcl) {
    fc_assert_ret(fcl->caller == nullptr);

    // Free lua state.
    if (fcl->state) {
      lua_gc(fcl->state, LUA_GCCOLLECT, 0); // Collected garbage
      lua_close(fcl->state);
    }
    delete[] fcl;
  }
}

/**
 * Loads a script from a Qt resource file and executes it.
 */
static void luascript_exec_resource(lua_State *L, const QString &filename)
{
  Q_INIT_RESOURCE(scriptcore);

  QFile in(filename);
  if (!in.open(QFile::ReadOnly)) {
    qCritical() << "Could not find resource:" << in.fileName();
    qFatal("Missing resource");
  }
  const auto data = in.readAll();

  // We trust that it loads.
  sol::state_view lua(L);
  auto result = lua.safe_script(data.data(), in.fileName().toStdString());
  if (!result.valid()) {
    sol::error err = result;
    std::cout << "An error (an expected one) occurred: " << err.what()
              << std::endl;

    return;
  }

  Q_CLEANUP_RESOURCE(scriptcore);
}

/**
 * Registers tolua_common_a functions and modules.
 */
static void luascript_common_a_register(sol::state_view state)
{
  // Intl module
  state["_"] = api_intl__;
  state["N_"] = api_intl_N_;
  state["Q_"] = api_intl_Q_;
  state["PL_"] = api_intl_PL_;

  // log module
  // clang-format off
  sol::table log = state.create_table_with(
      "base", api_utilities_log_base,
      "deprecation_warning", api_utilities_deprecation_warning);
  log.new_enum("level",
      "FATAL",   LOG_FATAL,
      "ERROR",   LOG_ERROR,
      "WARN",    LOG_WARN,
      "NORMAL",  LOG_NORMAL,
      "VERBOSE", LOG_VERBOSE,
      "DEBUG",   LOG_DEBUG);
  // clang-format on
  state["log"] = log;

  // Global functions
  state["random"] = api_utilities_random;
  state["fc_version"] = api_utilities_fc_version;
}

/**
 * Runs tolua_common_a.lua.
 */
void luascript_common_a(lua_State *L)
{
  luascript_common_a_register(L);

  sol::state_view lua(L);
  luascript_exec_resource(L, QStringLiteral(":/lua/tolua_common_a.lua"));
}

/**
 * Runs tolua_common_z.lua.
 */
void luascript_common_z(lua_State *L)
{
  luascript_exec_resource(L, QStringLiteral(":/lua/tolua_common_z.lua"));
}

/**
   Print a message to the selected output handle.
 */
void luascript_log(struct fc_lua *fcl, QtMsgType level, const char *format,
                   ...)
{
  va_list args;

  va_start(args, format);
  luascript_log_vargs(fcl, level, format, args);
  va_end(args);
}

/**
   Print a message to the selected output handle.
 */
void luascript_log_vargs(struct fc_lua *fcl, QtMsgType level,
                         const char *format, va_list args)
{
  char buf[1024];

  fc_assert_ret(fcl);
  fc_assert_ret(level >= QtDebugMsg && level <= QtInfoMsg);

  fc_vsnprintf(buf, sizeof(buf), format, args);

  if (fcl->output_fct) {
    fcl->output_fct(fcl, level, "%s", buf);
  } else {
    log_base(level, "%s", buf);
  }
}

/**
   Return if the function 'funcname' is define in the lua state 'fcl->state'.
 */
bool luascript_check_function(struct fc_lua *fcl, const char *funcname)
{
  bool defined;

  fc_assert_ret_val(fcl, false);
  fc_assert_ret_val(fcl->state, false);

  lua_getglobal(fcl->state, funcname);
  defined = lua_isfunction(fcl->state, -1);
  lua_pop(fcl->state, 1);

  return defined;
}

/**
   lua_dostring replacement with error message showing on errors.
 */
int luascript_do_string(struct fc_lua *fcl, const char *str,
                        const char *name)
{
  fc_assert_ret_val(fcl, 0);
  fc_assert_ret_val(fcl->state, 0);

  sol::state_view lua(fcl->state);

  auto result = lua.safe_script(str);
  if (!result.valid()) {
    sol::error err = result;
    std::cout << "An error (an expected one) occurred: " << err.what()
              << std::endl;
  }
  return !result.valid();
}

/**
   Parse and execute the script at filename.
 */
int luascript_do_file(struct fc_lua *fcl, const char *filename)
{
  fc_assert_ret_val(fcl, 0);
  fc_assert_ret_val(fcl->state, 0);

  sol::state_view lua(fcl->state);

  auto result = lua.safe_script_file(
      filename, [](lua_State *, sol::protected_function_result pfr) {
        // pfr will contain things that went wrong, for either loading or
        // executing the script the user can do whatever they like here,
        // including throw. Otherwise...
        sol::error err = pfr;
        std::cout << "An error (an expected one) occurred: " << err.what()
                  << std::endl;

        // ... they need to return the protected_function_result
        return pfr;
      });
  return !result.valid();
}

/**
   Save lua variables to file.
 */
void luascript_vars_save(struct fc_lua *fcl, struct section_file *file,
                         const char *section)
{
  fc_assert_ret(file);
  fc_assert_ret(fcl);
  fc_assert_ret(fcl->state);

  sol::state_view lua(fcl->state);

  sol::protected_function dump(lua["_freeciv_state_dump"]);
  if (!dump.valid()) {
    return;
  }

  auto result = dump();

  if (!result.valid() || result.get_type() != sol::type::string) {
    // _freeciv_state_dump in tolua_game.pkg is busted
    luascript_log(fcl, LOG_ERROR, "lua error: Failed to dump variables");
    sol::error err = result;
    std::cout << "An error (an expected one) occurred: " << err.what()
              << std::endl;

    return;
  }

  secfile_insert_str_noescape(file, result.get<const char *>(), "%s",
                              section);
}

/**
   Load lua variables from file.
 */
void luascript_vars_load(struct fc_lua *fcl, struct section_file *file,
                         const char *section)
{
  const char *vars;

  fc_assert_ret(file);
  fc_assert_ret(fcl);
  fc_assert_ret(fcl->state);

  vars = secfile_lookup_str_default(file, "", "%s", section);
  sol::state_view lua(fcl->state);
  luascript_do_string(fcl, vars, section);
}

/* FIXME: tolua-5.2 does not create a destructor for dynamically
 * allocated objects in non-C++ code but tries to call it. */
/* Thus, avoid returning any non-basic types. If you need them, put here
 * constructors returning them in lua_Object registering their destructor
 * in tolua system. (None yet.) */

/* To avoid the complexity and overheads of creating such objects for
 * 4-bit enums, here is a helper function to return Direction objects. */
/******
   Returns a pointer to a given value of enum direction8 (always the same
   address for the same value), or nullptr if the direction is invalid
   on the current map.
 */
const Direction *luascript_dir(enum direction8 dir)
{
  static const Direction etalon[8] = {
      DIR8_NORTHWEST, DIR8_NORTH,     DIR8_NORTHEAST, DIR8_WEST,
      DIR8_EAST,      DIR8_SOUTHWEST, DIR8_SOUTH,     DIR8_SOUTHEAST};
  if (is_valid_dir(dir)) {
    return &etalon[dir];
  } else {
    return nullptr;
  }
}
