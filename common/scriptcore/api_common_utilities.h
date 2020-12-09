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
#include "luascript_types.h"

struct lua_State;

int api_utilities_random(lua_State *L, int min, int max);

const Direction *api_utilities_str2dir(lua_State *L, const char *dir);
const Direction *api_utilities_dir_ccw(lua_State *L, Direction dir);
const Direction *api_utilities_dir_cw(lua_State *L, Direction dir);
const Direction *api_utilities_opposite_dir(lua_State *L, Direction dir);

const char *api_utilities_fc_version(lua_State *L);

void api_utilities_log_base(lua_State *L, int level, const char *message);

void api_utilities_deprecation_warning(lua_State *L, char *method,
                                       char *replacement,
                                       char *deprecated_since);


