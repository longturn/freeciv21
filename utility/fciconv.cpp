/*
             ____             Copyright (c) 1996-2020 Freeciv21 and Freeciv
            /    \__          contributors. This file is part of Freeciv21.
|\         /    @   \   Freeciv21 is free software: you can redistribute it
\ \_______|    \  .:|>         and/or modify it under the terms of the GNU
 \      ##|    | \__/     General Public License  as published by the Free
  |    ####\__/   \   Software Foundation, either version 3 of the License,
  /  /  ##       \|                  or (at your option) any later version.
 /  /__________\  \                 You should have received a copy of the
 L_JJ           \__JJ      GNU General Public License along with Freeciv21.
                                 If not, see https://www.gnu.org/licenses/.
 */

#include "fciconv.h"

#include "fc_config.h"
#include "fcintl.h"

#include <cstdarg>
#include <cstdio>

#include <QLocale>
#include <QTextStream>

/**
   Must be called during the initialization phase of server and client to
   initialize the character encodings to be used.

   Pass an internal encoding of nullptr to use the local encoding internally.
 */
void init_character_encodings()
{
#ifdef FREECIV_ENABLE_NLS
  bind_textdomain_codeset("freeciv21-core", "UTF-8");
#endif
}

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
