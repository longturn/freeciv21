// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// self
#include "fciconv.h"

// utility
#include "fc_config.h"
#include "fcintl.h"

// dependency
#ifdef FREECIV_ENABLE_NLS

/* Include libintl.h only if nls enabled.
 * It defines some wrapper macros that
 * we don't want defined when nls is disabled. */
#include <libintl.h>
#endif

// Qt
#include <QByteArray> // qstrdup(), qstrncpy()
#include <QLocale>
#include <QTextStream>

char *data_to_internal_string_malloc(const char *text)
{
  return qstrdup(text);
}

char *internal_to_data_string_malloc(const char *text)
{
  return qstrdup(text);
}

char *internal_to_local_string_malloc(const char *text)
{
  return qstrdup(QString(text).toLocal8Bit());
}

char *local_to_internal_string_malloc(const char *text)
{
  return qstrdup(QString::fromLocal8Bit(text).toUtf8());
}

char *local_to_internal_string_buffer(const char *text, char *buf,
                                      size_t bufsz)
{
  return qstrncpy(buf, QString::fromLocal8Bit(text).toUtf8(), bufsz);
}

/**
   Do a fprintf from the internal charset into the local charset.
 */
void fc_fprintf(FILE *stream, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  auto str = QString::vasprintf(format, ap);
  va_end(ap);

  QTextStream(stream) << str;
}

/**
   Return the length, in *characters*, of the string.  This can be used in
   place of qstrlen in some places because it returns the number of
 characters not the number of bytes (with multi-byte characters in UTF-8, the
 two may not be the same).

   Use of this function outside of GUI layout code is probably a hack.  For
   instance the demographics code uses it, but this should instead pass the
   data directly to the GUI library for formatting.
 */
size_t get_internal_string_length(const char *text)
{
  return QString(text).length();
}
