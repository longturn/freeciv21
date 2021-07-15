/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// utility
#include "fcintl.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_common_intl.h"

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
