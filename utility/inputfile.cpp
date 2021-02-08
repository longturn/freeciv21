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

/***********************************************************************
  A low-level object for reading a registry-format file.
  original author: David Pfitzner <dwp@mso.anu.edu.au>

  This module implements an object which is useful for reading/parsing
  a file in the registry format of registry.c.  It takes care of the
  low-level file-reading details, and provides functions to return
  specific "tokens" from the file.  Probably this should really use
  higher-level tools... (flex/lex bison/yacc?)

  When the user tries to read a token, we return a (const char*)
  pointing to some data if the token was found, or NULL otherwise.
  The data pointed to should not be modified.  The retuned pointer
  is valid _only_ until another inputfile is performed.  (So should
  be used immediately, or fc_strdup-ed etc.)

  The tokens recognised are as follows:
  (Single quotes are delimiters used here, but are not part of the
  actual tokens/strings.)
  Most tokens can be preceeded by optional whitespace; exceptions
  are section_name and entry_name.

  section_name:  '[foo]'
  returned token: 'foo'

  entry_name:  'foo =' (optional whitespace allowed before '=')
  returned token: 'foo'

  end_of_line: newline, or optional '#' or ';' (comment characters)
               followed by any other chars, then newline.
  returned token: should not be used except to check non-NULL.

  table_start: '{'
  returned token: should not be used except to check non-NULL.

  table_end: '}'
  returned token: should not be used except to check non-NULL.

  comma:  literal ','
  returned token: should not be used except to check non-NULL.

  value:  a signed integer, or a double-quoted string, or a
          gettext-marked double quoted string.  Strings _may_ contain
          raw embedded newlines, and escaped doublequotes, or \.
          eg:  '123', '-999', '"foo"', '_("foo")'
  returned token: string containing number, for numeric, or string
          starting at first doublequote for strings, but ommiting
          trailing double-quote.  Note this does _not_ translate
          escaped doublequotes etc back to normal.

***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <cstdarg>
#include <cstdio>
#include <cstring>

// Qt
#include <QLoggingCategory>

// KArchive
#include <KFilterDev>

// utility
#include "fcintl.h"

#include "inputfile.h"

#define INF_MAGIC (0xabdc0132) // arbitrary

struct inputfile {
  unsigned int magic;        // memory check
  QString filename;          // filename as passed to fopen
  QIODevice *fp;             // read from this
  QString cur_line;          // data from current line
  unsigned int cur_line_pos; // position in current line
  unsigned int line_num;     // line number from file in cur_line
  QString partial;           /* used in accumulating multi-line strings;
                                used only in get_token_value, but put
                                here so it gets freed when file closed */
  QString token;             // data returned to user
  datafilename_fn_t datafn;  /* function like datafilename(); use a
                                function pointer just to keep this
                                inputfile module "generic" */
  bool in_string;            /* set when reading multi-line strings,
                                to know not to handle *include at start
                                of line as include mechanism */
  int string_start_line;     /* when in_string is true, this is the
                                start line of current string */
  struct inputfile *included_from; /* NULL for toplevel file, otherwise
                                      points back to files which this one
                                      has been included from */
};

// A function to get a specific token type:
typedef QString (*get_token_fn_t)(struct inputfile *inf);

static QString get_token_section_name(struct inputfile *inf);
static QString get_token_entry_name(struct inputfile *inf);
static QString get_token_eol(struct inputfile *inf);
static QString get_token_table_start(struct inputfile *inf);
static QString get_token_table_end(struct inputfile *inf);
static QString get_token_comma(struct inputfile *inf);
static QString get_token_value(struct inputfile *inf);

static struct {
  const char *name;
  get_token_fn_t func;
} tok_tab[INF_TOK_LAST] = {
    {"section_name", get_token_section_name},
    {"entry_name", get_token_entry_name},
    {"end_of_line", get_token_eol},
    {"table_start", get_token_table_start},
    {"table_end", get_token_table_end},
    {"comma", get_token_comma},
    {"value", get_token_value},
};

static bool read_a_line(struct inputfile *inf);

Q_LOGGING_CATEGORY(inf_category, "freeciv.inputfile")

/**
   Return true if c is a 'comment' character: '#' or ';'
 */
template <class Char> static bool is_comment(Char c)
{
  return (c == '#' || c == ';');
}

/**
   Set values to zeros; should have free'd/closed everything before
   this if appropriate.
 */
static void init_zeros(struct inputfile *inf)
{
  fc_assert_ret(NULL != inf);
  inf->magic = INF_MAGIC;
  inf->filename.clear();
  inf->fp = NULL;
  inf->datafn = NULL;
  inf->included_from = NULL;
  inf->line_num = inf->cur_line_pos = 0;
  inf->in_string = false;
  inf->string_start_line = 0;
  inf->cur_line.clear();
  inf->token.clear();
  inf->partial.clear();
  inf->partial.reserve(200);
}

/**
   Check sensible values for an opened inputfile.
 */
static bool inf_sanity_check(struct inputfile *inf)
{
  fc_assert_ret_val(NULL != inf, false);
  fc_assert_ret_val(INF_MAGIC == inf->magic, false);
  fc_assert_ret_val(NULL != inf->fp, false);
  fc_assert_ret_val(false == inf->in_string || true == inf->in_string,
                    false);

#ifdef FREECIV_DEBUG
  fc_assert_ret_val(0 <= inf->string_start_line, false);
  if (inf->included_from && !inf_sanity_check(inf->included_from)) {
    return false;
  }
#endif // FREECIV_DEBUG

  return true;
}

/**
   Return the filename the inputfile was loaded as, or "(anonymous)"
   if this inputfile was loaded from a stream rather than from a file.
 */
static QString inf_filename(struct inputfile *inf)
{
  if (inf->filename.isEmpty()) {
    return QStringLiteral("(anonymous)");
  } else {
    return inf->filename;
  }
}

/**
   Open the file, and return an allocated, initialized structure.
   Returns NULL if the file could not be opened.
 */
struct inputfile *inf_from_file(const QString &filename,
                                datafilename_fn_t datafn)
{
  struct inputfile *inf;

  fc_assert_ret_val(!filename.isEmpty(), NULL);
  fc_assert_ret_val(0 < filename.length(), NULL);
  auto *fp = new KFilterDev(filename);
  fp->open(QIODevice::ReadOnly);
  if (!fp->isOpen()) {
    delete fp;
    return NULL;
  }
  qCDebug(inf_category) << "opened" << filename << "ok";
  inf = inf_from_stream(fp, datafn);
  inf->filename = filename;
  return inf;
}

/**
   Open the stream, and return an allocated, initialized structure.
   Returns NULL if the file could not be opened.
 */
struct inputfile *inf_from_stream(QIODevice *stream,
                                  datafilename_fn_t datafn)
{
  struct inputfile *inf;

  fc_assert_ret_val(NULL != stream, NULL);
  inf = new inputfile;
  init_zeros(inf);

  inf->filename.clear();
  inf->fp = stream;
  inf->datafn = datafn;

  qCDebug(inf_category) << "opened" << inf_filename(inf) << "ok";
  return inf;
}

/**
   Close the file and free associated memory, but don't recurse
   included_from files, and don't free the actual memory where
   the inf record is stored (ie, the memory where the users pointer
   points to).  This is used when closing an included file.
 */
static void inf_close_partial(struct inputfile *inf)
{
  fc_assert_ret(inf_sanity_check(inf));

  qCDebug(inf_category) << "sub-closing" << inf_filename(inf);

  bool error = !inf->fp->errorString().isEmpty();
  // KFilterDev returns "Unknown error" even when there's no error
  if (reinterpret_cast<KFilterDev *>(inf->fp)) {
    error = reinterpret_cast<KFilterDev *>(inf->fp)->error() != 0;
  }
  if (error) {
    qCCritical(inf_category) << "Error before closing" << inf_filename(inf)
                             << ":" << inf->fp->errorString();
  }
  delete inf->fp;
  inf->fp = nullptr;

  // assign zeros for safety if accidentally re-use etc:
  init_zeros(inf);
  inf->magic = ~INF_MAGIC;

  qCDebug(inf_category) << "sub-closed ok";
}

/**
   Close the file and free associated memory, included any partially
   recursed included files, and the memory allocated for 'inf' itself.
   Should only be used on an actually open inputfile.
   After this, the pointer should not be used.
 */
void inf_close(struct inputfile *inf)
{
  fc_assert_ret(inf_sanity_check(inf));

  qCDebug(inf_category) << "closing" << inf_filename(inf);
  if (inf->included_from) {
    inf_close(inf->included_from);
  }
  inf_close_partial(inf);
  delete inf;
  qCDebug(inf_category) << "closed ok";
}

/**
   Return TRUE if have data for current line.
 */
static bool have_line(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), false);

  return !inf->cur_line.isEmpty();
}

/**
   Return TRUE if current pos is at end of current line.
 */
static bool at_eol(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), true);
  fc_assert_ret_val(inf->cur_line_pos <= inf->cur_line.length(), true);

  return inf->cur_line_pos >= inf->cur_line.length();
}

/**
   Return TRUE if current pos is at end of file.
 */
bool inf_at_eof(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), true);

  return inf->included_from == nullptr && inf->fp->atEnd()
         && inf->cur_line_pos >= inf->cur_line.length();
}

/**
   Check for an include command, which is an isolated line with:
      *include "filename"
   If a file is included via this mechanism, returns 1, and sets up
   data appropriately: (*inf) will now correspond to the new file,
   which is opened but no data read, and inf->included_from is set
   to newly malloced memory which corresponds to the old file.
 */
static bool check_include(struct inputfile *inf)
{
  struct inputfile *new_inf, temp;

  fc_assert_ret_val(inf_sanity_check(inf), false);
  if (inf->in_string || inf->cur_line_pos > 0) {
    return false;
  }

  QString include_prefix = QStringLiteral("*include");
  if (!inf->cur_line.startsWith(include_prefix)) {
    return false;
  }

  // From here, the include-line must be well formed
  // Skip any whitespace
  for (inf->cur_line_pos = include_prefix.length();
       inf->cur_line_pos < inf->cur_line.length(); ++inf->cur_line_pos) {
    if (!inf->cur_line[inf->cur_line_pos].isSpace()) {
      break;
    }
  }

  // Check that we've got the opening ", and not EOL
  if (inf->cur_line_pos >= inf->cur_line.length()
      || inf->cur_line[inf->cur_line_pos] != '\"') {
    qCCritical(inf_category,
               "Did not find opening doublequote for '*include' line");
    return false;
  }

  // First char after the "
  auto start = inf->cur_line_pos + 1;

  // Find the closing "
  auto end = inf->cur_line.indexOf('\"', start);
  if (end < 0) {
    qCCritical(inf_category,
               "Did not find closing doublequote for '*include' line");
    return false;
  }

  auto name = inf->cur_line.mid(start, end - start);

  // Check that the rest of line is well-formed
  for (int i = end + 1; i < inf->cur_line.length(); ++i) {
    auto c = inf->cur_line[i];
    if (is_comment(c)) {
      // Ignore the rest of the line
      break;
    } else if (!c.isSpace()) {
      qCCritical(inf_category, "Junk after filename for '*include' line");
      return false;
    }
  }

  inf->cur_line_pos = inf->cur_line.length() - 1;
  auto full_name = inf->datafn(name);
  if (full_name.isEmpty()) {
    qCCritical(inf_category) << "Could not find included file» <<" << name;
    return false;
  }

  // Avoid recursion (first filename may not have the same path, but will at
  // east stop infinite recursion)
  {
    struct inputfile *inc = inf;
    do {
      if (full_name == inc->filename) {
        qCCritical(inf_category)
            << "Recursion trap on '*include' for" << full_name;
        return false;
      }
    } while ((inc = inc->included_from));
  }

  new_inf = inf_from_file(qUtf8Printable(full_name), inf->datafn);

  /* Swap things around so that memory pointed to by inf (user pointer,
     and pointer in calling functions) contains the new inputfile,
     and newly allocated memory for new_inf contains the old inputfile.
     This is pretty scary, lets hope it works...
  */
  temp = *new_inf;
  *new_inf = *inf;
  *inf = temp;
  inf->included_from = new_inf;
  return true;
}

/**
   Read a new line into cur_line.
   Increments line_num and cur_line_pos.
   Returns 0 if didn't read or other problem: treat as EOF.
   Strips newline from input.
 */
static bool read_a_line(struct inputfile *inf)
{
  fc_assert_ret_val(inf_sanity_check(inf), false);

  bool eof = inf->fp->atEnd() && inf->cur_line_pos >= inf->cur_line.length();
  if (eof && inf->included_from == nullptr) {
    qCDebug(inf_category) << "eof in:" << inf->filename;
    return false;
  }

  // Read a full line. Only ASCII line separators are valid.
  inf->cur_line = QString::fromUtf8(inf->fp->readLine());
  inf->cur_line_pos = 0;
  inf->line_num++;

  if (eof) {
    qCDebug(inf_category) << "*include end:" << inf->filename;
    // Pop the include, and get next line from file above instead.
    struct inputfile *inc = inf->included_from;
    inf_close_partial(inf);
    // So the user pointer in still valid (and inf pointers in calling
    // functions)
    *inf = std::move(*inc);
    delete inc;
    qCDebug(inf_category) << "back to:" << inf->filename;
    return read_a_line(inf);
  } else {
    // Normal behavior
    if (check_include(inf)) {
      return read_a_line(inf);
    }
    return true;
  }
}

/**
   Return a detailed log message, including information on current line
   number etc. Message can be NULL: then just logs information on where
   we are in the file.
 */
QString inf_log_str(struct inputfile *inf, const char *message, ...)
{
  fc_assert_ret_val(inf_sanity_check(inf), NULL);

  QString str;

  if (message) {
    va_list args;
    va_start(args, message);
    str = QString::vasprintf(message, args);
    va_end(args);
  }

  str += QStringLiteral("\n");
  str += QStringLiteral("  file \"%1\", line %2, pos %3")
             .arg(inf_filename(inf))
             .arg(inf->line_num)
             .arg(inf->cur_line_pos);
  if (inf_at_eof(inf)) {
    str += QStringLiteral(", EOF");
  }

  if (!inf->cur_line.isEmpty()) {
    str += QStringLiteral("\n  looking at: '%1'")
               .arg(inf->cur_line.mid(inf->cur_line_pos));
  }
  if (inf->in_string) {
    str += QStringLiteral("\n  processing string starting at line %1")
               .arg(inf->string_start_line);
  }
  while ((inf = inf->included_from)) { // local pointer assignment
    str += QStringLiteral("\n  included from file \"%1\", line %2")
               .arg(inf_filename(inf))
               .arg(inf->line_num);
  }

  return str;
}

/**
   Returns token of given type from given inputfile.
 */
QString inf_token(struct inputfile *inf, enum inf_token_type type)
{
  fc_assert_ret_val(inf_sanity_check(inf), NULL);
  fc_assert_ret_val(INF_TOK_FIRST <= type && INF_TOK_LAST > type, NULL);

  auto name = tok_tab[type].name ? tok_tab[type].name : "(unnamed)";
  auto func = tok_tab[type].func;

  QString s;
  if (func) {
    while (!have_line(inf) && read_a_line(inf)) {
      // Nothing.
    }
    if (have_line(inf)) {
      s = func(inf);
    }
  } else {
    qCCritical(inf_category)
        << "token type" << type << "(" << name << ") not supported yet";
  }
  return s;
}

/**
   Read as many tokens of specified type as possible, discarding
   the results; returns number of such tokens read and discarded.
 */
int inf_discard_tokens(struct inputfile *inf, enum inf_token_type type)
{
  int count = 0;

  while (!inf_token(inf, type).isEmpty()) {
    count++;
  }

  return count;
}

/**
   Returns section name in current position of inputfile. Returns NULL
   if there is no section name on that position. Sets inputfile position
   after section name.
 */
static QString get_token_section_name(struct inputfile *inf)
{
  fc_assert_ret_val(have_line(inf), "");

  auto start = inf->cur_line_pos;
  if (start >= inf->cur_line.length() || inf->cur_line[start] != '[') {
    return "";
  }
  ++start; // Skip the [
  auto end = inf->cur_line.indexOf(']', start);
  if (end < 0) {
    return "";
  }

  // Extract the name
  inf->token = inf->cur_line.mid(start, end - start);
  inf->cur_line_pos = end + 1;
  return inf->token;
}

/**
   Returns next entry name from inputfile. Skips white spaces and
   comments. Sets inputfile position after entry name.
 */
static QString get_token_entry_name(struct inputfile *inf)
{
  fc_assert_ret_val(have_line(inf), "");

  // Skip whitespace
  auto i = inf->cur_line_pos;
  for (; i < inf->cur_line.length(); ++i) {
    if (!inf->cur_line[i].isSpace()) {
      break;
    }
  }
  if (i >= inf->cur_line.length()) {
    return "";
  }
  auto start = i;

  // Find the end of the name
  for (; i < inf->cur_line.length(); ++i) {
    auto c = inf->cur_line[i];
    if (c.isSpace() || c == '=') {
      break;
    }
  }
  if (i >= inf->cur_line.length()) {
    return "";
  }
  auto end = i;

  // Find the equal sign
  auto eq = inf->cur_line.indexOf('=', end);
  if (eq < 0) {
    return "";
  }

  // Check that we didn't eat a comment in the middle
  auto ref = inf->cur_line.midRef(inf->cur_line_pos, eq - inf->cur_line_pos);
  if (ref.contains(';') || ref.contains('#')) {
    return "";
  }

  inf->cur_line_pos = eq + 1;
  inf->token = inf->cur_line.mid(start, end - start);

  return inf->token;
}

/**
   If inputfile is at end-of-line, frees current line, and returns " ".
   If there is still something on that line, returns "".
 */
static QString get_token_eol(struct inputfile *inf)
{
  fc_assert_ret_val(have_line(inf), "");

  if (!at_eol(inf)) {
    auto it = inf->cur_line.cbegin() + inf->cur_line_pos;
    for (; it < inf->cur_line.cend() && it->isSpace(); ++it) {
      // Skip
    }
    if (it != inf->cur_line.cend() && !is_comment(*it)) {
      return "";
    }
  }

  // finished with this line: say that we don't have it any more
  inf->cur_line.clear();
  inf->cur_line_pos = 0;

  inf->token = QStringLiteral(" ");
  return inf->token;
}

/**
   Get a flag token of a single character, with optional
   preceeding whitespace.
 */
static QString get_token_white_char(struct inputfile *inf, char target)
{
  fc_assert_ret_val(have_line(inf), NULL);

  // Skip whitespace
  auto it = inf->cur_line.cbegin() + inf->cur_line_pos;
  for (; it != inf->cur_line.cend() && it->isSpace(); ++it) {
    // Skip
  }
  if (it == inf->cur_line.cend() || *it != target) {
    return "";
  }

  inf->cur_line_pos = it - inf->cur_line.cbegin() + 1;
  inf->token = target;
  return inf->token;
}

/**
   Get flag token for table start, or NULL if that is not next token.
 */
static QString get_token_table_start(struct inputfile *inf)
{
  return get_token_white_char(inf, '{');
}

/**
   Get flag token for table end, or NULL if that is not next token.
 */
static QString get_token_table_end(struct inputfile *inf)
{
  return get_token_white_char(inf, '}');
}

/**
   Get flag token comma, or NULL if that is not next token.
 */
static QString get_token_comma(struct inputfile *inf)
{
  return get_token_white_char(inf, ',');
}

/**
   This one is more complicated; note that it may read in multiple lines.
 */
static QString get_token_value(struct inputfile *inf)
{
  fc_assert_ret_val(have_line(inf), NULL);

  auto begin = inf->cur_line.cbegin();
  auto end = inf->cur_line.cend();

  // Skip whitespace
  auto c = begin + inf->cur_line_pos;
  for (; c != end && c->isSpace(); ++c) {
    // Skip
  }
  if (c == end) {
    return "";
  }

  // Advance
  inf->cur_line_pos = c - begin;

  if (*c == '-' || *c == '+' || c->isDigit()) {
    // A number
    auto start = c++;
    for (; c != end && c->isDigit(); ++c) {
      // Take
    }
    if (*c == '.') {
      // Float maybe
      c++;
      for (; c != end && c->isDigit(); ++c) {
        // Take
      }
    }
    // check that the trailing stuff is ok
    if (!(c == end || *c == ',' || c->isSpace() || is_comment(*c))) {
      return "";
    }

    inf->token = inf->cur_line.mid(start - begin, c - start);
    inf->cur_line_pos = c - begin;

    return inf->token;
  }

  // Allow gettext marker
  bool has_i18n_marking = false;
  if (*c == '_' && *(c + 1) == '(') {
    has_i18n_marking = true;
    c += 2;
    while (c != end && c->isSpace()) {
      c++;
    }
    if (c == end) {
      return NULL;
    }
  }

  auto border_character = *c;
  if (border_character == '*') {
    // File included as string
    auto first = c - begin + 1; // Switch to indexes

    // Find the closing *
    auto last = inf->cur_line.indexOf('*', first);
    if (last < 0) {
      return "";
    }
    // Check that the trailing stuff is ok
    c += last - first + 2;
    if (!(c == end || *c == ',' || c->isSpace() || is_comment(*c))) {
      return "";
    }

    // File name without *
    auto name = inf->cur_line.mid(first, last - first);
    auto rfname = inf->datafn(name);
    if (rfname == NULL) {

      qCCritical(inf_category, _("Cannot find stringfile \"%s\"."),
                 qPrintable(name));
      return "";
    }
    auto fp = new KFilterDev(rfname);
    fp->open(QIODevice::ReadOnly);
    if (!fp->isOpen()) {
      qCCritical(inf_category, _("Cannot open stringfile \"%s\"."),
                 qPrintable(rfname));
      delete fp;
      return "";
    }
    qCDebug(inf_category) << "Stringfile" << name << "opened ok";

    inf->token = QStringLiteral("*"); // Mark as a string read from a file
    inf->token += QString::fromUtf8(fp->readAll());

    delete fp;
    fp = nullptr;

    inf->cur_line_pos = c + 1 - begin;

    return inf->token;
  } else if (border_character != '\"' && border_character != '\''
             && border_character != '$') {
    // A one-word string: maybe FALSE or TRUE.
    auto start = c;
    for (; c->isLetterOrNumber(); ++c) {
      // Skip
    }
    // check that the trailing stuff is ok:
    if (!(c == end || *c == ',' || c->isSpace() || is_comment(*c))) {
      return NULL;
    }

    inf->cur_line_pos = c - begin;
    inf->token = inf->cur_line.mid(start - begin, c - start);

    return inf->token;
  }

  /* From here, we know we have a string, we just have to find the
     trailing (un-escaped) double-quote.  We read in extra lines if
     necessary to find it.  If we _don't_ find the end-of-string
     (that is, we come to end-of-file), we return NULL, but we
     leave the file in at_eof, and don't try to back-up to the
     current point.  (That would be more difficult, and probably
     not necessary: at that point we probably have a malformed
     string/file.)

     As we read extra lines, the string value from previous
     lines is placed in partial.
  */

  // prepare for possibly multi-line string:
  inf->string_start_line = inf->line_num;
  inf->in_string = true;
  inf->partial.clear();

  auto start = c++; /* start includes the initial \", to
                     * distinguish from a number */
  for (;;) {
    while (c != end && *c != border_character) {
      /* skip over escaped chars, including backslash-doublequote,
       * and backslash-backslash: */
      if (*c == '\\' && (c + 1) != end) {
        c++;
      }
      c++;
    }

    if (*c == border_character) {
      // Found end of string
      break;
    }

    inf->partial += QStringLiteral("%1\n").arg(start);

    if (!read_a_line(inf)) {
      // shouldn't happen
      qCCritical(inf_category,
                 "Bad return for multi-line string from read_a_line");
      return "";
    }
    begin = inf->cur_line.cbegin();
    end = inf->cur_line.cend();
    c = start = begin;
  }

  // found end of string
  inf->cur_line_pos = c + 1 - begin;
  inf->token = inf->partial + inf->cur_line.mid(start - begin, c - start);

  // check gettext tag at end:
  if (has_i18n_marking) {
    if (*++c == ')') {
      inf->cur_line_pos++;
    } else {
      qCWarning(inf_category, "Missing end of i18n string marking");
    }
  }
  inf->in_string = false;
  return inf->token;
}
