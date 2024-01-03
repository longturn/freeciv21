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

/**
  \file
  the idea with this file is to create something similar to the ms-windows
  .ini files functions.
  however the interface is nice. ie:
  secfile_lookup_str(file, "player%d.unit%d.name", plrno, unitno);

  Description of the file format
  ==============================

  (This is based on a format by the original authors, with
  various incremental extensions. --dwp)

  - Whitespace lines are ignored, as are lines where the first
  non-whitespace character is ';' (comment lines).
  Optionally '#' can also be used for comments.

  - A line of the form:
       *include "filename"
  includes the named file at that point.  (The '*' must be the
  first character on the line.) The file is found by looking in
  FREECIV_DATA_PATH.  Non-infinite recursive includes are allowed.

  - A line with "[name]" labels the start of a section with
  that name; one of these must be the first non-comment line in
  the file.  Any spaces within the brackets are included in the
  name, but this feature (?) should probably not be used...

  - Within a section, lines have one of the following forms:
      subname = "stringvalue"
      subname = -digits
      subname = digits
      subname = TRUE
      sunname = FALSE
  for a value with given name and string, negative integer, and
  positive integer values, respectively.  These entries are
  referenced in the following functions as "sectionname.subname".
  The section name should not contain any dots ('.'); the subname
  can, but they have no particular significance.  There can be
  optional whitespace before and/or after the equals sign.
  You can put a newline after (but not before) the equals sign.

  Backslash is an escape character in strings (double-quoted strings
  only, not names); recognised escapes are \n, \\, and \".
  (Any other \<char> is just treated as <char>.)

  - Gettext markings:  You can surround strings like so:
      foo = _("stringvalue")
  The registry just ignores these extra markings, but this is
  useful for marking strings for translations via gettext tools.

  - Multiline strings:  Strings can have embeded newlines, eg:
    foo = _("
    This is a string
    over multiple lines
    ")
  This is equivalent to:
    foo = _("\nThis is a string\nover multiple lines\n")
  Note that if you missplace the trailing doublequote you can
  easily end up with strange errors reading the file...

  - Strings read from a file: A file can be read as a string value:
    foo = *filename.txt*

  - Vector format: An entry can have multiple values separated
  by commas, eg:
      foo = 10, 11, "x"
  These are accessed by names "foo", "foo,1" and "foo,2"
  (with section prefix as above).  So the above is equivalent to:
      foo   = 10
      foo,1 = 11
      foo,2 = "x"
  As in the example, in principle you can mix integers and strings,
  but the calling program will probably require elements to be the
  same type.   Note that the first element of a vector is not "foo,0",
  in order that the name of the first element is the same whether or
  not there are subsequent elements.  However as a convenience, if
  you try to lookup "foo,0" then you get back "foo".  (So you should
  never have "foo,0" as a real name in the datafile.)

  - Tabular format:  The lines:
      foo = { "bar",  "baz",   "bax"
              "wow",   10,     -5
              "cool",  "str"
              "hmm",    314,   99, 33, 11
      }
  are equivalent to the following:
      foo0.bar = "wow"
      foo0.baz = 10
      foo0.bax = -5
      foo1.bar = "cool"
      foo1.baz = "str"
      foo2.bar = "hmm"
      foo2.baz = 314
      foo2.bax = 99
      foo2.bax,1 = 33
      foo2.bax,2 = 11
  The first line specifies the base name and the column names, and the
  subsequent lines have data.  Again it is possible to mix string and
  integer values in a column, and have either more or less values
  in a row than there are column headings, but the code which uses
  this information (via the registry) may set more stringent conditions.
  If a row has more entries than column headings, the last column is
  treated as a vector (as above).  You can optionally put a newline
  after '=' and/or after '{'.

  The equivalence above between the new and old formats is fairly
  direct: internally, data is converted to the old format.
  In principle it could be a good idea to represent the data
  as a table (2-d array) internally, but the current method
  seems sufficient and relatively simple...

  There is a limited ability to save data in tabular:
  So long as the section_file is constructed in an expected way,
  tabular data (with no missing or extra values) can be saved
  in tabular form.  (See section_file_save().)

  - Multiline vectors: if the last non-comment non-whitespace
  character in a line is a comma, the line is considered to
  continue on to the next line.  Eg:
      foo = 10,
            11,
            "x"
  This is equivalent to the original "vector format" example above.
  Such multi-lines can occur for column headings, vectors, or
  table rows, again with some potential for strange errors...

  Hashing registry lookups
  ========================

  (by dwp)
  - Have a hash table direct to entries, bypassing sections division.
  - For convenience, store the key (the full section+entry name)
    in the hash table (some memory overhead).
  - The number of entries is fixed when the hash table is built.
  - Now uses hash.c
 */
// KArchive
#include <KFilterDev>

// utility
#include "bugs.h"
#include "deprecations.h"
#include "fcintl.h"
#include "inputfile.h"
#include "log.h"
#include "registry.h"
#include "section_file.h"
#include "shared.h"
#include "support.h"

#include "registry_ini.h"

#define MAX_LEN_SECPATH 1024

// Set to FALSE for old-style savefiles.
#define SAVE_TABLES true

static inline bool entry_used(const struct entry *pentry);
static inline void entry_use(struct entry *pentry);

static bool entry_to_file(const struct entry *pentry, QIODevice *fs);
static void entry_from_inf_token(struct section *psection,
                                 const QString &name, const QString &tok,
                                 struct inputfile *file);

/* An 'entry' is a string, integer, boolean or string vector;
 * See enum entry_type in registry.h.
 */
struct entry {
  struct section *psection; // Parent section.
  char *name;               // Name, not including section prefix.
  enum entry_type type;     // The type of the entry.
  int used;                 // Number of times entry looked up.
  char *comment;            // Comment, may be nullptr.

  union {
    // ENTRY_BOOL
    struct {
      bool value;
    } boolean;
    // ENTRY_INT
    struct {
      int value;
    } integer;
    // ENTRY_FLOAT
    struct {
      float value;
    } floating;
    // ENTRY_STR
    struct {
      char *value;     // Malloced string.
      bool escaped;    // " or $. Usually TRUE
      bool raw;        // Do not add anything.
      bool gt_marking; // Save with gettext marking.
    } string;
  };
};

static struct entry *
section_entry_filereference_new(struct section *psection, const char *name,
                                const char *value);

/**
   Simplification of fileinfoname().
 */
static QString datafilename(const QString &filename)
{
  return fileinfoname(get_data_dirs(), qUtf8Printable(filename));
}

/**
   Ensure name is correct to use it as section or entry name.
 */
static bool is_secfile_entry_name_valid(const QString &name)
{
  static const auto allowed = QStringLiteral("_.,-[]");

  for (const auto &c : name) {
    if (!c.isLetterOrNumber() && !allowed.contains(c)) {
      return false;
    }
  }
  return true;
}

/**
   Insert an entry into the hash table.  Returns TRUE on success.
 */
static bool secfile_hash_insert(struct section_file *secfile,
                                struct entry *pentry)
{
  char buf[256];
  struct entry *hentry;

  if (nullptr == secfile->hash.entries) {
    /* Consider as success if this secfile doesn't have built the entries
     * hash table. */
    return true;
  }

  entry_path(pentry, buf, sizeof(buf));

  hentry = secfile->hash.entries->value(buf, nullptr);
  if (hentry) {
    entry_use(hentry);
    if (!secfile->allow_duplicates) {
      SECFILE_LOG(secfile, entry_section(hentry),
                  "Tried to insert same value twice: %s", buf);
      return false;
    }
  }
  secfile->hash.entries->insert(buf, pentry);
  return true;
}

/**
   Delete an entry from the hash table.  Returns TRUE on success.
 */
static bool secfile_hash_delete(struct section_file *secfile,
                                struct entry *pentry)
{
  char buf[256];

  if (nullptr == secfile->hash.entries) {
    /* Consider as success if this secfile doesn't have built the entries
     * hash table. */
    return true;
  }

  entry_path(pentry, buf, sizeof(buf));
  secfile->hash.entries->remove(buf);
  return true;
}

/**
   Base function to load a section file.  Note it closes the inputfile.
 */
static struct section_file *secfile_from_input_file(struct inputfile *inf,
                                                    const QString &filename,
                                                    const QString &section,
                                                    bool allow_duplicates)
{
  struct section_file *secfile;
  struct section *psection = nullptr;
  struct section *single_section = nullptr;
  bool table_state = false; // TRUE when within tabular format.
  int table_lineno = 0;     // Row number in tabular, 0 top data row.
  QString tok;
  int i;
  QString base_name; // for table or single entry
  QString field_name;
  QVector<QString> columns; // qstrings for column headings
  bool found_my_section = false;
  bool error = false;

  if (!inf) {
    return nullptr;
  }

  // Assign the real value later, to speed up the creation of new entries.
  secfile = secfile_new(true);
  if (!filename.isEmpty()) {
    secfile->name = fc_strdup(qUtf8Printable(filename));
  } else {
    secfile->name = nullptr;
  }

  if (!filename.isEmpty()) {
    qDebug("Reading registry from \"%s\"", qUtf8Printable(filename));
  } else {
    qDebug("Reading registry");
  }

  while (!inf_at_eof(inf)) {
    if (!inf_token(inf, INF_TOK_EOL).isEmpty()) {
      continue;
    }
    if (inf_at_eof(inf)) {
      // may only realise at eof after trying to read eol above
      break;
    }
    tok = inf_token(inf, INF_TOK_SECTION_NAME);
    if (!tok.isEmpty()) {
      if (found_my_section) {
        /* This shortcut will stop any further loading after the requested
         * section has been loaded (i.e., at the start of a new section).
         * This is needed to make the behavior useful, since the whole
         * purpose is to short-cut further loading of the file.  However
         * normally a section may be split up, and that will no longer
         * work here because it will be short-cut. */
        SECFILE_LOG(secfile, psection, "%s",
                    qUtf8Printable(inf_log_str(
                        inf, "Found requested section; finishing")));
        goto END;
      }
      if (table_state) {
        SECFILE_LOG(
            secfile, psection, "%s",
            qUtf8Printable(inf_log_str(inf, "New section during table")));
        error = true;
        goto END;
      }
      /* Check if we already have a section with this name.
         (Could ignore this and have a duplicate sections internally,
         but then secfile_get_secnames_prefix would return duplicates.)
         Duplicate section in input are likely to be useful for includes.
      */
      psection = secfile_section_by_name(secfile, tok);
      if (!psection) {
        if (section.isEmpty() || tok == section) {
          psection = secfile_section_new(secfile, tok);
          if (!section.isEmpty()) {
            single_section = psection;
            found_my_section = true;
          }
        }
      }
      if (inf_token(inf, INF_TOK_EOL).isEmpty()) {
        SECFILE_LOG(
            secfile, psection, "%s",
            qUtf8Printable(inf_log_str(inf, "Expected end of line")));
        error = true;
        goto END;
      }
      continue;
    }
    if (!inf_token(inf, INF_TOK_TABLE_END).isEmpty()) {
      if (!table_state) {
        SECFILE_LOG(secfile, psection, "%s",
                    qUtf8Printable(inf_log_str(inf, "Misplaced \"}\"")));
        error = true;
        goto END;
      }
      if (inf_token(inf, INF_TOK_EOL).isEmpty()) {
        SECFILE_LOG(
            secfile, psection, "%s",
            qUtf8Printable(inf_log_str(inf, "Expected end of line")));
        error = true;
        goto END;
      }
      columns.clear();
      table_state = false;
      continue;
    }
    if (table_state) {
      i = -1;
      do {
        int num_columns = columns.size();

        i++;
        inf_discard_tokens(inf, INF_TOK_EOL); // allow newlines
        if ((tok = inf_token(inf, INF_TOK_VALUE)).isEmpty()) {
          SECFILE_LOG(secfile, psection, "%s",
                      qUtf8Printable(inf_log_str(inf, "Expected value")));
          error = true;
          goto END;
        }

        if (i < num_columns) {
          field_name = QStringLiteral("%1%2.%3").arg(
              base_name, QString::number(table_lineno), columns.at(i));
        } else {
          field_name = QStringLiteral("%1%2.%3,%4")
                           .arg(base_name, QString::number(table_lineno),
                                columns.at(num_columns - 1),
                                QString::number((i - num_columns + 1)));
        }
        entry_from_inf_token(psection, field_name, tok, inf);
      } while (!inf_token(inf, INF_TOK_COMMA).isEmpty());

      if (inf_token(inf, INF_TOK_EOL).isEmpty()) {
        SECFILE_LOG(
            secfile, psection, "%s",
            qUtf8Printable(inf_log_str(inf, "Expected end of line")));
        error = true;
        goto END;
      }
      table_lineno++;
      continue;
    }

    if ((tok = inf_token(inf, INF_TOK_ENTRY_NAME)).isEmpty()) {
      SECFILE_LOG(secfile, psection, "%s",
                  qUtf8Printable(inf_log_str(inf, "Expected entry name")));
      error = true;
      goto END;
    }

    // need to store tok before next calls:
    base_name = tok;

    inf_discard_tokens(inf, INF_TOK_EOL); // allow newlines

    if (!inf_token(inf, INF_TOK_TABLE_START).isEmpty()) {
      i = -1;
      do {
        i++;
        inf_discard_tokens(inf, INF_TOK_EOL); // allow newlines
        if ((tok = inf_token(inf, INF_TOK_VALUE)).isEmpty()) {
          SECFILE_LOG(secfile, psection, "%s",
                      qUtf8Printable(inf_log_str(inf, "Expected value")));
          error = true;
          goto END;
        }
        if (tok[0] != '\"') {
          SECFILE_LOG(secfile, psection, "%s",
                      qUtf8Printable(inf_log_str(
                          inf, "Table column header non-string")));
          error = true;
          goto END;
        }
        columns.append(tok.mid(1));
      } while (!inf_token(inf, INF_TOK_COMMA).isEmpty());

      if (inf_token(inf, INF_TOK_EOL).isEmpty()) {
        SECFILE_LOG(
            secfile, psection, "%s",
            qUtf8Printable(inf_log_str(inf, "Expected end of line")));
        error = true;
        goto END;
      }
      table_state = true;
      table_lineno = 0;
      continue;
    }
    // ordinary value:
    i = -1;
    do {
      i++;
      inf_discard_tokens(inf, INF_TOK_EOL); // allow newlines
      if ((tok = inf_token(inf, INF_TOK_VALUE)).isEmpty()) {
        SECFILE_LOG(secfile, psection, "%s",
                    qUtf8Printable(inf_log_str(inf, "Expected value")));
        error = true;
        goto END;
      }
      if (i == 0) {
        entry_from_inf_token(psection, qUtf8Printable(base_name), tok, inf);
      } else {
        field_name =
            QStringLiteral("%1,%2").arg(base_name, QString::number(i));
        entry_from_inf_token(psection, qUtf8Printable(field_name), tok, inf);
      }
    } while (!inf_token(inf, INF_TOK_COMMA).isEmpty());
    if (inf_token(inf, INF_TOK_EOL).isEmpty()) {
      SECFILE_LOG(secfile, psection, "%s",
                  qUtf8Printable(inf_log_str(inf, "Expected end of line")));
      error = true;
      goto END;
    }
  }

  if (table_state) {
    SECFILE_LOG(secfile, psection, "Finished registry before end of table");
    error = true;
  }

END:
  inf_close(inf);

  if (section != nullptr) {
    if (!found_my_section) {
      secfile_destroy(secfile);
      return nullptr;
    }

    // Build the entry hash table with single section information
    secfile->allow_duplicates = allow_duplicates;
    entry_list_iterate(section_entries(single_section), pentry)
    {
      if (!secfile_hash_insert(secfile, pentry)) {
        secfile_destroy(secfile);
        return nullptr;
      }
    }
    entry_list_iterate_end;

    return secfile;
  }

  if (!error) {
    // Build the entry hash table.
    secfile->allow_duplicates = allow_duplicates;
    secfile->hash.entries = new QMultiHash<QString, struct entry *>;
    section_list_iterate(secfile->sections, hashing_section)
    {
      entry_list_iterate(section_entries(hashing_section), pentry)
      {
        if (!secfile_hash_insert(secfile, pentry)) {
          error = true;
          break;
        }
      }
      entry_list_iterate_end;
      if (error) {
        break;
      }
    }
    section_list_iterate_end;
  }
  if (error) {
    secfile_destroy(secfile);
    return nullptr;
  } else {
    return secfile;
  }
}

/**
   Create a section file from a file, read only one particular section.
   Returns nullptr on error.
 */
struct section_file *secfile_load_section(const QString &filename,
                                          const QString &section,
                                          bool allow_duplicates)
{
  const auto real_filename = interpret_tilde(filename);
  return secfile_from_input_file(inf_from_file(real_filename, datafilename),
                                 filename, section, allow_duplicates);
}

/**
   Create a section file from a stream.  Returns nullptr on error.
 */
struct section_file *secfile_from_stream(QIODevice *stream,
                                         bool allow_duplicates)
{
  return secfile_from_input_file(inf_from_stream(stream, datafilename),
                                 nullptr, nullptr, allow_duplicates);
}

/**
   Returns TRUE iff the character is legal in a table entry name.
 */
static bool is_legal_table_entry_name(char c, bool num)
{
  return (num ? QChar::isLetterOrNumber(c) : QChar::isLetter(c)) || c == '_';
}

/**
   Save the previously filled in section_file to disk.

   There is now limited ability to save in the new tabular format
   (to give smaller savefiles).
   The start of a table is detected by an entry with name of the form:
     (alphabetical_component)(zero)(period)(alphanumeric_component)
   Eg: u0.id, or c0.id, in the freeciv savefile.
   The alphabetical component is taken as the "name" of the table,
   and the component after the period as the first column name.
   This should be followed by the other column values for u0,
   and then subsequent u1, u2, etc, in strict order with no omissions,
   and with all of the columns for all uN in the same order as for u0.
 */
bool secfile_save(const struct section_file *secfile, QString filename)
{
  char pentry_name[128];
  const char *col_entry_name;
  const struct entry_list_link *ent_iter, *save_iter, *col_iter;
  struct entry *pentry, *col_pentry;
  int i;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);

  if (filename.isEmpty()) {
    filename = secfile->name;
  }

  auto real_filename = interpret_tilde(filename);
  auto fs = std::make_unique<KFilterDev>(real_filename);
  fs->open(QIODevice::WriteOnly);

  if (!fs->isOpen()) {
    SECFILE_LOG(secfile, nullptr, _("Could not open %s for writing"),
                qUtf8Printable((real_filename)));
    return false;
  }

  section_list_iterate(secfile->sections, psection)
  {
    if (psection->special == EST_INCLUDE) {
      for (ent_iter = entry_list_head(section_entries(psection));
           ent_iter && (pentry = entry_list_link_data(ent_iter));
           ent_iter = entry_list_link_next(ent_iter)) {
        fc_assert(!strcmp(entry_name(pentry), "file"));

        fc_assert_ret_val(fs->write("*include ") > 0, false);
        fc_assert_ret_val(entry_to_file(pentry, fs.get()), false);
        fc_assert_ret_val(fs->write("\n") > 0, false);
      }
    } else if (psection->special == EST_COMMENT) {
      for (ent_iter = entry_list_head(section_entries(psection));
           ent_iter && (pentry = entry_list_link_data(ent_iter));
           ent_iter = entry_list_link_next(ent_iter)) {
        fc_assert(!strcmp(entry_name(pentry), "comment"));

        fc_assert_ret_val(entry_to_file(pentry, fs.get()), false);
        fc_assert_ret_val(fs->write("\n") > 0, false);
      }
    } else {
      fc_assert_ret_val(fs->write("\n[") > 0, false);
      fc_assert_ret_val(fs->write(section_name(psection)) > 0, false);
      fc_assert_ret_val(fs->write("]\n") > 0, false);

      /* Following doesn't use entry_list_iterate() because we want to do
       * tricky things with the iterators...
       */
      for (ent_iter = entry_list_head(section_entries(psection));
           ent_iter && (pentry = entry_list_link_data(ent_iter));
           ent_iter = entry_list_link_next(ent_iter)) {
        const char *comment;

        /* Tables: break out of this loop if this is a non-table
         * entry (pentry and ent_iter unchanged) or after table (pentry
         * and ent_iter suitably updated, pentry possibly nullptr).
         * After each table, loop again in case the next entry
         * is another table.
         */
        for (;;) {
          char *c, *first, base[64];
          int offset, irow, icol, ncol;

          /* Example: for first table name of "xyz0.blah":
           *  first points to the original string pentry->name
           *  base contains "xyz";
           *  offset = 5 (so first+offset gives "blah")
           *  note qstrlen(base) = offset - 2
           */

          if (!SAVE_TABLES) {
            break;
          }

          sz_strlcpy(pentry_name, entry_name(pentry));
          c = first = pentry_name;
          if (*c == '\0' || !is_legal_table_entry_name(*c, false)) {
            break;
          }
          for (; *c != '\0' && is_legal_table_entry_name(*c, false); c++) {
            // nothing
          }
          if (0 != strncmp(c, "0.", 2)) {
            break;
          }
          c += 2;
          if (*c == '\0' || !is_legal_table_entry_name(*c, true)) {
            break;
          }

          offset = c - first;
          first[offset - 2] = '\0';
          sz_strlcpy(base, first);
          first[offset - 2] = '0';
          fc_assert_ret_val(fs->write(base) > 0, false);
          fc_assert_ret_val(fs->write("={") > 0, false);

          /* Save an iterator at this first entry, which we can later use
           * to repeatedly iterate over column names:
           */
          save_iter = ent_iter;

          // write the column names, and calculate ncol:
          ncol = 0;
          col_iter = save_iter;
          for (; (col_pentry = entry_list_link_data(col_iter));
               col_iter = entry_list_link_next(col_iter)) {
            col_entry_name = entry_name(col_pentry);
            if (strncmp(col_entry_name, first, offset) != 0) {
              break;
            }
            fc_assert_ret_val(fs->write(ncol == 0 ? "\"" : ",\"") > 0,
                              false);
            fc_assert_ret_val(fs->write(col_entry_name + offset) > 0, false);
            fc_assert_ret_val(fs->write("\"") > 0, false);
            ncol++;
          }
          fc_assert_ret_val(fs->write("\n") > 0, false);

          /* Iterate over rows and columns, incrementing ent_iter as we go,
           * and writing values to the table.  Have a separate iterator
           * to the column names to check they all match.
           */
          irow = icol = 0;
          col_iter = save_iter;
          for (;;) {
            char expect[128]; // pentry->name we're expecting

            pentry = entry_list_link_data(ent_iter);
            col_pentry = entry_list_link_data(col_iter);

            fc_snprintf(expect, sizeof(expect), "%s%d.%s", base, irow,
                        entry_name(col_pentry) + offset);

            // break out of tabular if doesn't match:
            if ((!pentry) || (strcmp(entry_name(pentry), expect) != 0)) {
              if (icol != 0) {
                /* If the second or later row of a table is missing some
                 * entries that the first row had, we drop out of the tabular
                 * format.  This is inefficient so we print a warning
                 * message; the calling code probably needs to be fixed so
                 * that it can use the more efficient tabular format.
                 *
                 * FIXME: If the first row is missing some entries that the
                 * second or later row has, then we'll drop out of tabular
                 * format without an error message. */
                qCCritical(
                    bugs_category,
                    "In file %s, there is no entry in the registry for\n"
                    "%s.%s (or the entries are out of order). This means\n"
                    "a less efficient non-tabular format will be used.\n"
                    "To avoid this make sure all rows of a table are\n"
                    "filled out with an entry for every column.",
                    qUtf8Printable(real_filename), section_name(psection),
                    expect);
                fc_assert_ret_val(fs->write("\n") > 0, false);
              }
              fc_assert_ret_val(fs->write("}\n") > 0, false);
              break;
            }

            if (icol > 0) {
              fc_assert_ret_val(fs->write(",") > 0, false);
            }
            fc_assert_ret_val(entry_to_file(pentry, fs.get()), false);

            ent_iter = entry_list_link_next(ent_iter);
            col_iter = entry_list_link_next(col_iter);

            icol++;
            if (icol == ncol) {
              fc_assert_ret_val(fs->write("\n") > 0, false);
              irow++;
              icol = 0;
              col_iter = save_iter;
            }
          }
          if (!pentry) {
            break;
          }
        }
        if (!pentry) {
          break;
        }

        // Classic entry.
        col_entry_name = entry_name(pentry);
        fc_assert_ret_val(fs->write(col_entry_name), false);
        fc_assert_ret_val(fs->write("="), false);
        fc_assert_ret_val(entry_to_file(pentry, fs.get()), false);

        // Check for vector.
        for (i = 1;; i++) {
          col_iter = entry_list_link_next(ent_iter);
          col_pentry = entry_list_link_data(col_iter);
          if (nullptr == col_pentry) {
            break;
          }
          fc_snprintf(pentry_name, sizeof(pentry_name), "%s,%d",
                      col_entry_name, i);
          if (0 != strcmp(pentry_name, entry_name(col_pentry))) {
            break;
          }
          fc_assert_ret_val(fs->write(",") > 0, false);
          fc_assert_ret_val(entry_to_file(col_pentry, fs.get()), false);
          ent_iter = col_iter;
        }

        comment = entry_comment(pentry);
        if (comment) {
          fc_assert_ret_val(fs->write("  # ") > 0, false);
          fc_assert_ret_val(fs->write(comment) > 0, false);
          fc_assert_ret_val(fs->write("\n") > 0, false);
        } else {
          fc_assert_ret_val(fs->write("\n") > 0, false);
        }
      }
    }
  }
  section_list_iterate_end;

  if (fs->error() != 0) {
    SECFILE_LOG(secfile, nullptr, "Error before closing %s: %s",
                qUtf8Printable(real_filename),
                qUtf8Printable(fs->errorString()));
    return false;
  }

  return true;
}

/**
   Print log messages for any entries in the file which have
   not been looked up -- ie, unused or unrecognised entries.
   To mark an entry as used without actually doing anything with it,
   you could do something like:
      section_file_lookup(&file, "foo.bar");  / * unused * /
 */
void secfile_check_unused(const struct section_file *secfile)
{
  bool any = false;

  section_list_iterate(secfile_sections(secfile), psection)
  {
    entry_list_iterate(section_entries(psection), pentry)
    {
      if (!entry_used(pentry)) {
        if (!any && secfile->name) {
          qDebug("Unused entries in file %s:", secfile->name);
          any = true;
        }
        qCWarning(deprecations_category, "%s: unused entry: %s.%s",
                  secfile->name != nullptr ? secfile->name : "nameless",
                  section_name(psection), entry_name(pentry));
      }
    }
    entry_list_iterate_end;
  }
  section_list_iterate_end;
}

/**
   Return the filename the section file was loaded as, or "(anonymous)"
   if this sectionfile was created rather than loaded from file.
   The memory is managed internally, and should not be altered,
   nor used after secfile_destroy() called for the section file.
 */
const char *secfile_name(const struct section_file *secfile)
{
  if (nullptr == secfile) {
    return "nullptr";
  } else if (secfile->name) {
    return secfile->name;
  } else {
    return "(anonymous)";
  }
}

/**
   Seperates the section and entry names.  Create the section if missing.
 */
static struct section *secfile_insert_base(struct section_file *secfile,
                                           const char *path,
                                           const char **pent_name)
{
  char fullpath[MAX_LEN_SECPATH];
  char *ent_name;
  struct section *psection;

  sz_strlcpy(fullpath, path);

  ent_name = strchr(fullpath, '.');
  if (!ent_name) {
    SECFILE_LOG(secfile, nullptr,
                "Section and entry names must be separated by a dot.");
    return nullptr;
  }

  // Separates section and entry names.
  *ent_name = '\0';
  *pent_name = path + (ent_name - fullpath) + 1;
  psection = secfile_section_by_name(secfile, fullpath);
  if (psection) {
    return psection;
  } else {
    return secfile_section_new(secfile, fullpath);
  }
}

/**
   Insert a boolean entry.
 */
struct entry *secfile_insert_bool_full(struct section_file *secfile,
                                       bool value, const char *comment,
                                       bool allow_replace, const char *path,
                                       ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_BOOL == entry_type_get(pentry)) {
        if (!entry_bool_set(pentry, value)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_bool_new(psection, ent_name, value);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  return pentry;
}

/**
   Insert 'dim' boolean entries at 'path,0', 'path,1' etc.  Returns
   the number of entries inserted or replaced.
 */
size_t secfile_insert_bool_vec_full(struct section_file *secfile,
                                    const bool *values, size_t dim,
                                    const char *comment, bool allow_replace,
                                    const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0
      && nullptr
             != secfile_insert_bool_full(secfile, values[0], comment,
                                         allow_replace, "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim; i++) {
    if (nullptr
        != secfile_insert_bool_full(secfile, values[i], comment,
                                    allow_replace, "%s,%d", fullpath,
                                    static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Insert a integer entry.
 */
struct entry *secfile_insert_int_full(struct section_file *secfile,
                                      int value, const char *comment,
                                      bool allow_replace, const char *path,
                                      ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_INT == entry_type_get(pentry)) {
        if (!entry_int_set(pentry, value)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_int_new(psection, ent_name, value);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  return pentry;
}

/**
   Insert 'dim' integer entries at 'path,0', 'path,1' etc.  Returns
   the number of entries inserted or replaced.
 */
size_t secfile_insert_int_vec_full(struct section_file *secfile,
                                   const int *values, size_t dim,
                                   const char *comment, bool allow_replace,
                                   const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0
      && nullptr
             != secfile_insert_int_full(secfile, values[0], comment,
                                        allow_replace, "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim; i++) {
    if (nullptr
        != secfile_insert_int_full(secfile, values[i], comment,
                                   allow_replace, "%s,%d", fullpath,
                                   static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Insert a floating entry.
 */
struct entry *secfile_insert_float_full(struct section_file *secfile,
                                        float value, const char *comment,
                                        bool allow_replace, const char *path,
                                        ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_FLOAT == entry_type_get(pentry)) {
        if (!entry_float_set(pentry, value)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_float_new(psection, ent_name, value);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  return pentry;
}

/**
   Insert a include entry.
 */
struct section *secfile_insert_include(struct section_file *secfile,
                                       const char *filename)
{
  struct section *psection;
  char buffer[200];

  fc_snprintf(buffer, sizeof(buffer), "include_%u", secfile->num_includes++);

  fc_assert_ret_val(secfile_section_by_name(secfile, buffer) == nullptr,
                    nullptr);

  // Create include section.
  psection = secfile_section_new(secfile, buffer);
  psection->special = EST_INCLUDE;

  // Then add string entry "file" to it.
  secfile_insert_str_full(secfile, filename, nullptr, false, false,
                          EST_INCLUDE, "%s.file", buffer);

  return psection;
}

/**
   Insert a long comment entry.
 */
struct section *secfile_insert_long_comment(struct section_file *secfile,
                                            const char *comment)
{
  struct section *psection;
  char buffer[200];

  fc_snprintf(buffer, sizeof(buffer), "long_comment_%u",
              secfile->num_long_comments++);

  fc_assert_ret_val(secfile_section_by_name(secfile, buffer) == nullptr,
                    nullptr);

  // Create long comment section.
  psection = secfile_section_new(secfile, buffer);
  psection->special = EST_COMMENT;

  // Then add string entry "comment" to it.
  secfile_insert_str_full(secfile, comment, nullptr, false, true,
                          EST_COMMENT, "%s.comment", buffer);

  return psection;
}

/**
   Insert a string entry.
 */
struct entry *secfile_insert_str_full(struct section_file *secfile,
                                      const char *str, const char *comment,
                                      bool allow_replace, bool no_escape,
                                      enum entry_special_type stype,
                                      const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (psection->special != stype) {
    qCritical("Tried to insert wrong type of entry to section");
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_STR == entry_type_get(pentry)) {
        if (!entry_str_set(pentry, str)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_str_new(psection, ent_name, str, !no_escape);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  if (pentry != nullptr && stype == EST_COMMENT) {
    pentry->string.raw = true;
  }

  return pentry;
}

/**
   Insert 'dim' string entries at 'path,0', 'path,1' etc. Returns
   the number of entries inserted or replaced.
 */
size_t secfile_insert_str_vec_full(struct section_file *secfile,
                                   const char *const *strings, size_t dim,
                                   const char *comment, bool allow_replace,
                                   bool no_escape, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0
      && nullptr
             != secfile_insert_str_full(secfile, strings[0], comment,
                                        allow_replace, no_escape, EST_NORMAL,
                                        "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim; i++) {
    if (nullptr
        != secfile_insert_str_full(secfile, strings[i], comment,
                                   allow_replace, no_escape, EST_NORMAL,
                                   "%s,%d", fullpath, static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Insert up to 'dim' string entries at 'path,0', 'path,1' etc. Returns
   the number of entries inserted or replaced.
 */
size_t secfile_insert_str_vec_full(struct section_file *secfile,
                                   const QVector<QString> &strings,
                                   size_t dim, const char *comment,
                                   bool allow_replace, bool no_escape,
                                   const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0 && !strings.isEmpty()
      && nullptr
             != secfile_insert_str_full(secfile, qUtf8Printable(strings[0]),
                                        comment, allow_replace, no_escape,
                                        EST_NORMAL, "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim && i < strings.size(); i++) {
    if (nullptr
        != secfile_insert_str_full(
            secfile, qUtf8Printable(strings[i]), comment, allow_replace,
            no_escape, EST_NORMAL, "%s,%d", fullpath, static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Insert a read-from-a-file string entry
 */
struct entry *secfile_insert_filereference(struct section_file *secfile,
                                           const char *filename,
                                           const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (psection->special != EST_NORMAL) {
    qCritical("Tried to insert normal entry to different kind of section");
    return nullptr;
  }

  if (nullptr == pentry) {
    pentry = section_entry_filereference_new(psection, ent_name, filename);
  }

  return pentry;
}

/**
   Insert a enumerator entry.
 */
struct entry *secfile_insert_plain_enum_full(struct section_file *secfile,
                                             int enumerator,
                                             secfile_enum_name_fn_t name_fn,
                                             const char *comment,
                                             bool allow_replace,
                                             const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const char *str;
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, nullptr);
  str = name_fn(enumerator);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != str, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_STR == entry_type_get(pentry)) {
        if (!entry_str_set(pentry, str)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_str_new(psection, ent_name, str, true);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  return pentry;
}

/**
   Insert 'dim' string entries at 'path,0', 'path,1' etc.  Returns
   the number of entries inserted or replaced.
 */
size_t secfile_insert_plain_enum_vec_full(struct section_file *secfile,
                                          const int *enumurators, size_t dim,
                                          secfile_enum_name_fn_t name_fn,
                                          const char *comment,
                                          bool allow_replace,
                                          const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0
      && nullptr
             != secfile_insert_plain_enum_full(
                 secfile, enumurators[0], name_fn, comment, allow_replace,
                 "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim; i++) {
    if (nullptr
        != secfile_insert_plain_enum_full(secfile, enumurators[i], name_fn,
                                          comment, allow_replace, "%s,%d",
                                          fullpath, static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Insert a bitwise value entry.
 */
struct entry *secfile_insert_bitwise_enum_full(
    struct section_file *secfile, int bitwise_val,
    secfile_enum_name_fn_t name_fn, secfile_enum_iter_fn_t begin_fn,
    secfile_enum_iter_fn_t end_fn, secfile_enum_next_fn_t next_fn,
    const char *comment, bool allow_replace, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH], str[MAX_LEN_SECPATH];
  const char *ent_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;
  int i;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != begin_fn, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != end_fn, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != next_fn, nullptr);

  // Compute a string containing all the values separated by '|'.
  str[0] = '\0'; // Insert at least an empty string.
  if (0 != bitwise_val) {
    for (i = begin_fn(); i != end_fn(); i = next_fn(i)) {
      if (i & bitwise_val) {
        if ('\0' == str[0]) {
          sz_strlcpy(str, name_fn(i));
        } else {
          cat_snprintf(str, sizeof(str), "|%s", name_fn(i));
        }
      }
    }
  }

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_STR == entry_type_get(pentry)) {
        if (!entry_str_set(pentry, str)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_str_new(psection, ent_name, str, true);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  return pentry;
}

/**
   Insert 'dim' string entries at 'path,0', 'path,1' etc.  Returns
   the number of entries inserted or replaced.
 */
size_t secfile_insert_bitwise_enum_vec_full(
    struct section_file *secfile, const int *bitwise_vals, size_t dim,
    secfile_enum_name_fn_t name_fn, secfile_enum_iter_fn_t begin_fn,
    secfile_enum_iter_fn_t end_fn, secfile_enum_next_fn_t next_fn,
    const char *comment, bool allow_replace, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, 0);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != begin_fn, 0);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != end_fn, 0);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != next_fn, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0
      && nullptr
             != secfile_insert_bitwise_enum_full(
                 secfile, bitwise_vals[0], name_fn, begin_fn, end_fn,
                 next_fn, comment, allow_replace, "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim; i++) {
    if (nullptr
        != secfile_insert_bitwise_enum_full(
            secfile, bitwise_vals[i], name_fn, begin_fn, end_fn, next_fn,
            comment, allow_replace, "%s,%d", fullpath,
            static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Insert an enumerator value entry that we only have a name accessor
   function.
 */
struct entry *secfile_insert_enum_data_full(
    struct section_file *secfile, int value, bool bitwise,
    secfile_enum_name_data_fn_t name_fn, secfile_data_t data,
    const char *comment, bool allow_replace, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH], str[MAX_LEN_SECPATH];
  const char *ent_name, *val_name;
  struct section *psection;
  struct entry *pentry = nullptr;
  va_list args;
  int i;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, nullptr);

  if (bitwise) {
    // Compute a string containing all the values separated by '|'.
    str[0] = '\0'; // Insert at least an empty string.
    if (0 != value) {
      for (i = 0; (val_name = name_fn(data, i)); i++) {
        if ((1 << i) & value) {
          if ('\0' == str[0]) {
            sz_strlcpy(str, val_name);
          } else {
            cat_snprintf(str, sizeof(str), "|%s", val_name);
          }
        }
      }
    }
  } else {
    if (!(val_name = name_fn(data, value))) {
      SECFILE_LOG(secfile, nullptr, "Value %d not supported.", value);
      return nullptr;
    }
    sz_strlcpy(str, val_name);
  }

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  psection = secfile_insert_base(secfile, fullpath, &ent_name);
  if (!psection) {
    return nullptr;
  }

  if (allow_replace) {
    pentry = section_entry_by_name(psection, ent_name);
    if (nullptr != pentry) {
      if (ENTRY_STR == entry_type_get(pentry)) {
        if (!entry_str_set(pentry, str)) {
          return nullptr;
        }
      } else {
        entry_destroy(pentry);
        pentry = nullptr;
      }
    }
  }

  if (nullptr == pentry) {
    pentry = section_entry_str_new(psection, ent_name, str, true);
  }

  if (nullptr != pentry && nullptr != comment) {
    entry_set_comment(pentry, comment);
  }

  return pentry;
}

/**
   Insert 'dim' entries at 'path,0', 'path,1' etc.  Returns the number of
   entries inserted or replaced.
 */
size_t secfile_insert_enum_vec_data_full(
    struct section_file *secfile, const int *values, size_t dim,
    bool bitwise, secfile_enum_name_data_fn_t name_fn, secfile_data_t data,
    const char *comment, bool allow_replace, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i, ret = 0;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, 0);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, 0);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  /* NB: 'path,0' is actually 'path'.  See comment in the head
   * of the file. */
  if (dim > 0
      && nullptr
             != secfile_insert_enum_data_full(
                 secfile, values[0], bitwise, name_fn, data, comment,
                 allow_replace, "%s", fullpath)) {
    ret++;
  }
  for (i = 1; i < dim; i++) {
    if (nullptr
        != secfile_insert_enum_data_full(
            secfile, values[i], bitwise, name_fn, data, comment,
            allow_replace, "%s,%d", fullpath, static_cast<int>(i))) {
      ret++;
    }
  }

  return ret;
}

/**
   Returns the entry by the name or nullptr if not matched.
 */
struct entry *secfile_entry_by_path(const struct section_file *secfile,
                                    const char *path)
{
  char fullpath[MAX_LEN_SECPATH];
  char *ent_name;
  size_t len;
  struct section *psection;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  sz_strlcpy(fullpath, path);

  // treat "sec.foo,0" as "sec.foo":
  len = qstrlen(fullpath);
  if (len > 2 && fullpath[len - 2] == ',' && fullpath[len - 1] == '0') {
    fullpath[len - 2] = '\0';
  }

  if (nullptr != secfile->hash.entries) {
    struct entry *pentry = secfile->hash.entries->value(fullpath, nullptr);

    if (pentry) {
      entry_use(pentry);
    }
    return pentry;
  }

  /* I dont like strtok.
   * - Me neither! */
  ent_name = strchr(fullpath, '.');
  if (!ent_name) {
    return nullptr;
  }

  // Separates section and entry names.
  *ent_name++ = '\0';
  psection = secfile_section_by_name(secfile, fullpath);
  if (psection) {
    return section_entry_by_name(psection, ent_name);
  } else {
    return nullptr;
  }
}

/**
   Delete an entry.
 */
bool secfile_entry_delete(struct section_file *secfile, const char *path,
                          ...)
{
  char fullpath[MAX_LEN_SECPATH];
  va_list args;
  struct entry *pentry;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "Path %s does not exists.", fullpath);
    return false;
  }

  entry_destroy(pentry);

  return true;
}

/**
   Returns the entry at "fullpath" or nullptr if not matched.
 */
struct entry *secfile_entry_lookup(const struct section_file *secfile,
                                   const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  return secfile_entry_by_path(secfile, fullpath);
}

/**
   Lookup a boolean value in the secfile.  Returns TRUE on success.
 */
bool secfile_lookup_bool(const struct section_file *secfile, bool *bval,
                         const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return false;
  }

  return entry_bool_get(pentry, bval);
}

/**
   Lookup a boolean value in the secfile.  On failure, use the default
   value.
 */
bool secfile_lookup_bool_default(const struct section_file *secfile,
                                 bool def, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  bool bval;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, def);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    return def;
  }

  if (entry_bool_get(pentry, &bval)) {
    return bval;
  }

  return def;
}

/**
   Lookup a integer value in the secfile.  Returns TRUE on success.
 */
bool secfile_lookup_int(const struct section_file *secfile, int *ival,
                        const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return false;
  }

  return entry_int_get(pentry, ival);
}

/**
   Lookup a integer value in the secfile.  On failure, use the default
   value.
 */
int secfile_lookup_int_default(const struct section_file *secfile, int def,
                               const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  int ival;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, def);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    return def;
  }

  if (entry_int_get(pentry, &ival)) {
    return ival;
  }

  return def;
}

/**
   Lookup a integer value in the secfile.  The value will be arranged to
   match the interval [minval, maxval].  On failure, use the default
   value.
 */
int secfile_lookup_int_def_min_max(const struct section_file *secfile,
                                   int defval, int minval, int maxval,
                                   const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  int value;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, defval);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    return defval;
  }

  if (!entry_int_get(pentry, &value)) {
    return defval;
  }

  if (value < minval) {
    SECFILE_LOG(secfile, entry_section(pentry),
                "\"%s\" should be in the interval [%d, %d] but is %d;"
                "using the minimal value.",
                fullpath, minval, maxval, value);
    value = minval;
  }

  if (value > maxval) {
    SECFILE_LOG(secfile, entry_section(pentry),
                "\"%s\" should be in the interval [%d, %d] but is %d;"
                "using the maximal value.",
                fullpath, minval, maxval, value);
    value = maxval;
  }

  return value;
}

/**
   Lookup a integer vector in the secfile.  Returns nullptr on error.  This
   vector is not owned by the registry module, and should be free by the
   user.
 */
int *secfile_lookup_int_vec(const struct section_file *secfile, size_t *dim,
                            const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i = 0;
  int *vec;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != dim, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  // Check size.
  while (nullptr
         != secfile_entry_lookup(secfile, "%s,%d", fullpath,
                                 static_cast<int>(i))) {
    i++;
  }
  *dim = i;

  if (0 == i) {
    // Doesn't exist.
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return nullptr;
  }

  vec = new int[i];
  for (i = 0; i < *dim; i++) {
    if (!secfile_lookup_int(secfile, vec + i, "%s,%d", fullpath,
                            static_cast<int>(i))) {
      SECFILE_LOG(secfile, nullptr,
                  "An error occurred when looking up to \"%s,%d\" entry.",
                  fullpath, (int) i);
      delete[] vec;
      *dim = 0;
      return nullptr;
    }
  }

  return vec;
}

/**
   Lookup a string value in the secfile.  Returns nullptr on error.
 */
const char *secfile_lookup_str(const struct section_file *secfile,
                               const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return nullptr;
  }

  if (entry_str_get(pentry, &str)) {
    return str;
  }

  return nullptr;
}

/**
   Lookup a string value in the secfile.  On failure, use the default
   value.
 */
const char *secfile_lookup_str_default(const struct section_file *secfile,
                                       const char *def, const char *path,
                                       ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, def);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    return def;
  }

  if (entry_str_get(pentry, &str)) {
    return str;
  }

  return def;
}

/**
   Lookup a string vector in the secfile.  Returns nullptr on error.  This
   vector is not owned by the registry module, and should be free by the
   user, but the string pointers stored inside the vector shouldn't be
   free.
 */
const char **secfile_lookup_str_vec(const struct section_file *secfile,
                                    size_t *dim, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i = 0;
  const char **vec;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != dim, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  // Check size.
  while (nullptr
         != secfile_entry_lookup(secfile, "%s,%d", fullpath,
                                 static_cast<int>(i))) {
    i++;
  }
  *dim = i;

  if (0 == i) {
    // Doesn't exist.
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return nullptr;
  }

  vec = new const char *[i];
  for (i = 0; i < *dim; i++) {
    if (!(vec[i] = secfile_lookup_str(secfile, "%s,%d", fullpath,
                                      static_cast<int>(i)))) {
      SECFILE_LOG(secfile, nullptr,
                  "An error occurred when looking up to \"%s,%d\" entry.",
                  fullpath, (int) i);
      delete[] vec;
      *dim = 0;
      return nullptr;
    }
  }

  return vec;
}

/**
   Lookup an enumerator value in the secfile.  Returns FALSE on error.
 */
bool secfile_lookup_plain_enum_full(const struct section_file *secfile,
                                    int *penumerator,
                                    secfile_enum_is_valid_fn_t is_valid_fn,
                                    secfile_enum_by_name_fn_t by_name_fn,
                                    const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != penumerator,
                             false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != is_valid_fn,
                             false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != by_name_fn, false);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return false;
  }

  if (!entry_str_get(pentry, &str)) {
    return false;
  }

  *penumerator = by_name_fn(str, strcmp);
  if (is_valid_fn(*penumerator)) {
    return true;
  }

  SECFILE_LOG(secfile, entry_section(pentry),
              "Entry \"%s\": no match for \"%s\".", entry_name(pentry), str);
  return false;
}

/**
   Lookup an enumerator value in the secfile.  Returns 'defval' on error.
 */
int secfile_lookup_plain_enum_default_full(
    const struct section_file *secfile, int defval,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str;
  int val;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, defval);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != is_valid_fn,
                             defval);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != by_name_fn,
                             defval);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    return defval;
  }

  if (!entry_str_get(pentry, &str)) {
    return defval;
  }

  val = by_name_fn(str, strcmp);
  if (is_valid_fn(val)) {
    return val;
  } else {
    return defval;
  }
}

/**
   Lookup a enumerator vector in the secfile.  Returns nullptr on error. This
   vector is not owned by the registry module, and should be free by the
   user.
 */
int *secfile_lookup_plain_enum_vec_full(
    const struct section_file *secfile, size_t *dim,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i = 0;
  int *vec;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != dim, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != is_valid_fn,
                             nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != by_name_fn,
                             nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  // Check size.
  while (nullptr
         != secfile_entry_lookup(secfile, "%s,%d", fullpath,
                                 static_cast<int>(i))) {
    i++;
  }
  *dim = i;

  if (0 == i) {
    // Doesn't exist.
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return nullptr;
  }

  vec = new int[i];
  for (i = 0; i < *dim; i++) {
    if (!secfile_lookup_plain_enum_full(secfile, vec + i, is_valid_fn,
                                        by_name_fn, "%s,%d", fullpath,
                                        static_cast<int>(i))) {
      SECFILE_LOG(secfile, nullptr,
                  "An error occurred when looking up to \"%s,%d\" entry.",
                  fullpath, (int) i);
      delete[] vec;
      *dim = 0;
      return nullptr;
    }
  }

  return vec;
}

/**
   Lookup a bitwise enumerator value in the secfile.  Returns FALSE on error.
 */
bool secfile_lookup_bitwise_enum_full(const struct section_file *secfile,
                                      int *penumerator,
                                      secfile_enum_is_valid_fn_t is_valid_fn,
                                      secfile_enum_by_name_fn_t by_name_fn,
                                      const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str, *p;
  char val_name[MAX_LEN_SECPATH];
  int val;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != penumerator,
                             false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != is_valid_fn,
                             false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != by_name_fn, false);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return false;
  }

  if (!entry_str_get(pentry, &str)) {
    return false;
  }

  *penumerator = 0;
  if ('\0' == str[0]) {
    // Empty string = no value.
    return true;
  }

  // Value names are separated by '|'.
  do {
    p = strchr(str, '|');
    if (nullptr != p) {
      p++;
      fc_strlcpy(val_name, str, p - str);
    } else {
      // Last segment, full copy.
      sz_strlcpy(val_name, str);
    }
    remove_leading_trailing_spaces(val_name);
    val = by_name_fn(val_name, strcmp);
    if (!is_valid_fn(val)) {
      SECFILE_LOG(secfile, entry_section(pentry),
                  "Entry \"%s\": no match for \"%s\".", entry_name(pentry),
                  val_name);
      return false;
    }
    *penumerator |= val;
    str = p;
  } while (nullptr != p);

  return true;
}

/**
   Lookup an enumerator value in the secfile.  Returns 'defval' on error.
 */
int secfile_lookup_bitwise_enum_default_full(
    const struct section_file *secfile, int defval,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str, *p;
  char val_name[MAX_LEN_SECPATH];
  int val, full_val;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, defval);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != is_valid_fn,
                             defval);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != by_name_fn,
                             defval);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    return defval;
  }

  if (!entry_str_get(pentry, &str)) {
    return defval;
  }

  if ('\0' == str[0]) {
    // Empty string = no value.
    return 0;
  }

  // Value names are separated by '|'.
  full_val = 0;
  do {
    p = strchr(str, '|');
    if (nullptr != p) {
      p++;
      fc_strlcpy(val_name, str, p - str);
    } else {
      // Last segment, full copy.
      sz_strlcpy(val_name, str);
    }
    remove_leading_trailing_spaces(val_name);
    val = by_name_fn(val_name, strcmp);
    if (!is_valid_fn(val)) {
      return defval;
    }
    full_val |= val;
    str = p;
  } while (nullptr != p);

  return full_val;
}

/**
   Lookup a enumerator vector in the secfile.  Returns nullptr on error. This
   vector is not owned by the registry module, and should be free by the
   user.
 */
int *secfile_lookup_bitwise_enum_vec_full(
    const struct section_file *secfile, size_t *dim,
    secfile_enum_is_valid_fn_t is_valid_fn,
    secfile_enum_by_name_fn_t by_name_fn, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  size_t i = 0;
  int *vec;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != dim, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != is_valid_fn,
                             nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != by_name_fn,
                             nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  // Check size.
  while (nullptr
         != secfile_entry_lookup(secfile, "%s,%d", fullpath,
                                 static_cast<int>(i))) {
    i++;
  }
  *dim = i;

  if (0 == i) {
    // Doesn't exist.
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return nullptr;
  }

  vec = new int[i];
  for (i = 0; i < *dim; i++) {
    if (!secfile_lookup_bitwise_enum_full(secfile, vec + i, is_valid_fn,
                                          by_name_fn, "%s,%d", fullpath,
                                          static_cast<int>(i))) {
      SECFILE_LOG(secfile, nullptr,
                  "An error occurred when looking up to \"%s,%d\" entry.",
                  fullpath, (int) i);
      delete[] vec;
      *dim = 0;

      return nullptr;
    }
  }

  return vec;
}

/**
   Lookup a value saved as string in the secfile.  Returns FALSE on error.
 */
bool secfile_lookup_enum_data(const struct section_file *secfile,
                              int *pvalue, bool bitwise,
                              secfile_enum_name_data_fn_t name_fn,
                              secfile_data_t data, const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str, *p, *name;
  char val_name[MAX_LEN_SECPATH];
  int val;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != pvalue, false);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, false);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return false;
  }

  if (!entry_str_get(pentry, &str)) {
    return false;
  }

  if (bitwise) {
    *pvalue = 0;
    if ('\0' == str[0]) {
      // Empty string = no value.
      return true;
    }

    // Value names are separated by '|'.
    do {
      p = strchr(str, '|');
      if (nullptr != p) {
        p++;
        fc_strlcpy(val_name, str, p - str);
      } else {
        // Last segment, full copy.
        sz_strlcpy(val_name, str);
      }
      remove_leading_trailing_spaces(val_name);
      for (val = 0; (name = name_fn(data, val)); val++) {
        if (0 == fc_strcasecmp(name, val_name)) {
          break;
        }
      }
      if (nullptr == name) {
        SECFILE_LOG(secfile, entry_section(pentry),
                    "Entry \"%s\": no match for \"%s\".", entry_name(pentry),
                    val_name);
        return false;
      }
      *pvalue |= 1 << val;
      str = p;
    } while (nullptr != p);
  } else {
    for (val = 0; (name = name_fn(data, val)); val++) {
      if (0 == fc_strcasecmp(name, str)) {
        *pvalue = val;
        break;
      }
    }
    if (nullptr == name) {
      SECFILE_LOG(secfile, entry_section(pentry),
                  "Entry \"%s\": no match for \"%s\".", entry_name(pentry),
                  str);
      return false;
    }
  }

  return true;
}

/**
   Lookup a value saved as string in the secfile.  Returns 'defval' on error.
 */
int secfile_lookup_enum_default_data(const struct section_file *secfile,
                                     int defval, bool bitwise,
                                     secfile_enum_name_data_fn_t name_fn,
                                     secfile_data_t data, const char *path,
                                     ...)
{
  char fullpath[MAX_LEN_SECPATH];
  const struct entry *pentry;
  const char *str, *p, *name;
  char val_name[MAX_LEN_SECPATH];
  int value, val;
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, defval);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != name_fn, defval);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  if (!(pentry = secfile_entry_by_path(secfile, fullpath))) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" entry doesn't exist.", fullpath);
    return defval;
  }

  if (!entry_str_get(pentry, &str)) {
    return defval;
  }

  value = 0;
  if (bitwise) {
    if ('\0' == str[0]) {
      // Empty string = no value.
      return value;
    }

    // Value names are separated by '|'.
    do {
      p = strchr(str, '|');
      if (nullptr != p) {
        p++;
        fc_strlcpy(val_name, str, p - str);
      } else {
        // Last segment, full copy.
        sz_strlcpy(val_name, str);
      }
      remove_leading_trailing_spaces(val_name);
      for (val = 0; (name = name_fn(data, val)); val++) {
        if (0 == strcmp(name, val_name)) {
          break;
        }
      }
      if (nullptr == name) {
        SECFILE_LOG(secfile, entry_section(pentry),
                    "Entry \"%s\": no match for \"%s\".", entry_name(pentry),
                    val_name);
        return defval;
      }
      value |= 1 << val;
      str = p;
    } while (nullptr != p);
  } else {
    for (val = 0; (name = name_fn(data, val)); val++) {
      if (0 == strcmp(name, str)) {
        value = val;
        break;
      }
    }
    if (nullptr == name) {
      SECFILE_LOG(secfile, entry_section(pentry),
                  "Entry \"%s\": no match for \"%s\".", entry_name(pentry),
                  str);
      return defval;
    }
  }

  return value;
}

/**
   Returns the first section matching the name.
 */
struct section *secfile_section_by_name(const struct section_file *secfile,
                                        const QString &name)
{
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  section_list_iterate(secfile->sections, psection)
  {
    if (section_name(psection) == name) {
      return psection;
    }
  }
  section_list_iterate_end;

  return nullptr;
}

/**
   Find a section by path.
 */
struct section *secfile_section_lookup(const struct section_file *secfile,
                                       const char *path, ...)
{
  char fullpath[MAX_LEN_SECPATH];
  va_list args;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  va_start(args, path);
  fc_vsnprintf(fullpath, sizeof(fullpath), path, args);
  va_end(args);

  return secfile_section_by_name(secfile, fullpath);
}

/**
   Returns the list of sections.  This list is owned by the registry module
   and shouldn't be modified and destroyed.
 */
const struct section_list *
secfile_sections(const struct section_file *secfile)
{
  return (nullptr != secfile ? secfile->sections : nullptr);
}

/**
   Returns the list of sections which match the name prefix.  Returns nullptr
   if no section was found.  This list is not owned by the registry module
   and the user must destroy it when he finished to work with it.
 */
struct section_list *
secfile_sections_by_name_prefix(const struct section_file *secfile,
                                const char *prefix)
{
  struct section_list *matches = nullptr;
  size_t len;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);
  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != prefix, nullptr);

  len = qstrlen(prefix);
  if (0 == len) {
    return nullptr;
  }

  section_list_iterate(secfile->sections, psection)
  {
    if (0 == strncmp(section_name(psection), prefix, len)) {
      if (nullptr == matches) {
        matches = section_list_new();
      }
      section_list_append(matches, psection);
    }
  }
  section_list_iterate_end;

  return matches;
}

/**
   Create a new section in the secfile.
 */
struct section *secfile_section_new(struct section_file *secfile,
                                    const QString &name)
{
  struct section *psection;

  SECFILE_RETURN_VAL_IF_FAIL(secfile, nullptr, nullptr != secfile, nullptr);

  if (nullptr == name || '\0' == name[0]) {
    SECFILE_LOG(secfile, nullptr, "Cannot create a section without name.");
    return nullptr;
  }

  if (!is_secfile_entry_name_valid(name)) {
    SECFILE_LOG(secfile, nullptr, "\"%s\" is not a valid section name.",
                qUtf8Printable(name));
    return nullptr;
  }

  if (nullptr != secfile_section_by_name(secfile, name)) {
    /* We cannot duplicate sections in any case! Not even if one is
     * include -section and the other not. */
    SECFILE_LOG(secfile, nullptr, "Section \"%s\" already exists.",
                qUtf8Printable(name));
    return nullptr;
  }

  psection = new section;
  psection->special = EST_NORMAL;
  psection->name = qstrdup(qUtf8Printable(name));
  psection->entries = entry_list_new_full(entry_destroy);

  // Append to secfile.
  psection->secfile = secfile;
  section_list_append(secfile->sections, psection);

  if (nullptr != secfile->hash.sections) {
    secfile->hash.sections->insert(psection->name, psection);
  }

  return psection;
}

/**
   Remove this section from the secfile.
 */
void section_destroy(struct section *psection)
{
  struct section_file *secfile;

  SECFILE_RETURN_IF_FAIL(nullptr, psection, nullptr != psection);

  section_clear_all(psection);

  if ((secfile = psection->secfile)) {
    // Detach from secfile.
    if (section_list_remove(secfile->sections, psection)) {
      // This has called section_destroy() already then.
      return;
    }
    if (nullptr != secfile->hash.sections) {
      secfile->hash.sections->remove(psection->name);
    }
  }

  entry_list_destroy(psection->entries);
  delete[] psection->name;
  delete psection;
}

/**
   Remove all entries.
 */
void section_clear_all(struct section *psection)
{
  SECFILE_RETURN_IF_FAIL(nullptr, psection, nullptr != psection);

  // This include the removing of the hash datas.
  entry_list_clear(psection->entries);

  if (0 < entry_list_size(psection->entries)) {
    SECFILE_LOG(psection->secfile, psection,
                "After clearing all, %d entries are still remaining.",
                entry_list_size(psection->entries));
  }
}

/**
   Returns a list containing all the entries.  This list is owned by the
   secfile, so don't modify or destroy it.
 */
const struct entry_list *section_entries(const struct section *psection)
{
  return (nullptr != psection ? psection->entries : nullptr);
}

/**
   Returns the first entry matching the name.
 */
struct entry *section_entry_by_name(const struct section *psection,
                                    const QString &name)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, psection, nullptr != psection,
                             nullptr);

  entry_list_iterate(psection->entries, pentry)
  {
    if (entry_name(pentry) == name) {
      entry_use(pentry);
      return pentry;
    }
  }
  entry_list_iterate_end;

  return nullptr;
}

/**
   Returns a new entry.
 */
static entry *entry_new(struct section *psection, const QString &name)
{
  struct section_file *secfile;
  struct entry *pentry;

  SECFILE_RETURN_VAL_IF_FAIL(nullptr, psection, nullptr != psection,
                             nullptr);

  secfile = psection->secfile;
  if (nullptr == name || '\0' == name[0]) {
    SECFILE_LOG(secfile, psection, "Cannot create an entry without name.");
    return nullptr;
  }

  if (!is_secfile_entry_name_valid(name)) {
    SECFILE_LOG(secfile, psection, "\"%s\" is not a valid entry name.",
                qUtf8Printable(name));
    return nullptr;
  }

  if (!secfile->allow_duplicates
      && nullptr != section_entry_by_name(psection, name)) {
    SECFILE_LOG(secfile, psection, "Entry \"%s\" already exists.",
                qUtf8Printable(name));
    return nullptr;
  }

  pentry = new entry;
  pentry->name = qstrdup(qUtf8Printable(name));
  pentry->type = ENTRY_ILLEGAL; // Invalid case
  pentry->used = 0;
  pentry->comment = nullptr;

  // Append to section.
  pentry->psection = psection;
  entry_list_append(psection->entries, pentry);

  // Notify secfile.
  secfile->num_entries++;
  secfile_hash_insert(secfile, pentry);

  return pentry;
}

/**
   Returns a new entry of type ENTRY_INT.
 */
struct entry *section_entry_int_new(struct section *psection,
                                    const QString &name, int value)
{
  struct entry *pentry = entry_new(psection, name);

  if (nullptr != pentry) {
    pentry->type = ENTRY_INT;
    pentry->integer.value = value;
  }

  return pentry;
}

/**
   Returns a new entry of type ENTRY_BOOL.
 */
struct entry *section_entry_bool_new(struct section *psection,
                                     const QString &name, bool value)
{
  struct entry *pentry = entry_new(psection, name);

  if (nullptr != pentry) {
    pentry->type = ENTRY_BOOL;
    pentry->boolean.value = value;
  }

  return pentry;
}

/**
   Returns a new entry of type ENTRY_FLOAT.
 */
struct entry *section_entry_float_new(struct section *psection,
                                      const QString &name, float value)
{
  struct entry *pentry = entry_new(psection, name);

  if (nullptr != pentry) {
    pentry->type = ENTRY_FLOAT;
    pentry->floating.value = value;
  }

  return pentry;
}

/**
   Returns a new entry of type ENTRY_STR.
 */
struct entry *section_entry_str_new(struct section *psection,
                                    const QString &name,
                                    const QString &value, bool escaped)
{
  struct entry *pentry = entry_new(psection, name);

  if (nullptr != pentry) {
    pentry->type = ENTRY_STR;
    pentry->string.value = qstrdup(qUtf8Printable(value));
    pentry->string.escaped = escaped;
    pentry->string.raw = false;
    pentry->string.gt_marking = false;
  }

  return pentry;
}

/**
   Returns a new entry of type ENTRY_FILEREFERENCE.
 */
static struct entry *
section_entry_filereference_new(struct section *psection, const char *name,
                                const char *value)
{
  struct entry *pentry = entry_new(psection, name);

  if (nullptr != pentry) {
    pentry->type = ENTRY_FILEREFERENCE;
    pentry->string.value = fc_strdup(nullptr != value ? value : "");
  }

  return pentry;
}

/**
   Entry structure destructor.
 */
void entry_destroy(struct entry *pentry)
{
  struct section_file *secfile;
  struct section *psection;

  if (nullptr == pentry) {
    return;
  }

  if ((psection = pentry->psection)) {
    // Detach from section.
    if (entry_list_remove(psection->entries, pentry)) {
      // This has called entry_destroy() already then.
      return;
    }
    if ((secfile = psection->secfile)) {
      // Detach from secfile.
      secfile->num_entries--;
      secfile_hash_delete(secfile, pentry);
    }
  }

  // Specific type free.
  switch (pentry->type) {
  case ENTRY_BOOL:
  case ENTRY_INT:
  case ENTRY_FLOAT:
    break;

  case ENTRY_STR:
  case ENTRY_FILEREFERENCE:
    delete[] pentry->string.value;
    break;

  case ENTRY_ILLEGAL:
    fc_assert(pentry->type != ENTRY_ILLEGAL);
    break;
  }

  // Common free.
  delete[] pentry->name;
  delete[] pentry->comment;
  delete pentry;
  pentry = nullptr;
}

/**
   Returns the parent section of this entry.
 */
struct section *entry_section(const struct entry *pentry)
{
  return (nullptr != pentry ? pentry->psection : nullptr);
}

/**
   Returns the type of this entry or ENTRY_ILLEGAL or error.
 */
enum entry_type entry_type_get(const struct entry *pentry)
{
  return (nullptr != pentry ? pentry->type : ENTRY_ILLEGAL);
}

/**
   Build the entry path.  Returns like snprintf().
 */
int entry_path(const struct entry *pentry, char *buf, size_t buf_len)
{
  return fc_snprintf(buf, buf_len, "%s.%s",
                     section_name(entry_section(pentry)),
                     entry_name(pentry));
}

/**
   Returns the name of this entry.
 */
const char *entry_name(const struct entry *pentry)
{
  return (nullptr != pentry ? pentry->name : nullptr);
}

/**
   Sets the name of the entry.  Returns TRUE on success.
 */
bool entry_set_name(struct entry *pentry, const char *name)
{
  struct section *psection;
  struct section_file *secfile;

  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  psection = pentry->psection;
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != psection, false);
  secfile = psection->secfile;
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, psection, nullptr != secfile, false);

  if (nullptr == name || '\0' == name[0]) {
    SECFILE_LOG(secfile, psection, "No new name for entry \"%s\".",
                pentry->name);
    return false;
  }

  if (!is_secfile_entry_name_valid(name)) {
    SECFILE_LOG(secfile, psection,
                "\"%s\" is not a valid entry name for entry \"%s\".", name,
                pentry->name);
    return false;
  }

  if (!secfile->allow_duplicates) {
    struct entry *pother = section_entry_by_name(psection, name);

    if (nullptr != pother && pother != pentry) {
      SECFILE_LOG(secfile, psection, "Entry \"%s\" already exists.", name);
      return false;
    }
  }

  // Remove from hash table the old path.
  secfile_hash_delete(secfile, pentry);

  // Really rename the entry.
  free(pentry->name);
  pentry->name = fc_strdup(name);

  // Insert into hash table the new path.
  secfile_hash_insert(secfile, pentry);
  return true;
}

/**
   Returns the comment associated to this entry.
 */
const char *entry_comment(const struct entry *pentry)
{
  return (nullptr != pentry ? pentry->comment : nullptr);
}

/**
   Sets a comment for the entry.  Pass nullptr to remove the current one.
 */
void entry_set_comment(struct entry *pentry, const QString &comment)
{
  if (nullptr == pentry) {
    return;
  }

  pentry->comment =
      (!comment.isEmpty() ? fc_strdup(qUtf8Printable(comment)) : nullptr);
}

/**
   Returns TRUE if this entry has been used.
 */
static inline bool entry_used(const struct entry *pentry)
{
  return (0 < pentry->used);
}

/**
   Increase the used count.
 */
static inline void entry_use(struct entry *pentry) { pentry->used++; }

/**
   Gets an boolean value.  Returns TRUE on success.
   On old saved files, 0 and 1 can also be considered as bool.
 */
bool entry_bool_get(const struct entry *pentry, bool *value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);

  if (ENTRY_INT == pentry->type
      && (pentry->integer.value == 0 || pentry->integer.value == 1)
      && nullptr != pentry->psection->secfile
      && pentry->psection->secfile->allow_digital_boolean) {
    *value = (0 != pentry->integer.value);
    return true;
  }

  SECFILE_RETURN_VAL_IF_FAIL(
      pentry->psection->secfile, pentry->psection,
      pentry->psection != nullptr && ENTRY_BOOL == pentry->type, false);

  if (nullptr != value) {
    *value = pentry->boolean.value;
  }
  return true;
}

/**
   Sets an boolean value.  Returns TRUE on success.
 */
bool entry_bool_set(struct entry *pentry, bool value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_BOOL == pentry->type, false);

  pentry->boolean.value = value;
  return true;
}

/**
   Gets an floating value. Returns TRUE on success.
 */
bool entry_float_get(const struct entry *pentry, float *value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_FLOAT == pentry->type, false);

  if (nullptr != value) {
    *value = pentry->floating.value;
  }

  return true;
}

/**
   Sets an floating value. Returns TRUE on success.
 */
bool entry_float_set(struct entry *pentry, float value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_FLOAT == pentry->type, false);

  pentry->floating.value = value;

  return true;
}

/**
   Gets an integer value.  Returns TRUE on success.
 */
bool entry_int_get(const struct entry *pentry, int *value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_INT == pentry->type, false);

  if (nullptr != value) {
    *value = pentry->integer.value;
  }
  return true;
}

/**
   Sets an integer value.  Returns TRUE on success.
 */
bool entry_int_set(struct entry *pentry, int value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_INT == pentry->type, false);

  pentry->integer.value = value;
  return true;
}

/**
   Gets an string value.  Returns TRUE on success.
 */
bool entry_str_get(const struct entry *pentry, const char **value)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_STR == pentry->type, false);

  if (nullptr != value) {
    *value = pentry->string.value;
  }
  return true;
}

/**
   Sets an string value.  Returns TRUE on success.
 */
bool entry_str_set(struct entry *pentry, const char *value)
{
  char *old_val;

  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_STR == pentry->type, false);

  /* We free() old value only after we've placed the new one, to
   * support secfile_replace_str_vec() calls that want to keep some of
   * the entries from the old vector in the new one. We don't want
   * to lose the entry in between. */
  old_val = pentry->string.value;
  pentry->string.value = fc_strdup(nullptr != value ? value : "");
  free(old_val);
  return true;
}

/**
   Sets if the string should get gettext marking. Returns TRUE on success.
 */
bool entry_str_set_gt_marking(struct entry *pentry, bool gt_marking)
{
  SECFILE_RETURN_VAL_IF_FAIL(nullptr, nullptr, nullptr != pentry, false);
  SECFILE_RETURN_VAL_IF_FAIL(pentry->psection->secfile, pentry->psection,
                             ENTRY_STR == pentry->type, false);

  pentry->string.gt_marking = gt_marking;

  return true;
}

/**
   Push an entry into a file stream.
 */
static bool entry_to_file(const struct entry *pentry, QIODevice *fs)
{
  static char buf[8192];

  switch (pentry->type) {
  case ENTRY_BOOL:
    fc_assert_ret_val(
        fs->write(pentry->boolean.value ? "TRUE" : "FALSE") > 0, false);
    break;
  case ENTRY_INT: {
    auto number = QString::number(pentry->integer.value);
    fc_assert_ret_val(fs->write(number.toUtf8()) > 0, false);
  } break;
  case ENTRY_FLOAT: {
    auto number = QString::number(pentry->floating.value, 'f');
    if (!number.contains('.')) {
      number += QStringLiteral(".0");
    }
    fc_assert_ret_val(fs->write(number.toUtf8()) > 0, false);
  } break;
  case ENTRY_STR:
    if (pentry->string.escaped) {
      make_escapes(pentry->string.value, buf, sizeof(buf));
      if (pentry->string.gt_marking) {
        fc_assert_ret_val(fs->write("_(\"" + QByteArray(buf) + "\")") > 0,
                          false);
      } else {
        fc_assert_ret_val(fs->write("\"" + QByteArray(buf) + "\"") > 0,
                          false);
      }
    } else if (pentry->string.raw) {
      fc_assert_ret_val(fs->write(pentry->string.value) > 0, false);
    } else {
      fc_assert_ret_val(
          fs->write("$" + QByteArray(pentry->string.value) + "$") > 0,
          false);
    }
    break;
  case ENTRY_FILEREFERENCE:
    fc_assert_ret_val(
        fs->write("*" + QByteArray(pentry->string.value) + "*") > 0, false);
    break;
  case ENTRY_ILLEGAL:
    fc_assert(pentry->type != ENTRY_ILLEGAL);
    return false;
  }

  return true;
}

/**
   Creates a new entry from the token.
 */
static void entry_from_inf_token(struct section *psection,
                                 const QString &name, const QString &tok,
                                 struct inputfile *inf)
{
  if (!entry_from_token(psection, name, tok)) {
    qCritical("%s", qUtf8Printable(
                        inf_log_str(inf, "Entry value not recognized: %s",
                                    qUtf8Printable(tok))));
  }
}
