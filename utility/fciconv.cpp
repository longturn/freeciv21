// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/**
 * \file
 * Technical details:
 *
 * - There are two encodings used by freeciv21: the data encoding and the
 *  system encoding. The data encoding corresponds to all data files and the
 *  network protocal. It is always UTF-8. The system may use any encoding it
 *  likes, and a conversion from UTF-8 may be needed, for instance when
 *  printing to the terminal. Thankfully, Qt abstracts much of this away for
 *  us. Use QString whenever possible and avoid this header.
 *
 * - The local_encoding is the one supported on the command line, which is
 *  generally the value listed in the $LANG environment variable.  This is
 *  not under freeciv control; all output to the command line must be
 *  converted or it will not display correctly.
 *
 * Practical details:
 *
 * - Translation files are not controlled by freeciv iconv. The .po files
 *  can be in any character set, as set at the top of the file.
 *
 * - All translatable texts should be American English ASCII. In the past,
 *  gettext documentation has always said to stick to ASCII for the gettext
 *  input (pre-translated texts) and rely on translations to supply the
 *  needed non-ASCII characters.
 *
 * - All other texts, including rulesets, nations, and code files must be in
 *  UTF-8 (ASCII is a subset of UTF-8, and is fine for use here).
 *
 * - The server uses UTF-8 for everything; UTF-8 is the server's "internal
 *  encoding".
 *
 * - Everything sent over the network is always in UTF-8.
 *
 * - Everything in the client is also in UTF-8.
 *
 * - Everything printed to the command line must be converted into the
 *  "local encoding" which may be anything as defined by the system.  Qt's
 *  logging functions (qInfo and similar) do this for us.
 */

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
