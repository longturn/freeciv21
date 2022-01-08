/**************************************************************************
    Copyright (c) 1996-2020 Freeciv21 and Freeciv  contributors. This file
                         is part of Freeciv21. Freeciv21 is free software:
|\_/|,,_____,~~`        you can redistribute it and/or modify it under the
(.".)~~     )`~}}    terms of the GNU General Public License  as published
 \o/\ /---~\\ ~}}     by the Free Software Foundation, either version 3 of
   _//    _// ~}       the License, or (at your option) any later version.
                        You should have received a copy of the GNU General
                          Public License along with Freeciv21. If not, see
                                            https://www.gnu.org/licenses/.
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cmath>

// Sol
#include "sol/sol.hpp"

// utilities
#include "deprecations.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"

// common
#include "map.h"
#include "version.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_common_utilities.h"

/**
   Generate random number.
 */
int api_utilities_random(int min, int max)
{
  double roll;

  roll =
      (static_cast<double>(fc_rand(MAX_UINT32) % MAX_UINT32) / MAX_UINT32);

  return (min + floor(roll * (max - min + 1)));
}

/**
   Return the version of freeciv lua script
 */
const char *api_utilities_fc_version() { return freeciv_name_version(); }

/**
   One log message. This module is used by script_game and script_auth.
 */
void api_utilities_log_base(sol::this_state s, int level,
                            const char *message)
{
  auto fcl = luascript_get_fcl(s);

  LUASCRIPT_CHECK(s, fcl != NULL, "Undefined Freeciv21 lua state!");

  luascript_log(fcl, QtMsgType(level), "%s", message);
}

/**
   Convert text describing direction into direction
 */
const Direction *api_utilities_str2dir(lua_State *L, const char *dir)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, dir, 2, string, NULL);

  return luascript_dir(direction8_by_name(dir, fc_strcasecmp));
}

/**
   Previous (counter-clockwise) valid direction
 */
const Direction *api_utilities_dir_ccw(lua_State *L, Direction dir)
{
  Direction new_dir = dir;

  LUASCRIPT_CHECK_STATE(L, NULL);

  do {
    new_dir = dir_ccw(new_dir);
  } while (!is_valid_dir(new_dir));

  return luascript_dir(new_dir);
}

/**
   Next (clockwise) valid direction
 */
const Direction *api_utilities_dir_cw(lua_State *L, Direction dir)
{
  Direction new_dir = dir;

  LUASCRIPT_CHECK_STATE(L, NULL);

  do {
    new_dir = dir_cw(new_dir);
  } while (!is_valid_dir(new_dir));

  return luascript_dir(new_dir);
}

/**
   Opposite direction - validity not checked, but it's valid iff
   original direction is.
 */
const Direction *api_utilities_opposite_dir(lua_State *L, Direction dir)
{
  LUASCRIPT_CHECK_STATE(L, NULL);

  return luascript_dir(opposite_direction(dir));
}

/**
   Lua script wants to warn about use of deprecated construct.
 */
void api_utilities_deprecation_warning(char *method, char *replacement,
                                       char *deprecated_since)
{
  /* TODO: Keep track which deprecations we have already warned about, and
   * do not keep spamming about them. */
  if (deprecated_since != NULL && replacement != NULL) {
    qCWarning(
        deprecations_category,
        "Deprecated: lua construct \"%s\", deprecated since \"%s\", used. "
        "Use \"%s\" instead",
        method, deprecated_since, replacement);
  } else if (replacement != NULL) {
    qCWarning(deprecations_category,
              "Deprecated: lua construct \"%s\" used. "
              "Use \"%s\" instead",
              method, replacement);
  } else {
    qCWarning(deprecations_category,
              "Deprecated: lua construct \"%s\" used.", method);
  }
}
