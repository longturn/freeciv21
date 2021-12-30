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

// sol2
#include "sol/sol.hpp"

const char *api_intl__(sol::this_state s, const char *untranslated);
const char *api_intl_N_(sol::this_state s, const char *untranslated);
const char *api_intl_Q_(sol::this_state s, const char *untranslated);
const char *api_intl_PL_(sol::this_state s, const char *singular,
                         const char *plural, int n);
