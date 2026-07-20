// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "api_common_intl.h"

// dependencies/sol2
#include "sol/sol.hpp"

// utility
#include "fcintl.h"

// common
#include "luascript.h"

/**
   Translation helper function.
 */
const char *api_intl__(sol::this_state s, const char *untranslated)
{
  LUASCRIPT_CHECK_ARG_NIL(s, untranslated, 2, string, "");

  return _(untranslated);
}

/**
   Translation helper function.
 */
const char *api_intl_N_(sol::this_state s, const char *untranslated)
{
  LUASCRIPT_CHECK_ARG_NIL(s, untranslated, 2, string, "");

  return N_(untranslated);
}

/**
   Translation helper function.
 */
const char *api_intl_Q_(sol::this_state s, const char *untranslated)
{
  LUASCRIPT_CHECK_ARG_NIL(s, untranslated, 2, string, "");

  return Q_(untranslated);
}

/**
   Translation helper function.
 */
const char *api_intl_PL_(sol::this_state s, const char *singular,
                         const char *plural, int n)
{
  LUASCRIPT_CHECK_ARG_NIL(s, singular, 2, string, "");
  LUASCRIPT_CHECK_ARG_NIL(s, plural, 3, string, "");

  return PL_(singular, plural, n);
}
