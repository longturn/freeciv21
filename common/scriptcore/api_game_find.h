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

/* dependencies/lua */
#include "lua.h"

// Sol
#include "sol/sol.hpp"

/* common/scriptcore */
#include "luascript_types.h"

// Object find module.
void setup_lua_find(sol::state_view lua);
