/**************************************************************************
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
**************************************************************************/
#pragma once

#include "support.h"

#include <stdio.h>

/*
  Technical details:

  - There are three encodings used by freeciv: the data encoding, the
    internal encoding, and the local encoding.  Each is a character set
    (like utf-8 or latin1).  Each string in the code must be in one of these
    three encodings; to cut down on bugs always document whenever you have
    a string in anything other than the internal encoding and never make
    global variables hold anything other than the internal encoding; the
    local and data encodings should only be used locally within the code
    and always documented as such.

  - The data_encoding is used in all data files and network transactions.
    This is always UTF-8.

  - The internal_encoding is used internally within freeciv.  This is always
    UTF-8.

  - The local_encoding is the one supported on the command line, which is
    generally the value listed in the $LANG environment variable.  This is
    not under freeciv control; all output to the command line must be
    converted or it will not display correctly.

  Practical details:

  - Translation files are not controlled by freeciv iconv.  The .po files
    can be in any character set, as set at the top of the file.

  - All translatable texts should be American English ASCII. In the past,
    gettext documentation has always said to stick to ASCII for the gettext
    input (pre-translated texts) and rely on translations to supply the
    needed non-ASCII characters.

  - All other texts, including rulesets, nations, and code files must be in
    UTF-8 (ASCII is a subset of UTF-8, and is fine for use here).

  - The server uses UTF-8 for everything; UTF-8 is the server's "internal
    encoding".

  - Everything sent over the network is always in UTF-8.

  - Everything in the client is also in UTF-8.

  - Everything printed to the command line must be converted into the
    "local encoding" which may be anything as defined by the system.  Using
    fc_fprintf is generally the easiest way to print to the command line
    in which case all strings passed to it should be in the internal
    encoding.
*/

#define FC_DEFAULT_DATA_ENCODING "UTF-8"

void init_character_encodings(const char *internal_encoding,
                              bool use_transliteration);

const char *get_internal_encoding();

char *data_to_internal_string_malloc(const char *text);
char *internal_to_data_string_malloc(const char *text);
char *internal_to_local_string_malloc(const char *text);
char *local_to_internal_string_malloc(const char *text);

char *local_to_internal_string_buffer(const char *text, char *buf,
                                      size_t bufsz);

#define fc_printf(...) fc_fprintf(stdout, __VA_ARGS__)
void fc_fprintf(FILE *stream, const char *format, ...)
    fc__attribute((__format__(__printf__, 2, 3)));

size_t get_internal_string_length(const char *text);
