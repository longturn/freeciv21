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

struct lua_State;

void api_signal_connect(lua_State *L, const char *signal_name,
                        const char *callback_name);
void api_signal_remove(lua_State *L, const char *signal_name,
                       const char *callback_name);
bool api_signal_defined(lua_State *L, const char *signal_name,
                        const char *callback_name);

const char *api_signal_callback_by_index(lua_State *L,
                                         const char *signal_name,
                                         int sindex);

const char *api_signal_by_index(lua_State *L, int sindex);


