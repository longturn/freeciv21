/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <vector>

// Qt
#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "fcthread.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "deprecations.h"
#include "log.h"

#define MAX_LEN_LOG_LINE 5120

static void log_write(FILE *fs, QtMsgType level, bool print_from_where,
                      const char *where, const char *message);
static void log_real(QtMsgType level, bool print_from_where,
                     const char *where, const char *msg);

static char *log_filename = NULL;
static log_pre_callback_fn log_pre_callback = log_real;
static log_callback_fn log_callback = NULL;
static log_prefix_fn log_prefix = NULL;

static fc_mutex logfile_mutex;

#ifdef FREECIV_DEBUG
static const QtMsgType max_level = LOG_DEBUG;
#else
static const QtMsgType max_level = LOG_VERBOSE;
#endif /* FREECIV_DEBUG */

static QtMsgType fc_QtMsgType = LOG_NORMAL;
static int fc_fatal_assertions = -1;

#ifdef FREECIV_DEBUG
struct log_fileinfo {
  char *name;
  QtMsgType level;
  unsigned int min;
  unsigned int max;
};
static std::vector<log_fileinfo> log_files;
#endif /* FREECIV_DEBUG */

static const char *QtMsgType_names[] = {
    "Fatal", "Error", "Warning", "Normal", "Verbose", "Debug", NULL};

/* A helper variable to indicate that there is no log message. The '%s' is
 * added to use it as format string as well as the argument. */
const char *nologmsg = "nologmsg:%s";

/**********************************************************************/ /**
  Parses a log level string as provided by the user on the command line, and
  installs the corresponding Qt log filters. Prints a warning and returns
  false if the log level name isn't known.
 **************************************************************************/
bool log_parse_level_str(const QString &level_str)
{
  // Create default filter rules to pass to Qt. We do it this way so the user
  // can override our simplistic rules with environment variables.
  if (level_str == QStringLiteral("fatal")) {
    // Level "fatal" cannot be disabled, so we omit it below.
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = false\n"
                                                    "*.warning = false\n"
                                                    "*.info = false\n"
                                                    "*.debug = false\n"));
    return true;
  } else if (level_str == QStringLiteral("critical")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = false\n"
                                                    "*.info = false\n"
                                                    "*.debug = false\n"));
    return true;
  } else if (level_str == QStringLiteral("warning")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = true\n"
                                                    "*.info = false\n"
                                                    "*.debug = false\n"));
    return true;
  } else if (level_str == QStringLiteral("info")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = true\n"
                                                    "*.info = true\n"
                                                    "*.debug = false\n"
                                                    "qt.*.info = false\n"));
    return true;
  } else if (level_str == QStringLiteral("debug")) {
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical = true\n"
                                                    "*.warning = true\n"
                                                    "*.info = true\n"
                                                    "*.debug = true\n"
                                                    "qt.*.info = false\n"
                                                    "qt.*.debug = false\n"));
    return true;
  } else {
    // Not a known name
    // TRANS: Do not translate "fatal", "critical", "warning", "info" or
    //        "debug". It's exactly what the user must type.
    qCritical(_("\"%s\" is not a valid log level name (valid names are "
                "fatal/critical/warning/info/debug)"),
              qPrintable(level_str));
    return false;
  }
}

/**********************************************************************/ /**
   Initialise the log module. Either 'filename' or 'callback' may be NULL.
   If both are NULL, print to stderr. If both are non-NULL, both callback,
   and fprintf to file.  Pass -1 for fatal_assertions to don't raise any
   signal on failed assertion.
 **************************************************************************/
void log_init(const char *filename, log_callback_fn callback,
              log_prefix_fn prefix, int fatal_assertions)
{
  if (log_filename) {
    free(log_filename);
    log_filename = NULL;
  }
  if (filename && strlen(filename) > 0) {
    log_filename = fc_strdup(filename);
  } else {
    log_filename = NULL;
  }
  log_callback = callback;
  log_prefix = prefix;
  fc_fatal_assertions = fatal_assertions;
  fc_init_mutex(&logfile_mutex);
  qDebug("log started");
  log_debug("LOG_DEBUG test");
}

/**********************************************************************/ /**
    Deinitialize logging module.
 **************************************************************************/
void log_close(void) { fc_destroy_mutex(&logfile_mutex); }

/**********************************************************************/ /**
   Adjust the log preparation callback function.
 **************************************************************************/
log_pre_callback_fn log_set_pre_callback(log_pre_callback_fn precallback)
{
  log_pre_callback_fn old = log_pre_callback;

  log_pre_callback = precallback;

  return old;
}

/**********************************************************************/ /**
   Adjust the callback function after initial log_init().
 **************************************************************************/
log_callback_fn log_set_callback(log_callback_fn callback)
{
  log_callback_fn old = log_callback;

  log_callback = callback;

  return old;
}

/**********************************************************************/ /**
   Adjust the prefix callback function after initial log_init().
 **************************************************************************/
log_prefix_fn log_set_prefix(log_prefix_fn prefix)
{
  log_prefix_fn old = log_prefix;

  log_prefix = prefix;

  return old;
}

/**********************************************************************/ /**
   Adjust the logging level after initial log_init().
 **************************************************************************/
void log_set_level(QtMsgType level) { fc_QtMsgType = level; }

/**********************************************************************/ /**
   Returns the current log level.
 **************************************************************************/
QtMsgType log_get_level(void) { return fc_QtMsgType; }

#ifdef FREECIV_DEBUG
/**********************************************************************/ /**
   Returns wether we should do an output for this level, in this file,
   at this line.
 **************************************************************************/
bool log_do_output_for_level_at_location(QtMsgType level, const char *file,
                                         int line)
{
  auto name = QFileInfo(file).fileName();
  for (const auto &pfile : log_files) {
    if (pfile.level >= level && name == pfile.name
        && ((0 == pfile.min && 0 == pfile.max)
            || (pfile.min <= line && pfile.max >= line))) {
      return TRUE;
    }
  }
  return (fc_QtMsgType >= level);
}
#endif /* FREECIV_DEBUG */

/**********************************************************************/ /**
   Unconditionally print a simple string.
   Let the callback do its own level formatting and add a '\n' if it wants.
 **************************************************************************/
static void log_write(FILE *fs, QtMsgType level, bool print_from_where,
                      const char *where, const char *message)
{
  if (log_filename || (!log_callback)) {
    char prefix[128];

    if (log_prefix) {
      /* Get the log prefix. */
      fc_snprintf(prefix, sizeof(prefix), "[%s] ", log_prefix());
    } else {
      prefix[0] = '\0';
    }

    if (log_filename || (print_from_where && where)) {
      fc_fprintf(fs, "%d: %s%s%s\n", level, prefix, where, message);
    } else {
      fc_fprintf(fs, "%d: %s%s\n", level, prefix, message);
    }
    fflush(fs);
  }

  if (log_callback) {
    if (print_from_where) {
      char buf[MAX_LEN_LOG_LINE];

      fc_snprintf(buf, sizeof(buf), "%s%s", where, message);
      log_callback(level, buf, log_filename != NULL);
    } else {
      log_callback(level, message, log_filename != NULL);
    }
  }
}

/**********************************************************************/ /**
   Unconditionally print a log message. This function is usually protected
   by do_log_for().
 **************************************************************************/
void vdo_log(const char *file, const char *function, int line,
             bool print_from_where, QtMsgType level, char *buf, int buflen,
             const char *message, va_list args)
{
  char buf_where[MAX_LEN_LOG_LINE];

  /* There used to be check against recursive logging here, but
   * the way it worked prevented any kind of simultaneous logging,
   * not just recursive. Multiple threads should be able to log
   * simultaneously. */

  fc_vsnprintf(buf, buflen, message, args);
  fc_snprintf(buf_where, sizeof(buf_where), "in %s() [%s::%d]: ", function,
              file, line);

  /* In the default configuration log_pre_callback is equal to log_real(). */
  if (log_pre_callback) {
    log_pre_callback(level, print_from_where, buf_where, buf);
  }
}

/**********************************************************************/ /**
   Really print a log message.
   For repeat message, may wait and print instead "last message repeated ..."
   at some later time.
   Calls log_callback if non-null, else prints to stderr.
 **************************************************************************/
static void log_real(QtMsgType level, bool print_from_where,
                     const char *where, const char *msg)
{
  static char last_msg[MAX_LEN_LOG_LINE] = "";
  static unsigned int repeated =
      0;                        /* total times current message repeated */
  static unsigned int next = 2; /* next total to print update */
  static unsigned int prev = 0; /* total on last update */
  /* only count as repeat if same level */
  static QtMsgType prev_level = static_cast<QtMsgType>(-1);
  char buf[MAX_LEN_LOG_LINE];
  FILE *fs;

  if (log_filename) {
    fc_allocate_mutex(&logfile_mutex);
    if (!(fs = fc_fopen(log_filename, "a"))) {
      fc_fprintf(stderr,
                 _("Couldn't open logfile: %s for appending \"%s\".\n"),
                 log_filename, msg);
      exit(EXIT_FAILURE);
    }
  } else {
    fs = stderr;
  }

  if (level == prev_level
      && 0 == strncmp(msg, last_msg, MAX_LEN_LOG_LINE - 1)) {
    repeated++;
    if (repeated == next) {
      fc_snprintf(buf, sizeof(buf),
                  PL_("last message repeated %d time",
                      "last message repeated %d times", repeated - prev),
                  repeated - prev);
      if (repeated > 2) {
        cat_snprintf(
            buf, sizeof(buf),
            /* TRANS: preserve leading space */
            PL_(" (total %d repeat)", " (total %d repeats)", repeated),
            repeated);
      }
      log_write(fs, prev_level, print_from_where, where, buf);
      prev = repeated;
      next *= 2;
    }
  } else {
    if (repeated > 0 && repeated != prev) {
      if (repeated == 1) {
        /* just repeat the previous message: */
        log_write(fs, prev_level, print_from_where, where, last_msg);
      } else {
        fc_snprintf(buf, sizeof(buf),
                    PL_("last message repeated %d time",
                        "last message repeated %d times", repeated - prev),
                    repeated - prev);
        if (repeated > 2) {
          cat_snprintf(
              buf, sizeof(buf),
              PL_(" (total %d repeat)", " (total %d repeats)", repeated),
              repeated);
        }
        log_write(fs, prev_level, print_from_where, where, buf);
      }
    }
    prev_level = level;
    repeated = 0;
    next = 2;
    prev = 0;
    log_write(fs, level, print_from_where, where, msg);
  }
  /* Save last message. */
  sz_strlcpy(last_msg, msg);

  fflush(fs);
  if (log_filename) {
    fclose(fs);
    fc_release_mutex(&logfile_mutex);
  }
}

/**********************************************************************/ /**
   Unconditionally print a log message. This function is usually protected
   by do_log_for().
   For repeat message, may wait and print instead
   "last message repeated ..." at some later time.
   Calls log_callback if non-null, else prints to stderr.
 **************************************************************************/
void do_log(const char *file, const char *function, int line,
            bool print_from_where, QtMsgType level, const char *message, ...)
{
  char buf[MAX_LEN_LOG_LINE];
  va_list args;

  va_start(args, message);
  vdo_log(file, function, line, print_from_where, level, buf,
          MAX_LEN_LOG_LINE, message, args);
  va_end(args);
}

/**********************************************************************/ /**
   Set what signal the fc_assert* macros should raise on failed assertion
   (-1 to disable).
 **************************************************************************/
void fc_assert_set_fatal(int fatal_assertions)
{
  fc_fatal_assertions = fatal_assertions;
}

/**********************************************************************/ /**
   Returns wether the fc_assert* macros should raise a signal on failed
   assertion.
 **************************************************************************/
void fc_assert_fail(const char *file, const char *function, int line,
                    const char *assertion, const char *message, ...)
{
  QtMsgType level = (0 <= fc_fatal_assertions ? LOG_FATAL : LOG_ERROR);

  if (NULL != assertion) {
    do_log(file, function, line, TRUE, level, "assertion '%s' failed.",
           assertion);
  }

  if (NULL != message && NOLOGMSG != message) {
    /* Additional message. */
    char buf[MAX_LEN_LOG_LINE];
    va_list args;

    va_start(args, message);
    vdo_log(file, function, line, FALSE, level, buf, MAX_LEN_LOG_LINE,
            message, args);
    va_end(args);
  }

  do_log(file, function, line, FALSE, level,
         /* TRANS: No full stop after the URL, could cause confusion. */
         _("Please report this message at %s"), BUG_URL);

  if (0 <= fc_fatal_assertions) {
    /* Emit a signal. */
    raise(fc_fatal_assertions);
  }
}

void log_time(QString msg, bool log)
{
  static bool logging;
  if (log) {
    logging = true;
  }
  if (logging) {
    qInfo() << qPrintable(msg);
  }
}
