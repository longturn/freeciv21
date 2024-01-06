/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include "capability.h"

#include "log.h"

#include <QChar>

#include <cstring>

#define GET_TOKEN(start, end)                                               \
  {                                                                         \
    /* skip leading whitespace */                                           \
    while (QChar::isSpace(*start)) {                                        \
      start++;                                                              \
    }                                                                       \
    /* skip to end of token */                                              \
    for (end = start; *end != '\0' && !QChar::isSpace(*end) && *end != ','; \
         end++) {                                                           \
      /* nothing */                                                         \
    }                                                                       \
  }

/**
   This routine returns true if the capability in cap appears
   in the capability list in capstr.  The capabilities in capstr
   are allowed to start with a "+", but the capability in cap must not.
 */
static bool fc_has_capability(const char *cap, const char *capstr,
                              const size_t cap_len)
{
  const char *next;

  fc_assert_ret_val(capstr != nullptr, false);

  for (;;) {
    GET_TOKEN(capstr, next);

    if (*capstr == '+') {
      capstr++;
    }

    fc_assert(next >= capstr);

    if ((static_cast<size_t>(next - capstr) == cap_len)
        && strncmp(cap, capstr, cap_len) == 0) {
      return true;
    }
    if (*next == '\0') {
      return false;
    }

    capstr = next + 1;
  }
}

/**
   Wrapper for fc_has_capability() for nullptr terminated strings.
 */
bool has_capability(const char *cap, const char *capstr)
{
  return fc_has_capability(cap, capstr, qstrlen(cap));
}

/**
   This routine returns true if all the mandatory capabilities in
   us appear in them.
 */
bool has_capabilities(const char *us, const char *them)
{
  const char *next;

  for (;;) {
    GET_TOKEN(us, next);

    if (*us == '+' && !fc_has_capability(us + 1, them, next - (us + 1))) {
      return false;
    }
    if (*next == '\0') {
      return true;
    }

    us = next + 1;
  }
}
