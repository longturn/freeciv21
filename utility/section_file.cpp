/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "registry.h"
#include "section_file.h"

#define MAX_LEN_ERRORBUF 1024

static char error_buffer[MAX_LEN_ERRORBUF] = "\0";

/* Debug function for every new entry. */
#define DEBUG_ENTRIES(...) /* log_debug(__VA_ARGS__); */

/**********************************************************************/ /**
   Returns the last error which occurred in a string.  It never returns NULL.
 **************************************************************************/
const char *secfile_error() { return error_buffer; }

/**********************************************************************/ /**
   Edit the error_buffer.
 **************************************************************************/
void secfile_log(const struct section_file *secfile,
                 const struct section *psection, const char *file,
                 const char *function, int line, const char *format, ...)
{
  char message[MAX_LEN_ERRORBUF];
  va_list args;

  va_start(args, format);
  fc_vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  fc_snprintf(error_buffer, sizeof(error_buffer),
              "In %s() [%s:%d]: secfile '%s' in section '%s': %s", function,
              file, line, secfile_name(secfile),
              psection != NULL ? section_name(psection) : "NULL", message);
}

/**********************************************************************/ /**
   Returns the section name.
 **************************************************************************/
const char *section_name(const struct section *psection)
{
  return (NULL != psection ? psection->name : NULL);
}

/**********************************************************************/ /**
   Create a new empty section file.
 **************************************************************************/
struct section_file *secfile_new(bool allow_duplicates)
{
  section_file *secfile = new section_file;

  secfile->name = NULL;
  secfile->num_entries = 0;
  secfile->num_includes = 0;
  secfile->num_long_comments = 0;
  secfile->sections = section_list_new_full(section_destroy);
  secfile->allow_duplicates = allow_duplicates;
  secfile->allow_digital_boolean = false; /* Default */

  secfile->hash.sections = new QMultiHash<QString, struct section *>;
  /* Maybe allocated later. */
  secfile->hash.entries = NULL;

  return secfile;
}

/**********************************************************************/ /**
   Free a section file.
 **************************************************************************/
void secfile_destroy(struct section_file *secfile)
{
  SECFILE_RETURN_IF_FAIL(secfile, NULL, secfile != NULL);

  delete secfile->hash.sections;
  /* Mark it NULL to be sure to don't try to make operations when
   * deleting the entries. */
  secfile->hash.sections = NULL;
  NFCN_FREE(secfile->hash.entries);
  section_list_destroy(secfile->sections);
  NFCPP_FREE(secfile->name);

  delete secfile;
}

/**********************************************************************/ /**
   Set if we could consider values 0 and 1 as boolean. By default, this is
   not allowed, but we need to keep compatibility with old Freeciv version
   for savegames, ruleset etc.
 **************************************************************************/
void secfile_allow_digital_boolean(struct section_file *secfile,
                                   bool allow_digital_boolean)
{
  fc_assert_ret(NULL != secfile);
  secfile->allow_digital_boolean = allow_digital_boolean;
}

/**********************************************************************/ /**
   Add entry to section from token.
 **************************************************************************/
bool entry_from_token(struct section *psection, const char *name,
                      const char *tok)
{
  if ('*' == tok[0]) {
    char *buf = new char[strlen(tok) + 1];
    remove_escapes(tok + 1, false, buf, strlen(tok) + 1);
    (void) section_entry_str_new(psection, name, buf, false);
    DEBUG_ENTRIES("entry %s '%s'", name, buf);
    delete[] buf;
    return true;
  }

  if ('$' == tok[0] || '"' == tok[0]) {
    char *buf = new char[strlen(tok) + 1];
    bool escaped = ('"' == tok[0]);

    remove_escapes(tok + 1, escaped, buf, strlen(tok) + 1);
    (void) section_entry_str_new(psection, name, buf, escaped);
    DEBUG_ENTRIES("entry %s '%s'", name, buf);
    delete[] buf;
    return true;
  }

  if (QChar::isDigit(tok[0])
      || (('-' == tok[0] || '+' == tok[0]) && QChar::isDigit(tok[1]))) {
    float fvalue;

    if (str_to_float(tok, &fvalue)) {
      (void) section_entry_float_new(psection, name, fvalue);
      DEBUG_ENTRIES("entry %s %d", name, fvalue);

      return true;
    } else {
      int ivalue;

      if (str_to_int(tok, &ivalue)) {
        (void) section_entry_int_new(psection, name, ivalue);
        DEBUG_ENTRIES("entry %s %d", name, ivalue);

        return true;
      }
    }
  }

  if (0 == fc_strncasecmp(tok, "FALSE", 5)
      || 0 == fc_strncasecmp(tok, "TRUE", 4)) {
    bool value = (0 == fc_strncasecmp(tok, "TRUE", 4));

    (void) section_entry_bool_new(psection, name, value);
    DEBUG_ENTRIES("entry %s %s", name, value ? "TRUE" : "FALSE");
    return true;
  }

  return false;
}
