/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

#include <QByteArray>
#include <string.h> /* qstrlen() */

/* utility */
#include "support.h" /* bool, fc__attribute() */

/* Don't let others modules using the fields directly. */
#define str _private_str_
#define n _private_n_
#define n_alloc _private_n_alloc_

class QStringLiteral;
struct astring {
  char *str;      /* the string */
  size_t n;       /* size most recently requested */
  size_t n_alloc; /* total allocated */
};

/* Can assign this in variable declaration to initialize:
 * Notice a static astring var is exactly this already. */
#define ASTRING_INIT                                                        \
  {                                                                         \
    NULL, 0, 0                                                              \
  }

void astr_init(struct astring *astr) fc__attribute((nonnull(1)));
void astr_free(struct astring *astr) fc__attribute((nonnull(1)));

static inline const char *astr_str(const struct astring *astr)
    fc__attribute((nonnull(1)));
static inline size_t astr_len(const struct astring *astr)
    fc__attribute((nonnull(1)));
static inline size_t astr_size(const struct astring *astr)
    fc__attribute((nonnull(1)));
static inline size_t astr_capacity(const struct astring *astr)
    fc__attribute((nonnull(1)));
static inline bool astr_empty(const struct astring *astr)
    fc__attribute((nonnull(1)));

char *astr_to_str(struct astring *astr) fc__attribute((nonnull(1)));
void astr_reserve(struct astring *astr, size_t size)
    fc__attribute((nonnull(1)));
void astr_clear(struct astring *astr) fc__attribute((nonnull(1)));
void astr_set(struct astring *astr, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)))
        fc__attribute((nonnull(1, 2)));
void astr_vadd(struct astring *astr, const char *format, va_list ap)
    fc__attribute((nonnull(1, 2)));
void astr_add(struct astring *astr, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)))
        fc__attribute((nonnull(1, 2)));
void astr_add_line(struct astring *astr, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)))
        fc__attribute((nonnull(1, 2)));
void astr_break_lines(struct astring *astr, size_t desired_len)
    fc__attribute((nonnull(1)));
void astr_copy(struct astring *dest, const struct astring *src)
    fc__attribute((nonnull(1, 2)));

/****************************************************************************
  Returns the string.
****************************************************************************/
static inline const char *astr_str(const struct astring *astr)
{
  return astr->str;
}

/****************************************************************************
  Returns the length of the string.
****************************************************************************/
static inline size_t astr_len(const struct astring *astr)
{
  return (NULL != astr->str ? qstrlen(astr->str) : 0);
}

/****************************************************************************
  Returns the current size requested for the string.
****************************************************************************/
static inline size_t astr_size(const struct astring *astr)
{
  return astr->n;
}

/****************************************************************************
  Returns the real size requested for the string.
****************************************************************************/
static inline size_t astr_capacity(const struct astring *astr)
{
  return astr->n_alloc;
}

/****************************************************************************
  Returns whether the string is empty or not.
****************************************************************************/
static inline bool astr_empty(const struct astring *astr)
{
  return (0 == astr->n || '\0' == astr->str[0]);
}

#undef str
#undef n
#undef n_alloc

QString strvec_to_or_list(const QVector<QString> &psv);
QString strvec_to_and_list(const QVector<QString> &psv);
QString break_lines(QString src, int after);
QString qendl();