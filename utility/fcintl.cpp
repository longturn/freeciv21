// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

// self
#include "fcintl.h"

// generated
#include <fc_config.h>

// utility
#include "support.h"

// Qt
#include <QByteArray> // qstrlen()
#include <QChar>

// std
#include <cstring>

static bool autocap = false;

/**
   Some strings are ambiguous for translation.  For example, "Game" is
   something you play (like Freeciv!) or animals that can be hunted.
   To distinguish strings for translation, we qualify them with a prefix
   string of the form "?qualifier:".  So, the above two cases might be:
     "Game"           -- when used as meaning something you play
     "?animals:Game"  -- when used as animals to be hunted
   Notice that only the second is qualified; the first is processed in
   the normal gettext() manner (as at most one ambiguous string can be).

   This function tests for, and removes if found, the qualifier prefix part
   of a string.

   This function is called by the Q_() macro and specenum.  If used in the
   Q_() macro it should, if NLS is enabled, have called gettext() to get the
   argument to pass to this function. Specenum use it untranslated.
 */
const char *skip_intl_qualifier_prefix(const char *str)
{
  const char *ptr;

  if (*str != '?') {
    return str;
  } else if ((ptr = strchr(str, ':'))) {
    return (ptr + 1);
  } else {
    return str; // may be something wrong
  }
}

/**
   This function tries to capitalize first letter of the string.
   Currently this handles just single byte UTF-8 characters, since
   those are same as ASCII.
 */
char *capitalized_string(const char *str)
{
  int len = qstrlen(str);
  char *result = new char[len + 1];
  fc_strlcpy(result, str, len + 1);

  if (autocap) {
    if (static_cast<unsigned char>(result[0]) < 128) {
      result[0] = QChar::toUpper(result[0]);
    }
  }

  return result;
}

/**
   Free capitalized string.
 */
void free_capitalized(char *str)
{
  delete[] str;
  str = nullptr;
}

/**
   Translation opts in to automatic capitalization features.
 */
void capitalization_opt_in(bool opt_in) { autocap = opt_in; }

/**
   Automatic capitalization features requested.
 */
bool is_capitalization_enabled() { return autocap; }

/**
   Return directory containing locales.
 */
const char *get_locale_dir() { return LOCALEDIR; }
