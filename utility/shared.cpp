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

#include <fc_config.h>

#include <climits>
#include <clocale>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#ifdef FREECIV_MSWINDOWS
#define ALWAYS_ROOT
#include <lmcons.h> // UNLEN
#include <shlobj.h>
#include <windows.h>
#else               // FREECIV_MSWINDOWS
#include <unistd.h> // getuid, geteuid
#endif              // FREECIV_MSWINDOWS

// Qt
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QtGlobal>

// utility
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "rand.h"

#include "shared.h"

static QStringList default_data_path()
{
  // Make sure that all executables get the same directory.
  auto app_name = QCoreApplication::applicationName();
  QCoreApplication::setApplicationName("freeciv21");

  auto paths =
      QStringList{QStringLiteral("."), QStringLiteral("data"),
                  freeciv_storage_dir() + QStringLiteral("/" DATASUBDIR),
                  QStringLiteral(FREECIV_INSTALL_DATADIR)}
      + QStandardPaths::standardLocations(QStandardPaths::DataLocation);
  QCoreApplication::setApplicationName(app_name);
  return paths;
}

static QStringList default_save_path()
{
  return {QStringLiteral("."),
          freeciv_storage_dir() + QStringLiteral("/saves")};
}

static QStringList default_scenario_path()
{
  auto paths = default_data_path();
  for (auto &path : paths) {
    path += QStringLiteral("/scenarios");
  }
  return paths;
}

/* Both of these are stored in the local encoding.  The grouping_sep must
 * be converted to the internal encoding when it's used. */
static char *grouping = nullptr;
static char *grouping_sep = nullptr;

/* As well as base64 functions, this string is used for checking for
 * 'safe' filenames, so should not contain / \ . */
static const char base64url[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static QStringList data_dir_names = {};
static QStringList save_dir_names = {};
static QStringList scenario_dir_names = {};

static char *mc_group = nullptr;

Q_GLOBAL_STATIC(QString, realfile);

/**
   An AND function for fc_tristate.
 */
enum fc_tristate fc_tristate_and(enum fc_tristate one, enum fc_tristate two)
{
  if (TRI_NO == one || TRI_NO == two) {
    return TRI_NO;
  }

  if (TRI_MAYBE == one || TRI_MAYBE == two) {
    return TRI_MAYBE;
  }

  return TRI_YES;
}

/**
   Returns a statically allocated string containing a nicely-formatted
   version of the given number according to the user's locale.  (Only
   works for numbers >= zero.)  The number is given in scientific notation
   as mantissa * 10^exponent.
 */
const char *big_int_to_text(unsigned int mantissa, unsigned int exponent)
{
  static char buf[64]; // Note that we'll be filling this in right to left.
  char *grp = grouping;
  char *ptr;
  unsigned int cnt = 0;
  char sep[64];
  size_t seplen;

  /* We have to convert the encoding here (rather than when the locale
   * is initialized) because it can't be done before the charsets are
   * initialized. */
  local_to_internal_string_buffer(grouping_sep, sep, sizeof(sep));
  seplen = qstrlen(sep);

#if 0 // Not needed while the values are unsigned.
  fc_assert_ret_val(0 <= mantissa, nullptr);
  fc_assert_ret_val(0 <= exponent, nullptr);
#endif

  if (mantissa == 0) {
    return "0";
  }

  /* We fill the string in backwards, starting from the right.  So the first
   * thing we do is terminate it. */
  ptr = &buf[sizeof(buf)];
  *(--ptr) = '\0';

  while (mantissa != 0) {
    int dig;

    if (ptr <= buf + seplen) {
      // Avoid a buffer overflow.
      fc_assert_ret_val(ptr > buf + seplen, nullptr);
      return ptr;
    }

    // Add on another character.
    if (exponent > 0) {
      dig = 0;
      exponent--;
    } else {
      dig = mantissa % 10;
      mantissa /= 10;
    }
    *(--ptr) = '0' + dig;

    cnt++;
    if (mantissa != 0 && cnt == *grp) {
      /* Reached count of digits in group: insert separator and reset count.
       */
      cnt = 0;
      if (*grp == CHAR_MAX) {
        /* This test is unlikely to be necessary since we would need at
           least 421-bit ints to break the 127 digit barrier, but why not. */
        break;
      }
      ptr -= seplen;
      fc_assert_ret_val(ptr >= buf, nullptr);
      memcpy(ptr, sep, seplen);
      if (*(grp + 1) != 0) {
        // Zero means to repeat the present group-size indefinitely.
        grp++;
      }
    }
  }

  return ptr;
}

/**
   Return a prettily formatted string containing the given number.
 */
const char *int_to_text(unsigned int number)
{
  return big_int_to_text(number, 0);
}

/**
   Check whether or not the given char is a valid ascii character.  The
   character can be in any charset so long as it is a superset of ascii.
 */
static bool is_ascii(char ch)
{
  // this works with both signed and unsigned char's.
  return ch >= ' ' && ch <= '~';
}

/**
   Check if the name is safe security-wise.  This is intended to be used to
   make sure an untrusted filename is safe to be used.
 */
bool is_safe_filename(const QString &name)
{
  const QRegularExpression regex(QLatin1String("^[\\w_\\-.@]+$"));
  return regex.match(name).hasMatch()
         && !name.contains(QLatin1String(PARENT_DIR_OPERATOR));
}

/**
   This is used in sundry places to make sure that names of cities,
   players etc. do not contain yucky characters of various sorts.
   Returns TRUE iff the name is acceptable.
   FIXME:  Not internationalised.
 */
bool is_ascii_name(const char *name)
{
  const char illegal_chars[] = {'|', '%', '"', ',', '*', '<', '>', '\0'};
  int i, j;

  // must not be nullptr or empty
  if (!name || *name == '\0') {
    return false;
  }

  // must begin and end with some non-space character
  if ((*name == ' ') || (*(strchr(name, '\0') - 1) == ' ')) {
    return false;
  }

  /* must be composed entirely of printable ascii characters,
   * and no illegal characters which can break ranking scripts. */
  for (i = 0; name[i]; i++) {
    if (!is_ascii(name[i])) {
      return false;
    }
    for (j = 0; illegal_chars[j]; j++) {
      if (name[i] == illegal_chars[j]) {
        return false;
      }
    }
  }

  // otherwise, it's okay...
  return true;
}

/**
   Check for valid base64url.
 */
bool is_base64url(const char *s)
{
  size_t i = 0;

  // must not be nullptr or empty
  if (nullptr == s || '\0' == *s) {
    return false;
  }

  for (; '\0' != s[i]; i++) {
    if (nullptr == strchr(base64url, s[i])) {
      return false;
    }
  }
  return true;
}

/**
   generate a random string meeting criteria such as is_ascii_name(),
   is_base64url(), and is_safe_filename().
 */
void randomize_base64url_string(char *s, size_t n)
{
  size_t i = 0;

  // must not be nullptr or too short
  if (nullptr == s || 1 > n) {
    return;
  }

  for (; i < (n - 1); i++) {
    s[i] = base64url[fc_rand(sizeof(base64url) - 1)];
  }
  s[i] = '\0';
}

/**
   Returns 's' incremented to first non-space character.
 */
char *skip_leading_spaces(char *s)
{
  fc_assert_ret_val(nullptr != s, nullptr);

  while (*s != '\0' && QChar::isSpace(*s)) {
    s++;
  }

  return s;
}

/**
   Removes leading spaces in string pointed to by 's'.
   Note 's' must point to writeable memory!
 */
void remove_leading_spaces(char *s)
{
  char *t;

  fc_assert_ret(nullptr != s);
  t = skip_leading_spaces(s);
  if (t != s) {
    while (*t != '\0') {
      *s++ = *t++;
    }
    *s = '\0';
  }
}

/**
   Terminates string pointed to by 's' to remove traling spaces;
   Note 's' must point to writeable memory!
 */
void remove_trailing_spaces(char *s)
{
  char *t;
  size_t len;

  fc_assert_ret(nullptr != s);
  len = qstrlen(s);
  if (len > 0) {
    t = s + len - 1;
    while (QChar::isSpace(*t)) {
      *t = '\0';
      if (t == s) {
        break;
      }
      t--;
    }
  }
}

/**
   Removes leading and trailing spaces in string pointed to by 's'.
   Note 's' must point to writeable memory!
 */
void remove_leading_trailing_spaces(char *s)
{
  remove_leading_spaces(s);
  remove_trailing_spaces(s);
}

/**
   Check the length of the given string.  If the string is too long,
   log errmsg, which should be a string in printf-format taking up to
   two arguments: the string and the length.
 */
bool check_strlen(const char *str, size_t len, const char *errmsg)
{
  fc_assert_ret_val_msg(strlen(str) < len, true, errmsg, str, len);
  return false;
}

/**
   Call check_strlen() on str and then strlcpy() it into buffer.
 */
size_t loud_strlcpy(char *buffer, const char *str, size_t len,
                    const char *errmsg)
{
  (void) check_strlen(str, len, errmsg);
  return fc_strlcpy(buffer, str, len);
}

/**
   Convert 'str' to it's int reprentation if possible. 'pint' can be nullptr,
   then it will only test 'str' only contains an integer number.
 */
bool str_to_int(const char *str, int *pint)
{
  const char *start;

  fc_assert_ret_val(nullptr != str, false);

  while (QChar::isSpace(*str)) {
    // Skip leading spaces.
    str++;
  }

  start = str;
  if ('-' == *str || '+' == *str) {
    // Handle sign.
    str++;
  }
  while (QChar::isDigit(*str)) {
    // Digits.
    str++;
  }

  while (QChar::isSpace(*str)) {
    // Ignore trailing spaces.
    str++;
  }

  return ('\0' == *str
          && (nullptr == pint || 1 == sscanf(start, "%d", pint)));
}

/**
   Returns string which gives freeciv storage dir.
   Gets value once, and then caches result.
   Note the caller should not mess with the returned string.
 */
QString freeciv_storage_dir()
{
  static QString storage_dir;
  if (storage_dir.isEmpty()) {
    // Make sure that all exe get the same directory.
    auto app_name = QCoreApplication::applicationName();
    QCoreApplication::setApplicationName("freeciv21");
    storage_dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QCoreApplication::setApplicationName(app_name);

    qDebug() << _("Storage dir:") << storage_dir;
  }

  return storage_dir;
}

/**
   Returns string which gives user's username, as specified by $USER or
   as given in password file for this user's uid, or a made up name if
   we can't get either of the above.
   Gets value once, and then caches result.
   Note the caller should not mess with returned string.
 */
char *user_username(char *buf, size_t bufsz)
{
  /* This function uses a number of different methods to try to find a
   * username.  This username then has to be truncated to bufsz
   * characters (including terminator) and checked for sanity.  Note that
   * truncating a sane name can leave you with an insane name under some
   * charsets. */

  // If the environment variable $USER is present and sane, use it.
  {
    char *env = getenv("USER");

    if (env) {
      fc_strlcpy(buf, env, bufsz);
      if (is_ascii_name(buf)) {
        qDebug("USER username is %s", buf);
        return buf;
      }
    }
  }

#ifndef FREECIV_MSWINDOWS
  {
    fc_strlcpy(buf, qUtf8Printable(QDir::homePath().split("/").last()),
               bufsz);
    if (is_ascii_name(buf)) {
      qDebug("username from homepath is %s", buf);
      return buf;
    }
  }
#endif

#ifdef FREECIV_MSWINDOWS
  // On windows the GetUserName function will give us the login name.
  {
    char name[UNLEN + 1];
    DWORD length = sizeof(name);

    if (GetUserName(name, &length)) {
      fc_strlcpy(buf, name, bufsz);
      if (is_ascii_name(buf)) {
        qDebug("GetUserName username is %s", buf);
        return buf;
      }
    }
  }
#endif // FREECIV_MSWINDOWS

#ifdef ALWAYS_ROOT
  fc_strlcpy(buf, "name", bufsz);
#else
  fc_snprintf(buf, bufsz, "name%d", static_cast<int>(getuid()));
#endif
  qDebug("fake username is %s", buf);
  fc_assert(is_ascii_name(buf));
  return buf;
}

/**
   Returns a list of directory paths, in the order in which they should
   be searched.  Base function for get_data_dirs(), get_save_dirs(),
   get_scenario_dirs()
 */
static QStringList base_get_dirs(const char *env_var)
{
  if (qEnvironmentVariableIsSet(env_var)
      && qEnvironmentVariableIsEmpty(env_var)) {
    qCritical(_("\"%s\" is set but empty; using default "
                "data directories instead."),
              env_var);
    return {};
  } else if (qEnvironmentVariableIsSet(env_var)) {
    return QString(qEnvironmentVariable(env_var))
        .split(QDir::listSeparator(), Qt::SkipEmptyParts);
  } else {
    return {};
  }
}

/**
   Returns a list of data directory paths, in the order in which they should
   be searched.  These paths are specified internally or may be set as the
   environment variable $FREECIV_DATA PATH (a separated list of directories,
   where the separator itself is specified internally, platform-dependent).
   '~' at the start of a component (provided followed by '/' or '\0') is
   expanded as $HOME.

   The returned pointer is static and shouldn't be modified, nor destroyed
   by the user caller.
 */
const QStringList &get_data_dirs()
{
  /* The first time this function is called it will search and
   * allocate the directory listing.  Subsequently we will already
   * know the list and can just return it. */
  if (data_dir_names.isEmpty()) {
    data_dir_names = base_get_dirs("FREECIV_DATA_PATH");
    if (data_dir_names.isEmpty()) {
      data_dir_names = default_data_path();
      data_dir_names.removeDuplicates();
    }
    for (const auto &name : qAsConst(data_dir_names)) {
      qDebug() << "Data path component:" << name;
    }
  }

  return data_dir_names;
}

/**
   Returns a list of save directory paths, in the order in which they should
   be searched.  These paths are specified internally or may be set as the
   environment variable $FREECIV_SAVE_PATH (a separated list of directories,
   where the separator itself is specified internally, platform-dependent).
   '~' at the start of a component (provided followed by '/' or '\0') is
   expanded as $HOME.

   The returned pointer is static and shouldn't be modified, nor destroyed
   by the user caller.
 */
const QStringList &get_save_dirs()
{
  /* The first time this function is called it will search and
   * allocate the directory listing.  Subsequently we will already
   * know the list and can just return it. */
  if (save_dir_names.isEmpty()) {
    save_dir_names = base_get_dirs("FREECIV_SAVE_PATH");
    if (save_dir_names.isEmpty()) {
      save_dir_names = default_save_path();
      save_dir_names.removeDuplicates();
    }
    for (const auto &name : qAsConst(save_dir_names)) {
      qDebug() << "Save path component:" << name;
    }
  }

  return save_dir_names;
}

/**
   Returns a list of scenario directory paths, in the order in which they
   should be searched.  These paths are specified internally or may be set
   as the environment variable $FREECIV_SCENARIO_PATH (a separated list of
   directories, where the separator itself is specified internally,
   platform-dependent).
   '~' at the start of a component (provided followed
   by '/' or '\0') is expanded as $HOME.

   The returned pointer is static and shouldn't be modified, nor destroyed
   by the user caller.
 */
const QStringList &get_scenario_dirs()
{
  /* The first time this function is called it will search and
   * allocate the directory listing.  Subsequently we will already
   * know the list and can just return it. */
  if (scenario_dir_names.isEmpty()) {
    scenario_dir_names = base_get_dirs("FREECIV_SCENARIO_PATH");
    if (scenario_dir_names.isEmpty()) {
      scenario_dir_names = default_scenario_path();
      scenario_dir_names.removeDuplicates();
    }
    for (const auto &name : qAsConst(scenario_dir_names)) {
      qDebug() << "Scenario path component:" << name;
    }
  }

  return scenario_dir_names;
}

/**
   Returns a string vector storing the filenames in the data directories
   matching the given suffix.

   The list is allocated when the function is called; it should either
   be stored permanently or destroyed (with strvec_destroy()).

   The suffixes are removed from the filenames before the list is
   returned.
 */
QVector<QString> *fileinfolist(const QStringList &dirs, const char *suffix)
{
  fc_assert_ret_val(!strchr(suffix, '/'), nullptr);

  QVector<QString> *files = new QVector<QString>();
  if (dirs.isEmpty()) {
    return files;
  }

  // First assemble a full list of names.
  for (const auto &dirname : dirs) {
    QDir dir(dirname);

    if (!dir.exists()) {
      qDebug("Skipping non-existing data directory %s.",
             qUtf8Printable(dirname));
      continue;
    }

    // Get all entries in the directory matching the pattern
    dir.setNameFilters({QStringLiteral("*") + QString::fromUtf8(suffix)});
    for (auto name : dir.entryList()) {
      name.truncate(name.length() - qstrlen(suffix));
      files->append(name.toUtf8().data());
    }
  }
  std::sort(files->begin(), files->end());
  files->erase(std::unique(files->begin(), files->end()), files->end());
  return files;
}

/**
   Returns a filename to access the specified file from a
   directory by searching all specified directories for the file.

   If the specified 'filename' is empty, the returned string contains
   the effective path.  (But this should probably only be used for
   debug output.)

   Returns an empty string if the specified filename cannot be found
   in any of the data directories.
 */
QString fileinfoname(const QStringList &dirs, const QString &filename)
{
  if (dirs.isEmpty()) {
    return QString();
  }

  if (filename.isEmpty()) {
    bool first = true;

    realfile->clear();
    for (const auto &dirname : dirs) {
      if (first) {
        *realfile += QStringLiteral("/%1").arg(dirname);
        first = false;
      } else {
        *realfile += QStringLiteral("%1").arg(dirname);
      }
    }

    return *realfile;
  }

  for (const auto &dirname : dirs) {
    struct stat buf; // see if we can open the file or directory

    *realfile = QStringLiteral("%1/%2").arg(dirname, filename);
    if (fc_stat(qUtf8Printable(*realfile), &buf) == 0) {
      return *realfile;
    }
  }

  qDebug("Could not find readable file \"%s\" in data path.",
         qUtf8Printable(filename));

  return nullptr;
}

/**
 * Search for file names matching the pattern in the provided list of
 * directories. "nodups" removes duplicates.
 * The returned list will be sorted by modification time.
 */
QFileInfoList find_files_in_path(const QStringList &path,
                                 const QString &pattern, bool nodups)
{
  // First assemble a full list of files.
  auto files = QFileInfoList();
  for (const auto &dirname : path) {
    QDir dir(dirname);

    if (!dir.exists()) {
      continue;
    }

    files += dir.entryInfoList({pattern}, QDir::NoFilter);
  }

  // Sort the list by name.
  if (nodups) {
    std::sort(files.begin(), files.end(),
              [](const auto &lhs, const auto &rhs) {
                return lhs.absoluteFilePath() < rhs.absoluteFilePath();
              });
    files.erase(std::unique(files.begin(), files.end()), files.end());
  }

  // Sort the list by last modification time.
  std::sort(files.begin(), files.end(),
            [](const auto &lhs, const auto &rhs) {
              return lhs.lastModified() < rhs.lastModified();
            });

  return files;
}

/**
   Language environmental variable (with emulation).
 */
char *setup_langname()
{
  char *langname = nullptr;

#ifdef ENABLE_NLS
  langname = getenv("LANG");

#ifdef FREECIV_MSWINDOWS
  // set LANG by hand if it is not set
  if (!langname) {
    switch (PRIMARYLANGID(GetUserDefaultLangID())) {
    case LANG_ARABIC:
      langname = "ar";
      break;
    case LANG_CATALAN:
      langname = "ca";
      break;
    case LANG_CZECH:
      langname = "cs";
      break;
    case LANG_DANISH:
      langname = "da";
      break;
    case LANG_GERMAN:
      langname = "de";
      break;
    case LANG_GREEK:
      langname = "el";
      break;
    case LANG_ENGLISH:
      switch (SUBLANGID(GetUserDefaultLangID())) {
      case SUBLANG_ENGLISH_UK:
        langname = "en_GB";
        break;
      default:
        langname = "en";
        break;
      }
      break;
    case LANG_SPANISH:
      langname = "es";
      break;
    case LANG_ESTONIAN:
      langname = "et";
      break;
    case LANG_FARSI:
      langname = "fa";
      break;
    case LANG_FINNISH:
      langname = "fi";
      break;
    case LANG_FRENCH:
      langname = "fr";
      break;
    case LANG_HEBREW:
      langname = "he";
      break;
    case LANG_HUNGARIAN:
      langname = "hu";
      break;
    case LANG_ITALIAN:
      langname = "it";
      break;
    case LANG_JAPANESE:
      langname = "ja";
      break;
    case LANG_KOREAN:
      langname = "ko";
      break;
    case LANG_LITHUANIAN:
      langname = "lt";
      break;
    case LANG_DUTCH:
      langname = "nl";
      break;
    case LANG_NORWEGIAN:
      langname = "nb";
      break;
    case LANG_POLISH:
      langname = "pl";
      break;
    case LANG_PORTUGUESE:
      switch (SUBLANGID(GetUserDefaultLangID())) {
      case SUBLANG_PORTUGUESE_BRAZILIAN:
        langname = "pt_BR";
        break;
      default:
        langname = "pt";
        break;
      }
      break;
    case LANG_ROMANIAN:
      langname = "ro";
      break;
    case LANG_RUSSIAN:
      langname = "ru";
      break;
    case LANG_SWEDISH:
      langname = "sv";
      break;
    case LANG_TURKISH:
      langname = "tr";
      break;
    case LANG_UKRAINIAN:
      langname = "uk";
      break;
    case LANG_CHINESE:
      langname = "zh_CN";
      break;
    }

    if (langname != nullptr) {
      static char envstr[40];

      fc_snprintf(envstr, sizeof(envstr), "LANG=%s", langname);
      putenv(envstr);
    }
  }
#endif // FREECIV_MSWINDOWS
#endif // ENABLE_NLS

  return langname;
}

#ifdef FREECIV_ENABLE_NLS
/**
   Update autocap behavior to match current language.
 */
static void autocap_update(void)
{
  const char *autocap_opt_in[] = {"fi", nullptr};
  int i;
  bool ac_enabled = false;

  char *lang = getenv("LANG");

  if (lang != nullptr && lang[0] != '\0' && lang[1] != '\0') {
    for (i = 0; autocap_opt_in[i] != nullptr && !ac_enabled; i++) {
      if (lang[0] == autocap_opt_in[i][0]
          && lang[1] == autocap_opt_in[i][1]) {
        ac_enabled = true;
        break;
      }
    }
  }

  capitalization_opt_in(ac_enabled);
}
#endif // FREECIV_ENABLE_NLS

/**
   Setup for Native Language Support, if configured to use it.
   (Call this only once, or it may leak memory.)
 */
void init_nls()
{
  /*
   * Setup the cached locale numeric formatting information. Defaults
   * are as appropriate for the US.
   */
  grouping = fc_strdup("\3");
  grouping_sep = fc_strdup(",");

#ifdef ENABLE_NLS

#ifdef FREECIV_MSWINDOWS
  setup_langname(); // Makes sure LANG env variable has been set
#endif              // FREECIV_MSWINDOWS

  (void) setlocale(LC_ALL, "");
  (void) bindtextdomain("freeciv21-core", get_locale_dir());
  (void) textdomain("freeciv21-core");

  /* Don't touch the defaults when LC_NUMERIC == "C".
     This is intended to cater to the common case where:
       1) The user is from North America. ;-)
       2) The user has not set the proper environment variables.
          (Most applications are (unfortunately) US-centric
          by default, so why bother?)
     This would result in the "C" locale being used, with grouping ""
     and thousands_sep "", where we really want "\3" and ",". */

  if (strcmp(setlocale(LC_NUMERIC, nullptr), "C") != 0) {
    struct lconv *lc = localeconv();

    if (lc->grouping[0] == '\0') {
      // This actually indicates no grouping at all.
      char *m = new char;
      *m = CHAR_MAX;
      grouping = m;
    } else {
      size_t len;
      for (len = 0;
           lc->grouping[len] != '\0' && lc->grouping[len] != CHAR_MAX;
           len++) {
        // nothing
      }
      len++;
      delete[] grouping;
      grouping = new char[len];
      memcpy(grouping, lc->grouping, len);
    }
    delete[] grouping_sep;
    grouping_sep = fc_strdup(lc->thousands_sep);
  }

  autocap_update();

#endif // ENABLE_NLS
}

/**
   Free memory allocated by Native Language Support
 */
void free_nls()
{
  delete[] grouping;
  delete[] grouping_sep;
  grouping = nullptr;
  grouping_sep = nullptr;
}

/**
   If we have root privileges, die with an error.
   (Eg, for security reasons.)
   Param argv0 should be argv[0] or similar; fallback is
   used instead if argv0 is nullptr.
   But don't die on systems where the user is always root...
   (a general test for this would be better).
   Doesn't use log_*() because gets called before logging is setup.
 */
void dont_run_as_root(const char *argv0, const char *fallback)
{
#ifdef ALWAYS_ROOT
  return;
#else
  if (getuid() == 0 || geteuid() == 0) {
    fc_fprintf(stderr,
               _("%s: Fatal error: you're trying to run me as superuser!\n"),
               (argv0      ? argv0
                : fallback ? fallback
                           : "freeciv21"));
    fc_fprintf(stderr, _("Use a non-privileged account instead.\n"));
    exit(EXIT_FAILURE);
  }
#endif // ALWAYS_ROOT
}

/**
   Return a description string of the result.
   In English, form of description is suitable to substitute in, eg:
      prefix is <description>
   (N.B.: The description is always in English, but they have all been marked
    for translation.  If you want a localized version, use _() on the
 return.)
 */
const char *m_pre_description(enum m_pre_result result)
{
  static const char *const descriptions[] = {
      N_("exact match"), N_("only match"), N_("ambiguous"),
      N_("empty"),       N_("too long"),   N_("non-match")};
  fc_assert_ret_val(result >= 0 && result < ARRAY_SIZE(descriptions),
                    nullptr);
  return descriptions[result];
}

/**
   See match_prefix_full().
 */
enum m_pre_result match_prefix(m_pre_accessor_fn_t accessor_fn,
                               size_t n_names, size_t max_len_name,
                               m_pre_strncmp_fn_t cmp_fn,
                               m_strlen_fn_t len_fn, const char *prefix,
                               int *ind_result)
{
  return match_prefix_full(accessor_fn, n_names, max_len_name, cmp_fn,
                           len_fn, prefix, ind_result, nullptr, 0, nullptr);
}

/**
   Given n names, with maximum length max_len_name, accessed by
   accessor_fn(0) to accessor_fn(n-1), look for matching prefix
   according to given comparison function.
   Returns type of match or fail, and for return <= M_PRE_AMBIGUOUS
   sets *ind_result with matching index (or for ambiguous, first match).
   If max_len_name == 0, treat as no maximum.
   If the int array 'matches' is non-nullptr, up to 'max_matches' ambiguous
   matching names indices will be inserted into it. If 'pnum_matches' is
   non-nullptr, it will be set to the number of indices inserted into
   'matches'.
 */
enum m_pre_result match_prefix_full(m_pre_accessor_fn_t accessor_fn,
                                    size_t n_names, size_t max_len_name,
                                    m_pre_strncmp_fn_t cmp_fn,
                                    m_strlen_fn_t len_fn, const char *prefix,
                                    int *ind_result, int *matches,
                                    int max_matches, int *pnum_matches)
{
  int i, len, nmatches;

  if (len_fn == nullptr) {
    len = qstrlen(prefix);
  } else {
    len = len_fn(prefix);
  }
  if (len == 0) {
    return M_PRE_EMPTY;
  }
  if (len > max_len_name && max_len_name > 0) {
    return M_PRE_LONG;
  }

  nmatches = 0;
  for (i = 0; i < n_names; i++) {
    const char *name = accessor_fn(i);

    if (cmp_fn(name, prefix, len) == 0) {
      if (strlen(name) == len) {
        *ind_result = i;
        return M_PRE_EXACT;
      }
      if (nmatches == 0) {
        *ind_result = i; // first match
      }
      if (matches != nullptr && nmatches < max_matches) {
        matches[nmatches] = i;
      }
      nmatches++;
    }
  }

  if (nmatches == 1) {
    return M_PRE_ONLY;
  } else if (nmatches > 1) {
    if (pnum_matches != nullptr) {
      *pnum_matches = MIN(max_matches, nmatches);
    }
    return M_PRE_AMBIGUOUS;
  } else {
    return M_PRE_FAIL;
  }
}

/**
   Returns string which gives the multicast group IP address for finding
   servers on the LAN, as specified by $FREECIV_MULTICAST_GROUP.
   Gets value once, and then caches result.
 */
char *get_multicast_group(bool ipv6_preferred)
{
  static const char *default_multicast_group_ipv4 = "225.1.1.1";
  // TODO: Get useful group (this is node local)
  static const char *default_multicast_group_ipv6 = "FF31::8000:15B4";

  if (mc_group == nullptr) {
    char *env = getenv("FREECIV_MULTICAST_GROUP");

    if (env) {
      mc_group = fc_strdup(env);
    } else {
      if (ipv6_preferred) {
        mc_group = fc_strdup(default_multicast_group_ipv6);
      } else {
        mc_group = fc_strdup(default_multicast_group_ipv4);
      }
    }
  }

  return mc_group;
}

/**
   Free multicast group resources
 */
void free_multicast_group()
{
  delete[] mc_group;
  mc_group = nullptr;
}

/**
   Interpret ~/ in filename as home dir
   New path is returned in buf of size buf_size

   This may fail if the path is too long.  It is better to use
   interpret_tilde_alloc.
 */
void interpret_tilde(char *buf, size_t buf_size, const QString &filename)
{
  if (filename.startsWith(QLatin1String("~/"))) {
    fc_snprintf(buf, buf_size, "%s/%s", qUtf8Printable(QDir::homePath()),
                qUtf8Printable(filename.right(filename.length() - 2)));
  } else if (filename == QLatin1String("~")) {
    qstrncpy(buf, qUtf8Printable(QDir::homePath()), buf_size);
  } else {
    qstrncpy(buf, qUtf8Printable(filename), buf_size);
  }
}

/**
   Interpret ~/ in filename as home dir

   The new path is returned in buf, as a newly allocated buffer.  The new
   path will always be allocated and written, even if there is no ~ present.
 */
char *interpret_tilde_alloc(const char *filename)
{
  if (filename[0] == '~' && filename[1] == '/') {
    QString home = QDir::homePath();
    size_t sz;
    char *buf;

    filename += 2; /* Skip past "~/" */
    sz = home.length() + qstrlen(filename) + 2;
    buf = static_cast<char *>(fc_malloc(sz));
    fc_snprintf(buf, sz, "%s/%s", qUtf8Printable(home), filename);
    return buf;
  } else if (filename[0] == '~' && filename[1] == '\0') {
    return fc_strdup(qUtf8Printable(QDir::homePath()));
  } else {
    return fc_strdup(filename);
  }
}

/**
 * Interpret ~ in filename as home dir
 */
QString interpret_tilde(const QString &filename)
{
  if (filename == QLatin1String("~")) {
    return QDir::homePath();
  } else if (filename.startsWith(QLatin1String("~/"))) {
    return QDir::homePath() + filename.midRef(1);
  } else {
    return filename;
  }
}

/**
   If the directory "pathname" does not exist, recursively create all
   directories until it does.
 */
bool make_dir(const QString &pathname)
{
  // We can always create a directory with an empty name -- it's the current
  // folder.
  return pathname.isEmpty() || QDir().mkpath(interpret_tilde(pathname));
}

/**
   Scan in a word or set of words from start to but not including
   any of the given delimiters. The buf pointer will point past delimiter,
   or be set to nullptr if there is nothing there. Removes excess white
   space.

   This function should be safe, and dest will contain "\0" and
   *buf == nullptr on failure. We always fail gently.

   Due to the way the scanning is performed, looking for a space
   will give you the first word even if it comes before multiple
   spaces.

   Returns delimiter found.

   Pass in nullptr for dest and -1 for size to just skip ahead.  Note that if
   nothing is found, dest will contain the whole string, minus leading and
   trailing whitespace.  You can scan for "" to conveniently grab the
   remainder of a string.
 */
char scanin(char **buf, char *delimiters, char *dest, int size)
{
  char *ptr, found = '?';

  if (*buf == nullptr || qstrlen(*buf) == 0 || size == 0) {
    if (dest) {
      dest[0] = '\0';
    }
    *buf = nullptr;
    return '\0';
  }

  if (dest) {
    qstrncpy(dest, *buf, size - 1);
    dest[size - 1] = '\0';
    remove_leading_trailing_spaces(dest);
    ptr = strpbrk(dest, delimiters);
  } else {
    // Just skip ahead.
    ptr = strpbrk(*buf, delimiters);
  }
  if (ptr != nullptr) {
    found = *ptr;
    if (dest) {
      *ptr = '\0';
    }
    if (dest) {
      remove_leading_trailing_spaces(dest);
    }
    *buf = strpbrk(*buf, delimiters);
    if (*buf != nullptr) {
      (*buf)++; // skip delimiter
    } else {
    }
  } else {
    *buf = nullptr;
  }

  return found;
}

/**
   Convenience function to nicely format a time_t seconds value in to a
   string with hours, minutes, etc.
 */
void format_time_duration(time_t t, char *buf, int maxlen)
{
  int seconds, minutes, hours, days;
  bool space = false;

  seconds = t % 60;
  minutes = (t / 60) % 60;
  hours = (t / (60 * 60)) % 24;
  days = t / (60 * 60 * 24);

  if (maxlen <= 0) {
    return;
  }

  buf[0] = '\0';

  if (days > 0) {
    cat_snprintf(buf, maxlen, "%d %s", days, PL_("day", "days", days));
    space = true;
  }
  if (hours > 0) {
    cat_snprintf(buf, maxlen, "%s%d %s", space ? " " : "", hours,
                 PL_("hour", "hours", hours));
    space = true;
  }
  if (minutes > 0) {
    cat_snprintf(buf, maxlen, "%s%d %s", space ? " " : "", minutes,
                 PL_("minute", "minutes", minutes));
    space = true;
  }
  if (seconds > 0) {
    cat_snprintf(buf, maxlen, "%s%d %s", space ? " " : "", seconds,
                 PL_("second", "seconds", seconds));
  }
}

/**
   Randomize the elements of an array using the Fisher-Yates shuffle.

   see: http://benpfaff.org/writings/clc/shuffle.html
 */
void array_shuffle(int *array, int n)
{
  if (n > 1 && array != nullptr) {
    int i, j, t;
    for (i = 0; i < n - 1; i++) {
      j = i + fc_rand(n - i);
      t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

/**
   Test an asterisk in the pattern against test. Returns TRUE if test fit the
   pattern. May be recursive, as it calls wildcard_fit_string() itself (if
   many asterisks).
 */
static bool wildcard_asterisk_fit(const char *pattern, const char *test)
{
  char jump_to;

  // Jump over the leading asterisks.
  pattern++;
  while (true) {
    switch (*pattern) {
    case '\0':
      // It is a leading asterisk.
      return true;
    case '*':
      pattern++;
      continue;
    case '?':
      if ('\0' == *test) {
        return false;
      }
      test++;
      pattern++;
      continue;
    }

    break;
  }

  if ('[' != *pattern) {
    if ('\\' == *pattern) {
      jump_to = *(pattern + 1);
    } else {
      jump_to = *pattern;
    }
  } else {
    jump_to = '\0';
  }

  while ('\0' != *test) {
    if ('\0' != jump_to) {
      // Jump to next matching charather.
      test = strchr(test, jump_to);
      if (nullptr == test) {
        // No match.
        return false;
      }
    }

    if (wildcard_fit_string(pattern, test)) {
      return true;
    }

    (test)++;
  }

  return false;
}

/**
   Test a range in the pattern against test. Returns TRUE if **test fit the
   first range in *pattern.
 */
static bool wildcard_range_fit(const char **pattern, const char **test)
{
  const char *start = (*pattern + 1);
  char testc;
  bool negation;

  if ('\0' == **test) {
    // Need one character.
    return false;
  }

  // Find the end of the pattern.
  while (true) {
    *pattern = strchr(*pattern, ']');
    if (nullptr == *pattern) {
      // Wildcard format error.
      return false;
    } else if (*(*pattern - 1) != '\\') {
      // This is the end.
      break;
    } else {
      // Try again.
      (*pattern)++;
    }
  }

  if ('!' == *start) {
    negation = true;
    start++;
  } else {
    negation = false;
  }
  testc = **test;
  (*test)++;
  (*pattern)++;

  for (; start < *pattern; start++) {
    if ('-' == *start || '!' == *start) {
      // Wildcard format error.
      return false;
    } else if (start < *pattern - 2 && '-' == *(start + 1)) {
      // Case range.
      if (*start <= testc && testc <= *(start + 2)) {
        return !negation;
      }
      start += 2;
    } else if (*start == testc) {
      // Single character.
      return !negation;
    }
  }

  return negation;
}

/**
   Returns TRUE if test fit the pattern. The pattern can contain special
   characters:
   * '*': to specify a substitute for any zero or more characters.
   * '?': to specify a substitute for any one character.
   * '[...]': to specify a range of characters:
     * '!': at the begenning of the range means that the matching result
       will be inverted
     * 'A-Z': means any character between 'A' and 'Z'.
     * 'agr': means 'a', 'g' or 'r'.
 */
bool wildcard_fit_string(const char *pattern, const char *test)
{
  while (true) {
    switch (*pattern) {
    case '\0':
      // '/* '\0' != test. */' != test.
      return '\0' == *test;
    case '*':
      return wildcard_asterisk_fit(pattern, test); // Maybe recursive.
    case '[':
      if (!wildcard_range_fit(&pattern, &test)) {
        return false;
      }
      continue;
    case '?':
      if ('\0' == *test) {
        return false;
      }
      break;
    case '\\':
      pattern++;
      fc__fallthrough; // No break
    default:
      if (*pattern != *test) {
        return false;
      }
      break;
    }
    pattern++;
    test++;
  }

  return false;
}

/**
   Print a string with a custom format. sequences is a pointer to an array of
   sequences, probably defined with CF_*_SEQ(). sequences_num is the number
 of the sequences, or -1 in the case the array is terminated with CF_END.

   Example:
   static const struct cf_sequence sequences[] = {
     CF_INT_SEQ('y', 2010)
   };
   char buf[256];

   fc_vsnprintcf(buf, sizeof(buf), "%y %+06y", sequences, 1);
   // This will print "2010 +02010" into buf.
 */
int fc_vsnprintcf(char *buf, size_t buf_len, const char *format,
                  const struct cf_sequence *sequences, size_t sequences_num)
{
  const struct cf_sequence *pseq;
  char cformat[32];
  const char *f = format;
  char *const max = buf + buf_len - 1;
  char *b = buf, *c;
  const char *const cmax = cformat + sizeof(cformat) - 2;
  int i, j;

  if (static_cast<size_t>(-1) == sequences_num) {
    // Find the number of sequences.
    sequences_num = 0;
    for (pseq = sequences; CF_LAST != pseq->type; pseq++) {
      sequences_num++;
    }
  }

  while ('\0' != *f) {
    if ('%' == *f) {
      // Sequence.

      f++;
      if ('%' == *f) {
        // Double '%'.
        *b++ = '%';
        f++;
        continue;
      }

      // Make format.
      c = cformat;
      *c++ = '%';
      for (; !QChar::isLetter(*f) && '\0' != *f && '%' != *f && cmax > c;
           f++) {
        *c++ = *f;
      }

      if (!QChar::isLetter(*f)) {
        /* Beginning of a new sequence, end of the format, or too long
         * sequence. */
        *c = '\0';
        j = fc_snprintf(b, max - b + 1, "%s", cformat);
        if (-1 == j) {
          return -1;
        }
        b += j;
        continue;
      }

      for (i = 0, pseq = sequences; i < sequences_num; i++, pseq++) {
        if (pseq->letter == *f) {
          j = -2;
          switch (pseq->type) {
          case CF_BOOLEAN:
            *c++ = 's';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat,
                            pseq->bool_value ? "TRUE" : "FALSE");
            break;
          case CF_TRANS_BOOLEAN:
            *c++ = 's';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat,
                            pseq->bool_value ? _("TRUE") : _("FALSE"));
            break;
          case CF_CHARACTER:
            *c++ = 'c';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat, pseq->char_value);
            break;
          case CF_INTEGER:
            *c++ = 'd';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat, pseq->int_value);
            break;
          case CF_HEXA:
            *c++ = 'x';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat, pseq->int_value);
            break;
          case CF_FLOAT:
            *c++ = 'f';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat, pseq->float_value);
            break;
          case CF_POINTER:
            *c++ = 'p';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat, pseq->ptr_value);
            break;
          case CF_STRING:
            *c++ = 's';
            *c = '\0';
            j = fc_snprintf(b, max - b + 1, cformat, pseq->str_value);
            break;
          case CF_LAST:
            break;
          };
          if (-2 == j) {
            qCritical("Error: unsupported sequence type: %d.", pseq->type);
            break;
          }
          if (-1 == j) {
            // Full!
            return -1;
          }
          f++;
          b += j;
          break;
        }
      }
      if (i >= sequences_num) {
        // Format not supported.
        *c = '\0';
        j = fc_snprintf(b, max - b + 1, "%s%c", cformat, *f);
        if (-1 == j) {
          return -1;
        }
        f++;
        b += j;
      }
    } else {
      // Not a sequence.
      *b++ = *f++;
    }
    if (max <= b) {
      // Too long.
      *max = '\0';
      return -1;
    }
  }
  *b = '\0';
  return b - buf;
}

/**
   Extract the sequences of a format. Returns the number of extracted
   escapes.
 */
static size_t extract_escapes(const char *format, char *escapes,
                              size_t max_escapes)
{
  static const char format_escapes[] = {'*', 'd', 'i', 'o', 'u', 'x', 'X',
                                        'e', 'E', 'f', 'F', 'g', 'G', 'a',
                                        'A', 'c', 's', 'p', 'n', '\0'};
  bool reordered = false;
  size_t num = 0;
  int idx = 0;

  memset(escapes, 0, max_escapes);
  format = strchr(format, '%');
  while (nullptr != format) {
    format++;
    if ('%' == *format) {
      // Double, not a sequence.
      continue;
    } else if (QChar::isDigit(*format)) {
      const char *start = format;

      do {
        format++;
      } while (QChar::isDigit(*format));
      if ('$' == *format) {
        // Strings are reordered.
        if (1 != sscanf(start, "%d", &idx)) {
          reordered = true;
        } else {
          fc_assert_msg(false, "Invalid format string \"%s\"", format);
        }
      }
    }

    while ('\0' != *format && nullptr == strchr(format_escapes, *format)) {
      format++;
    }
    escapes[idx] = *format;

    // Increase the read count.
    if (reordered) {
      if (idx > num) {
        num = idx;
      }
    } else {
      idx++;
      num++;
    }

    if ('*' != *format) {
      format = strchr(format, '%');
    } // else we didn't have found the real sequence.
  }
  return num;
}

/**
   Returns TRUE iff both formats are compatible (if 'format1' can be used
   instead 'format2' and reciprocally).
 */
bool formats_match(const char *format1, const char *format2)
{
  char format1_escapes[256], format2_escapes[256];
  size_t format1_escapes_num =
      extract_escapes(format1, format1_escapes, sizeof(format1_escapes));
  size_t format2_escapes_num =
      extract_escapes(format2, format2_escapes, sizeof(format2_escapes));

  return (
      format1_escapes_num == format2_escapes_num
      && 0 == memcmp(format1_escapes, format2_escapes, format1_escapes_num));
}
