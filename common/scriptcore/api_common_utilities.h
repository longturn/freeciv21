// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// dependencies
extern "C" {
#include "lua.h"
}
#include "sol/sol.hpp"

// common
#include "luascript_types.h"

struct lua_State;

int api_utilities_random(int min, int max);

const Direction *api_utilities_str2dir(lua_State *L, const char *dir);
const Direction *api_utilities_dir_ccw(lua_State *L, Direction dir);
const Direction *api_utilities_dir_cw(lua_State *L, Direction dir);
const Direction *api_utilities_opposite_dir(lua_State *L, Direction dir);

const char *api_utilities_fc_version();

void api_utilities_log_base(sol::this_state s, int level,
                            const char *message);

void api_utilities_deprecation_warning(char *method, char *replacement,
                                       char *deprecated_since);
