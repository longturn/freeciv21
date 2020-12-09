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
#include "support.h"

struct fc_lua;

void luascript_func_init(struct fc_lua *fcl);
void luascript_func_free(struct fc_lua *fcl);

bool luascript_func_check(struct fc_lua *fcl,
                          struct strvec *missing_func_required,
                          struct strvec *missing_func_optional);
void luascript_func_add_valist(struct fc_lua *fcl, const char *func_name,
                               bool required, int nargs, int nreturns,
                               va_list args);
void luascript_func_add(struct fc_lua *fcl, const char *func_name,
                        bool required, int nargs, int nreturns, ...);
bool luascript_func_call_valist(struct fc_lua *fcl, const char *func_name,
                                va_list args);
bool luascript_func_call(struct fc_lua *fcl, const char *func_name, ...);

bool luascript_func_is_required(struct fc_lua *fcl, const char *func_name);




