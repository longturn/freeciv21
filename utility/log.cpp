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

#include <iostream>

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <vector>

// Qt
#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutexLocker>
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

Q_LOGGING_CATEGORY(assert_category, "freeciv.assert")

namespace {
static QString log_level = QStringLiteral();
static bool fatal_assertions = false;

static QBasicMutex mutex;
static void handle_message(QtMsgType type, const QMessageLogContext &context,
                           const QString &message);
static QtMessageHandler original_handler = nullptr;
static QFile *log_file = nullptr;
} // anonymous namespace

/**********************************************************************/ /**
  Parses a log level string as provided by the user on the command line, and
  installs the corresponding Qt log filters. Prints a warning and returns
  false if the log level name isn't known.
 **************************************************************************/
bool log_init(const QString &level_str)
{
  // Even if it's invalid.
  log_level = level_str;

  // Install our handler
  original_handler = qInstallMessageHandler(&handle_message);

  // Set the default format (override with QT_MESSAGE_PATTERN)
  qSetMessagePattern(
      QStringLiteral("[%{type}] %{appname} (%{file}:%{line}) - %{message}"));

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
   Prints a message, handling Freeciv-specific stuff before passing to
   the Qt handler.
 **************************************************************************/
namespace {
static void handle_message(QtMsgType type, const QMessageLogContext &context,
                           const QString &message)
{
  // Forward to file
  if (log_file != nullptr) {
    QMutexLocker lock(&mutex);
    log_file->write((message + QStringLiteral("\n")).toLocal8Bit());

    // Make sure we flush when it looks serious, maybe we'll crash soon
    if (type == QtFatalMsg || type == QtCriticalMsg) {
      log_file->flush();
    }
  }

  // Forward to the Qt handler
  if (original_handler != nullptr) {
    original_handler(type, context, message);
  }
}
} // anonymous namespace

/**********************************************************************/ /**
   Redirects the log to a file. It will still be shown on standard error.
   This function is *not* thread-safe.
 **************************************************************************/
void log_set_file(const QString &path)
{
  // Don't try to open null file names
  if (path.isEmpty()) {
    return;
  }

  // Open a new file. Note that we can't hold the mutex because QFile
  // might want to log.
  auto new_file = new QFile(path);
  if (!new_file->open(QIODevice::WriteOnly | QIODevice::Text)) {
    // Could not open the log file.
    // TRANS: %1 is an error message
    qCritical().noquote()
        << QString(_("Could not open log file for writing: %1"))
               .arg(new_file->errorString()); // FIXME translate?
    // Keep the old one if it was there
    return;
  }

  // Unset the old one
  if (log_file != nullptr) {
    delete log_file;
  }
  log_file = new_file;
}

/**********************************************************************/ /**
   Retrieves the log level passed to log_init (even if log_init failed).
   This can be overridden from the environment, so it's only useful when
   passing it to the server from the client.
 **************************************************************************/
const QString &log_get_level() { return log_level; }

/**********************************************************************/ /**
    Deinitialize logging module.
 **************************************************************************/
void log_close()
{
  QMutexLocker locker(&mutex);

  // Flush and delete log file
  if (log_file != nullptr) {
    delete log_file;
    log_file = nullptr;
  }

  // Reinstall the old handler
  qInstallMessageHandler(original_handler);
}

/**********************************************************************/ /**
   Set what signal the assert* macros should raise on failed assertion
   (-1 to disable).
 **************************************************************************/
void fc_assert_set_fatal(bool fatal) { fatal_assertions = fatal; }

/**********************************************************************/ /**
   Checks whether the fc_assert* macros should raise on failed assertion.
 **************************************************************************/
bool fc_assert_are_fatal() { return fatal_assertions; }

/**********************************************************************/ /**
   Handles a failed assertion.
 **************************************************************************/
void fc_assert_handle_failure(const char *condition, const char *file,
                              int line, const char *function,
                              const QString &message)
{
  QMessageLogger logger(file, line, assert_category().categoryName());
  logger.critical("Assertion %s failed", condition);
  if (!message.isEmpty()) {
    logger.critical().noquote() << message;
  }
  logger.critical().noquote() /* TRANS: No full stop after the URL. */
      << QString(_("Please report this message at %1")).arg(BUG_URL);
  if (fc_assert_are_fatal()) {
    logger.fatal("%s", _("Assertion failed"));
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
