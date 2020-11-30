/***********************************************************************
 Freeciv - Copyright (C) 2003-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <QLocale>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

static bool is_init = FALSE;
static const char *transliteration_string;

#ifdef HAVE_ICONV
static const char *local_encoding, *data_encoding, *internal_encoding;
#else /* HAVE_ICONV */
/* Hack to confuse the compiler into working. */
#define local_encoding get_local_encoding()
#define data_encoding get_local_encoding()
#define internal_encoding get_local_encoding()
#endif /* HAVE_ICONV */

/***********************************************************************/ /**
   Must be called during the initialization phase of server and client to
   initialize the character encodings to be used.

   Pass an internal encoding of NULL to use the local encoding internally.
 ***************************************************************************/
void init_character_encodings(const char *my_internal_encoding,
                              bool my_use_transliteration)
{
  transliteration_string = "";
#ifdef HAVE_ICONV
  if (my_use_transliteration) {
    transliteration_string = "//TRANSLIT";
  }

  /* Set the data encoding - first check $FREECIV_DATA_ENCODING,
   * then fall back to the default. */
  data_encoding = getenv("FREECIV_DATA_ENCODING");
  if (!data_encoding) {
    data_encoding = FC_DEFAULT_DATA_ENCODING;
  }

  /* Set the local encoding - first check $FREECIV_LOCAL_ENCODING,
   * then ask the system. */
  local_encoding = getenv("FREECIV_LOCAL_ENCODING");
  if (!local_encoding) {
    local_encoding = qstrdup(QLocale::system().name().toLocal8Bit().data());
  }

  /* Set the internal encoding - first check $FREECIV_INTERNAL_ENCODING,
   * then check the passed-in default value, then fall back to the local
   * encoding. */
  internal_encoding = getenv("FREECIV_INTERNAL_ENCODING");
  if (!internal_encoding) {
    internal_encoding = my_internal_encoding;

    if (!internal_encoding) {
      internal_encoding = local_encoding;
    }
  }

#ifdef FREECIV_ENABLE_NLS
  bind_textdomain_codeset("freeciv-core", internal_encoding);
#endif

#ifdef FREECIV_DEBUG
  fprintf(stderr, "Encodings: Data=%s, Local=%s, Internal=%s\n",
          data_encoding, local_encoding, internal_encoding);
#endif /* FREECIV_DEBUG */

#else  /* HAVE_ICONV */
  /* log_* may not work at this point. */
  fprintf(stderr,
          _("You are running Freeciv without using iconv. Unless\n"
            "you are using the UTF-8 character set, some characters\n"
            "may not be displayed properly. You can download iconv\n"
            "at http://gnu.org/.\n"));
#endif /* HAVE_ICONV */

  is_init = TRUE;
}

/***********************************************************************/ /**
   Return the local encoding (dependent on the system).
 ***************************************************************************/
const char *get_local_encoding(void)
{
  const char *x = QLocale::system().name().toLocal8Bit().data();
  return x;
}

/***********************************************************************/ /**
   Return the internal encoding.  This depends on the server or GUI being
   used.
 ***************************************************************************/
const char *get_internal_encoding(void)
{
  fc_assert_ret_val(is_init, NULL);
  return internal_encoding;
}

char *data_to_internal_string_malloc(const char *text){return qstrdup(text);}
char *internal_to_data_string_malloc(const char *text){return qstrdup(text);}
char *internal_to_local_string_malloc(const char *text){return qstrdup(text);}
char *local_to_internal_string_malloc(const char *text){return qstrdup(text);}


/***********************************************************************/ /**
   Do a fprintf from the internal charset into the local charset.
 ***************************************************************************/
void fc_fprintf(FILE *stream, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  auto str = QString::vasprintf(format, ap);
  va_end(ap);

  QTextStream(stream) << str << endl;
}

/***********************************************************************/ /**
   Return the length, in *characters*, of the string.  This can be used in
   place of strlen in some places because it returns the number of characters
   not the number of bytes (with multi-byte characters in UTF-8, the two
   may not be the same).

   Use of this function outside of GUI layout code is probably a hack.  For
   instance the demographics code uses it, but this should instead pass the
   data directly to the GUI library for formatting.
 ***************************************************************************/
size_t get_internal_string_length(const char *text)
{
  return QString(text).length();
}
