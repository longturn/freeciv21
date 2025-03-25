// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "capability.h"

// utility
#include "log.h"

// Qt
#include <QByteArray> // qstrlen()
#include <QChar>

// std
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
