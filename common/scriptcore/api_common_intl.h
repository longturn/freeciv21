// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// dependencies/sol2
#include "sol/sol.hpp"

const char *api_intl__(sol::this_state s, const char *untranslated);
const char *api_intl_N_(sol::this_state s, const char *untranslated);
const char *api_intl_Q_(sol::this_state s, const char *untranslated);
const char *api_intl_PL_(sol::this_state s, const char *singular,
                         const char *plural, int n);
