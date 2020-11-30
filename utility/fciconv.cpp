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
#include <QTextCodec>
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

static QTextCodec *localCodec;
static QTextCodec *dataCodec;
static QTextCodec *internalCodec;

static const char *transliteration_string;
static const char *local_encoding, *data_encoding, *internal_encoding;

/***********************************************************************/ /**
   Must be called during the initialization phase of server and client to
   initialize the character encodings to be used.

   Pass an internal encoding of NULL to use the local encoding internally.
 ***************************************************************************/
void init_character_encodings(const char *my_internal_encoding,
                              bool my_use_transliteration)
{
  transliteration_string = "";
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
    local_encoding =
        qstrdup(QLocale::system().name().toLocal8Bit().constData());
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
  localCodec = QTextCodec::codecForLocale();
  dataCodec = QTextCodec::codecForName(data_encoding);
  internalCodec = QTextCodec::codecForName(internal_encoding);
#ifdef FREECIV_ENABLE_NLS
  bind_textdomain_codeset("freeciv-core", internal_encoding);
#endif

#ifdef FREECIV_DEBUG
  fprintf(stderr, "Encodings: Data=%s, Local=%s, Internal=%s\n",
          data_encoding, local_encoding, internal_encoding);
#endif /* FREECIV_DEBUG */
}

/***********************************************************************/ /**
   Return the internal encoding.  This depends on the server or GUI being
   used.
 ***************************************************************************/
const char *get_internal_encoding(void) { return internal_encoding; }

char *data_to_internal_string_malloc(const char *text)
{
  QString s;
  s = dataCodec->toUnicode(text);
  s = internalCodec->fromUnicode(s);
  return qstrdup(s.toLocal8Bit().data());
}
char *internal_to_data_string_malloc(const char *text)
{
  QString s;
  s = internalCodec->toUnicode(text);
  s = dataCodec->fromUnicode(s);
  return qstrdup(s.toLocal8Bit().data());
}
char *internal_to_local_string_malloc(const char *text)
{
  QString s;
  s = internalCodec->toUnicode(text);
  s = localCodec->fromUnicode(s);
  return qstrdup(s.toLocal8Bit().data());
}
char *local_to_internal_string_malloc(const char *text)
{
  QString s;
  s = localCodec->toUnicode(text);
  s = internalCodec->fromUnicode(s);
  return qstrdup(s.toLocal8Bit().data());
}
char *local_to_internal_string_buffer(const char *text, char *buf,
                                      size_t bufsz)
{
  QString s;
  s = localCodec->toUnicode(text);
  s = internalCodec->fromUnicode(s);
  return qstrncpy(buf, s.toLocal8Bit().data(), bufsz);
}

/***********************************************************************/ /**
   Do a fprintf from the internal charset into the local charset.
 ***************************************************************************/
void fc_fprintf(FILE *stream, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  auto str = QString::vasprintf(format, ap);
  va_end(ap);

  QTextStream(stream) << str;
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
