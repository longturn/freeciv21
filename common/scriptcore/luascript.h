/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <QHash>
#include <QVector>
/* dependencies/tolua */
#include "tolua.h"

// utility
#include "log.h"
#include "support.h" // fc__attribute()

/* common/scriptcore */
#include "luascript_func.h"
#include "luascript_signal.h"
#include "luascript_types.h"

struct section_file;
struct luascript_func_hash;
struct luascript_signal_hash;
struct luascript_signal_name_list;
struct connection;
struct fc_lua;

typedef void (*luascript_log_func_t)(struct fc_lua *fcl, QtMsgType level,
                                     const char *format, ...)
    fc__attribute((__format__(__printf__, 3, 4)));

struct fc_lua {
  lua_State *state;

  luascript_log_func_t output_fct;
  // This is needed for server 'lua' and 'luafile' commands.
  struct connection *caller;

  QHash<QString, struct luascript_func *> *funcs;

  QHash<QString, struct signal *> *signals_hash;
  QVector<QString> *signal_names;
};

// Error functions for lua scripts.
int luascript_error(lua_State *L, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)));
int luascript_error_vargs(lua_State *L, const char *format, va_list vargs);
int luascript_arg_error(lua_State *L, int narg, const char *msg);

/* Create / destroy a freeciv lua instance. */
struct fc_lua *luascript_new(luascript_log_func_t outputfct,
                             bool secured_environment);
struct fc_lua *luascript_get_fcl(lua_State *L);
void luascript_destroy(struct fc_lua *fcl);

void luascript_log(struct fc_lua *fcl, QtMsgType level, const char *format,
                   ...) fc__attribute((__format__(__printf__, 3, 4)));
void luascript_log_vargs(struct fc_lua *fcl, QtMsgType level,
                         const char *format, va_list args);

void luascript_push_args(struct fc_lua *fcl, int nargs,
                         enum api_types *parg_types, va_list args);
void luascript_pop_returns(struct fc_lua *fcl, const char *func_name,
                           int nreturns, enum api_types *preturn_types,
                           va_list args);
bool luascript_check_function(struct fc_lua *fcl, const char *funcname);

int luascript_call(struct fc_lua *fcl, int narg, int nret, const char *code);
int luascript_do_string(struct fc_lua *fcl, const char *str,
                        const char *name);
int luascript_do_file(struct fc_lua *fcl, const char *filename);

// Callback invocation function.
bool luascript_callback_invoke(struct fc_lua *fcl, const char *callback_name,
                               int nargs, enum api_types *parg_types,
                               va_list args);

void luascript_remove_exported_object(struct fc_lua *fcl, void *object);

/* Load / save variables. */
void luascript_vars_save(struct fc_lua *fcl, struct section_file *file,
                         const char *section);
void luascript_vars_load(struct fc_lua *fcl, struct section_file *file,
                         const char *section);

const Direction *luascript_dir(enum direction8);

// Script assertion (for debugging only)
#ifdef FREECIV_DEBUG
#define LUASCRIPT_ASSERT(L, check, ...)                                     \
  if (!(check)) {                                                           \
    luascript_error(L, "in %s() [%s::%d]: the assertion '%s' failed.",      \
                    __FUNCTION__, __FILE__, __FC_LINE__, #check);           \
    return __VA_ARGS__;                                                     \
  }
#else
#define LUASCRIPT_ASSERT(check, ...)
#endif

#define LUASCRIPT_CHECK_STATE(L, ...)                                       \
  if (!L) {                                                                 \
    qCritical("No lua state available");                                    \
    return __VA_ARGS__;                                                     \
  }

// script_error on failed check
#define LUASCRIPT_CHECK(L, check, msg, ...)                                 \
  if (!(check)) {                                                           \
    luascript_error(L, msg);                                                \
    return __VA_ARGS__;                                                     \
  }

// script_arg_error on failed check
#define LUASCRIPT_CHECK_ARG(L, check, narg, msg, ...)                       \
  if (!(check)) {                                                           \
    luascript_arg_error(L, narg, msg);                                      \
    return __VA_ARGS__;                                                     \
  }

// script_arg_error on nil value
#define LUASCRIPT_CHECK_ARG_NIL(L, value, narg, type, ...)                  \
  if ((value) == NULL) {                                                    \
    luascript_arg_error(L, narg, "got 'nil', '" #type "' expected");        \
    return __VA_ARGS__;                                                     \
  }

/* script_arg_error on nil value. The first argument is the lua state and the
 * second is the pointer to self. */
#define LUASCRIPT_CHECK_SELF(L, value, ...)                                 \
  if ((value) == NULL) {                                                    \
    luascript_arg_error(L, 2, "got 'nil' for self");                        \
    return __VA_ARGS__;                                                     \
  }
