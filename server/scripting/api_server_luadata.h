// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// dependencies/lua
extern "C" {
#include "lua.h"
}

struct lua_State;

const char *api_luadata_get_str(lua_State *L, const char *field);
