// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
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
