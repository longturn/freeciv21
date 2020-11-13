/*****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/
#pragma once
/* utility */
#include "support.h"

#include "luascript_types.h"
struct fc_lua;

typedef char *signal_deprecator;

/* Signal datastructure. */
struct signal {
  int nargs;                              /* number of arguments to pass */
  enum api_types *arg_types;              /* argument types */
  struct signal_callback_list *callbacks; /* connected callbacks */
  char *depr_msg; /* deprecation message to show if handler added */
};

/* Signal callback datastructure. */
struct signal_callback {
  char *name; /* callback function name */
};

void luascript_signal_init(struct fc_lua *fcl);
void luascript_signal_free(struct fc_lua *fcl);

void luascript_signal_emit_valist(struct fc_lua *fcl,
                                  const char *signal_name, va_list args);
void luascript_signal_emit(struct fc_lua *fcl, const char *signal_name, ...);
signal_deprecator *luascript_signal_create(struct fc_lua *fcl,
                                           const char *signal_name,
                                           int nargs, ...);
void deprecate_signal(signal_deprecator *deprecator, char *signal_name,
                      char *replacement, char *deprecated_since);
void luascript_signal_callback(struct fc_lua *fcl, const char *signal_name,
                               const char *callback_name, bool create);
bool luascript_signal_callback_defined(struct fc_lua *fcl,
                                       const char *signal_name,
                                       const char *callback_name);

const QString& luascript_signal_by_index(struct fc_lua *fcl, int sindex);
const char * luascript_signal_callback_by_index(struct fc_lua *fcl,
                                               const char *signal_name,
                                               int sindex);
