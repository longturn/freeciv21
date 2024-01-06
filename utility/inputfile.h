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

/**********************************************************************
  A low-level object for reading a registry-format file.
  See comments in inputfile.c
***********************************************************************/

#pragma once

// utility
#include "support.h" // bool type and fc__attribute

#include <QString>

class QIODevice;
struct inputfile; // opaque

using datafilename_fn_t = QString (*)(const QString &filename);

struct inputfile *inf_from_file(const QString &filename,
                                datafilename_fn_t datafn);
struct inputfile *inf_from_stream(QIODevice *stream,
                                  datafilename_fn_t datafn);
void inf_close(struct inputfile *inf);
bool inf_at_eof(struct inputfile *inf);

enum inf_token_type {
  INF_TOK_SECTION_NAME,
  INF_TOK_ENTRY_NAME,
  INF_TOK_EOL,
  INF_TOK_TABLE_START,
  INF_TOK_TABLE_END,
  INF_TOK_COMMA,
  INF_TOK_VALUE,
  INF_TOK_LAST
};
#define INF_TOK_FIRST INF_TOK_SECTION_NAME

QString inf_token(struct inputfile *inf, enum inf_token_type type);
int inf_discard_tokens(struct inputfile *inf, enum inf_token_type type);

QString inf_log_str(struct inputfile *inf, const char *message, ...)
    fc__attribute((__format__(__printf__, 2, 3)));
