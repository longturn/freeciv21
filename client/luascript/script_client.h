/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifndef FC__SCRIPT_CLIENT_H
#define FC__SCRIPT_CLIENT_H



/* common/scriptcore */
#include "luascript_types.h"

struct section_file;

/* callback invocation function. */
bool script_client_callback_invoke(const char *callback_name, int nargs,
                                   enum api_types *parg_types, va_list args);

void script_client_remove_exported_object(void *object);

/* script functions. */
bool script_client_init(void);
void script_client_free(void);
bool script_client_do_string(const char *str);
bool script_client_do_file(const char *filename);

/* script state i/o. */
void script_client_state_load(struct section_file *file);
void script_client_state_save(struct section_file *file);

/* Signals. */
void script_client_signal_connect(const char *signal_name,
                                  const char *callback_name);
void script_client_signal_emit(const char *signal_name, ...);
const char *script_client_signal_list(void);



#endif /* FC__SCRIPT_CLIENT_H */
