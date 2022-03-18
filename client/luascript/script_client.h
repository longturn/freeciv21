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

/* common/scriptcore */
#include "luascript_signal.h"
#include "luascript_types.h"

struct section_file;

// script functions.
bool script_client_init();
void script_client_free();
bool script_client_do_string(const char *str);
bool script_client_do_file(const char *filename);

/* script state i/o. */
void script_client_state_load(struct section_file *file);
void script_client_state_save(struct section_file *file);

fc_lua *client_lua();

#define CLIENT_SIGNAL(name, ...)                                            \
  namespace client_signals {                                                \
  const Signal<__VA_ARGS__> name(#name, client_lua);                        \
  }
