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

/* dependencies/lua */
extern "C" {
#include "lua.h"
#include "lualib.h"
}

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"

#include "luascript_func.h"

static struct luascript_func *func_new(bool required, int nargs,
                                       enum api_types *parg_types,
                                       int nreturns,
                                       enum api_types *preturn_types);
static void func_destroy(struct luascript_func *pfunc);

struct luascript_func {
  bool required;                // if this function is required
  int nargs;                    // number of arguments to pass
  int nreturns;                 // number of return values to accept
  enum api_types *arg_types;    // argument types
  enum api_types *return_types; // return types
};

/**
   Create a new function definition.
 */
static struct luascript_func *func_new(bool required, int nargs,
                                       enum api_types *parg_types,
                                       int nreturns,
                                       enum api_types *preturn_types)
{
  auto *pfunc = new luascript_func;

  pfunc->required = required;
  pfunc->nargs = nargs;
  pfunc->arg_types = parg_types;
  pfunc->nreturns = nreturns;
  pfunc->return_types = preturn_types;

  return pfunc;
}

/**
   Free a function definition.
 */
static void func_destroy(struct luascript_func *pfunc)
{
  if (pfunc->arg_types) {
    delete[] pfunc->arg_types;
    delete[] pfunc->return_types;
  }
  delete pfunc;
}

/**
   Test if all function are defines. If it fails (return value FALSE), the
   missing functions are listed in 'missing_func_required' and
   'missing_func_optional'.
 */
bool luascript_func_check(struct fc_lua *fcl,
                          QVector<QString> *missing_func_required,
                          QVector<QString> *missing_func_optional)
{
  bool ret = true;

  fc_assert_ret_val(fcl, false);
  fc_assert_ret_val(fcl->funcs, false);

  for (auto const &qfunc_name : fcl->funcs->keys()) {
    char *func_name = qfunc_name.toLocal8Bit().data();
    if (!luascript_check_function(fcl, func_name)) {
      fc_assert_ret_val(fcl->funcs->contains(func_name), false);
      auto *pfunc = fcl->funcs->value(func_name);
      if (pfunc->required) {
        missing_func_required->append(func_name);
      } else {
        missing_func_optional->append(func_name);
      }

      ret = false;
    }
  }

  return ret;
}

/**
   Add a lua function.
 */
void luascript_func_add_valist(struct fc_lua *fcl, const char *func_name,
                               bool required, int nargs, int nreturns,
                               va_list args)
{
  struct luascript_func *pfunc;
  api_types *parg_types;
  api_types *pret_types;
  int i;

  fc_assert_ret(fcl);
  fc_assert_ret(fcl->funcs);

  if (fcl->funcs->contains(func_name)) {
    luascript_log(fcl, LOG_ERROR, "Function '%s' was already created.",
                  func_name);
    return;
  }
  pfunc = fcl->funcs->value(func_name);
  parg_types = new api_types[nargs]();
  pret_types = new api_types[nreturns]();

  for (i = 0; i < nargs; i++) {
    *(parg_types + i) = api_types(va_arg(args, int));
  }

  for (i = 0; i < nreturns; i++) {
    *(pret_types + i) = api_types(va_arg(args, int));
  }

  pfunc = func_new(required, nargs, parg_types, nreturns, pret_types);
  fcl->funcs->insert(func_name, pfunc);
}

/**
   Add a lua function.
 */
void luascript_func_add(struct fc_lua *fcl, const char *func_name,
                        bool required, int nargs, int nreturns, ...)
{
  va_list args;

  va_start(args, nreturns);
  luascript_func_add_valist(fcl, func_name, required, nargs, nreturns, args);
  va_end(args);
}

/**
   Free the function definitions.
 */
void luascript_func_free(struct fc_lua *fcl)
{
  if (fcl && fcl->funcs) {
    for (auto *a : qAsConst(*fcl->funcs)) {
      func_destroy(a);
    }
    delete fcl->funcs;
    fcl->funcs = nullptr;
  }
}

/**
   Initialize the structures needed to save functions definitions.
 */
void luascript_func_init(struct fc_lua *fcl)
{
  fc_assert_ret(fcl != nullptr);

  if (fcl->funcs == nullptr) {
    // Define the prototypes for the needed lua functions.
    fcl->funcs = new QHash<QString, luascript_func *>;
  }
}

/**
   Call a lua function; return value is TRUE if no errors occurred, otherwise
   FALSE.

   Example call to the lua function 'user_load()':
     success = luascript_func_call(L, "user_load", pconn, &password);
 */
bool luascript_func_call_valist(struct fc_lua *fcl, const char *func_name,
                                va_list args)
{
  struct luascript_func *pfunc;
  bool success = false;

  fc_assert_ret_val(fcl, false);
  fc_assert_ret_val(fcl->state, false);
  fc_assert_ret_val(fcl->funcs, false);

  if (!(fcl->funcs->contains(func_name))) {
    luascript_log(fcl, LOG_ERROR,
                  "Lua function '%s' does not exist, "
                  "so cannot be invoked.",
                  func_name);
    return false;
  }
  pfunc = fcl->funcs->value(func_name);
  // The function name
  lua_getglobal(fcl->state, func_name);

  if (!lua_isfunction(fcl->state, -1)) {
    if (pfunc->required) {
      // This function is required. Thus, this is an error.
      luascript_log(fcl, LOG_ERROR, "Unknown lua function '%s'", func_name);
      lua_pop(fcl->state, 1);
    }
    return false;
  }

  luascript_push_args(fcl, pfunc->nargs, pfunc->arg_types, args);

  // Call the function with nargs arguments, return 1 results
  if (luascript_call(fcl, pfunc->nargs, pfunc->nreturns, nullptr) == 0) {
    // Successful call to the script.
    success = true;

    luascript_pop_returns(fcl, func_name, pfunc->nreturns,
                          pfunc->return_types, args);
  }

  return success;
}

/**
   Call a lua function; return value is TRUE if no errors occurred, otherwise
   FALSE.

   Example call to the lua function 'user_load()':
     success = luascript_func_call(L, "user_load", pconn, &password);
 */
bool luascript_func_call(struct fc_lua *fcl, const char *func_name, ...)
{
  va_list args;
  bool success;

  va_start(args, func_name);
  success = luascript_func_call_valist(fcl, func_name, args);
  va_end(args);

  return success;
}

/**
   Return iff the function is required.
 */
bool luascript_func_is_required(struct fc_lua *fcl, const char *func_name)
{
  struct luascript_func *pfunc;

  fc_assert_ret_val(fcl, false);
  fc_assert_ret_val(fcl->state, false);
  fc_assert_ret_val(fcl->funcs, false);

  if (!fcl->funcs->contains(func_name)) {
    luascript_log(fcl, LOG_ERROR, "Lua function '%s' does not exist.",
                  func_name);
    return false;
  }
  pfunc = fcl->funcs->value(func_name);
  return pfunc->required;
}
