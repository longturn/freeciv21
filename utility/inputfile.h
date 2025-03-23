// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/**********************************************************************
  A low-level object for reading a registry-format file.
  See comments in inputfile.c
***********************************************************************/

#pragma once

// utility
#include "support.h" // bool type and fc__attribute

// Qt
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
