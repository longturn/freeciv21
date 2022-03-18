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

// Sol
#include "sol/sol.hpp"

/* common/scriptcore */
#include "luascript_types.h"

struct lua_State;

// type of clima change
enum climate_change_type {
  CLIMATE_CHANGE_GLOBAL_WARMING,
  CLIMATE_CHANGE_NUCLEAR_WINTER
};

void setup_server_edit(sol::state_view lua);
