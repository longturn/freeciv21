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
/* utility */
#include "support.h" /* fc__attribute() */

/* server */
#include "fcdb.h"


/* fcdb script functions. */
bool script_fcdb_init(const char *fcdb_luafile);
bool script_fcdb_call(const char *func_name, ...);
void script_fcdb_free(void);

bool script_fcdb_do_string(struct connection *caller, const char *str);


