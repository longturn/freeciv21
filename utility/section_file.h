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

/* This header contains internals of section_file that its users should
 * not care about. This header should be included by source files
 * implementing registry itself. */

// utility
#include "registry_ini.h"
#include "support.h"

#include <QString>

#include <stddef.h>

template <class Key, class T> class QMultiHash;

// Section structure.
struct section {
  struct section_file *secfile; // Parent structure.
  enum entry_special_type special;
  char *name;                 // Name of the section.
  struct entry_list *entries; // The list of the children.
};

// The section file struct itself.
struct section_file {
  char *name; // Can be nullptr.
  size_t num_entries;
  /* num_includes should be size_t, but as there's no truly portable
   * printf format for size_t and we need to construct string containing
   * num_includes with fc_snprintf(), we set for unsigned int. */
  unsigned int num_includes;
  unsigned int num_long_comments;
  struct section_list *sections;
  bool allow_duplicates;
  bool allow_digital_boolean;
  struct {
    QMultiHash<QString, struct section *> *sections;
    QMultiHash<QString, struct entry *> *entries;
  } hash;
};

void secfile_log(const struct section_file *secfile,
                 const struct section *psection, const char *file,
                 const char *function, int line, const char *format, ...)
    fc__attribute((__format__(__printf__, 6, 7)));

#define SECFILE_LOG(secfile, psection, format, ...)                         \
  secfile_log(secfile, psection, __FILE__, __FUNCTION__, __LINE__, format,  \
              ##__VA_ARGS__)
#define SECFILE_RETURN_IF_FAIL(secfile, psection, condition)                \
  if (!(condition)) {                                                       \
    SECFILE_LOG(secfile, psection, "Assertion '%s' failed.", #condition);   \
    return;                                                                 \
  }
#define SECFILE_RETURN_VAL_IF_FAIL(secfile, psection, condition, value)     \
  if (!(condition)) {                                                       \
    SECFILE_LOG(secfile, psection, "Assertion '%s' failed.", #condition);   \
    return value;                                                           \
  }

bool entry_from_token(struct section *psection, const QString &name,
                      const QString &tok);
