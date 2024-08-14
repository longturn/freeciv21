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

#include <vector>

// Qt
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QString>

// Windows
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// utility
#include "fcintl.h"
#include "shared.h"

#include "log.h"

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

/**
  Parses a log level string as provided by the user on the command line, and
  installs the corresponding Qt log filters. Prints a warning and returns
  false if the log level name isn't known.

  Additional logging rules can be passed in Qt format.
 */
bool log_init(const QString &level_str, const QStringList &extra_rules)
{
  // Even if it's invalid.
  log_level = level_str;

  // Install our handler
  original_handler = qInstallMessageHandler(&handle_message);

#ifdef Q_OS_WIN
  {
    // Enable VT-100 mode in cmd.exe
    auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle != INVALID_HANDLE_VALUE) {
      DWORD mode = 0;
      if (GetConsoleMode(handle, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(handle, mode);
      }
    }
  }
#endif

  // Set the default format (override with QT_MESSAGE_PATTERN)
#ifdef QT_DEBUG
  // In debug builds, we have the source location
  qSetMessagePattern(
      QStringLiteral("[%{type}] %{appname} (%{file}:%{line}) - %{message}"));
#else
  // Not a debug build, the function name will not be known
  qSetMessagePattern(QStringLiteral("[%{type}] %{appname} - %{message}"));
#endif

  // Create default filter rules to pass to Qt. We do it this way so the user
  // can override our simplistic rules with environment variables.
  auto rules = QStringList();
  if (level_str == QStringLiteral("fatal")) {
    // Level "fatal" cannot be disabled, so we omit it below.
    rules += {
        QStringLiteral("*.critical = false"),
        QStringLiteral("*.warning = false"),
        QStringLiteral("*.info = false"),
        QStringLiteral("*.debug = false"),
    };
  } else if (level_str == QStringLiteral("critical")) {
    rules += {
        QStringLiteral("*.critical = true"),
        QStringLiteral("*.warning = false"),
        QStringLiteral("*.info = false"),
        QStringLiteral("*.debug = false"),
    };
  } else if (level_str == QStringLiteral("warning")) {
    rules += {
        QStringLiteral("*.critical = true"),
        QStringLiteral("*.warning = true"),
        QStringLiteral("*.info = false"),
        QStringLiteral("*.debug = false"),
    };
  } else if (level_str == QStringLiteral("info")) {
    rules += {
        QStringLiteral("*.critical = true"),
        QStringLiteral("*.warning = true"),
        QStringLiteral("*.info = true"),
        QStringLiteral("*.debug = false"),
        QStringLiteral("qt.*.info = false"),
    };
  } else if (level_str == QStringLiteral("debug")) {
    rules += {
        QStringLiteral("*.critical = true"),
        QStringLiteral("*.warning = true"),
        QStringLiteral("*.info = true"),
        QStringLiteral("*.debug = true"),
        QStringLiteral("qt.*.info = false"),
        QStringLiteral("qt.*.debug = false"),
    };
  } else {
    // Not a known name
    // TRANS: Do not translate "fatal", "critical", "warning", "info" or
    //        "debug". It's exactly what the user must type.
    qCritical(_("\"%s\" is not a valid log level name (valid names are "
                "fatal/critical/warning/info/debug)"),
              qUtf8Printable(level_str));
    return false;
  }

  rules += extra_rules;
  QLoggingCategory::setFilterRules(rules.join('\n'));

  qDebug() << "Applied logging rules" << rules;

  return true;
}

/**
   Prints a message, handling Freeciv-specific stuff before passing to
   the Qt handler.
 */
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

/**
   Redirects the log to a file. It will still be shown on standard error.
   This function is *not* thread-safe.
 */
void log_set_file(const QString &path)
{
  // Don't try to open null file names
  if (path.isEmpty()) {
    return;
  }

  // Open a new file. Note that we can't hold the mutex because QFile
  // might want to log.
  auto *new_file = new QFile(path);
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
  delete log_file;
  log_file = new_file;
}

/**
   Retrieves the log level passed to log_init (even if log_init failed).
   This can be overridden from the environment, so it's only useful when
   passing it to the server from the client.
 */
const QString &log_get_level() { return log_level; }

/**
    Deinitialize logging module.
 */
void log_close()
{
  QMutexLocker locker(&mutex);

  delete log_file;
  log_file = nullptr;
  // Reinstall the old handler
  qInstallMessageHandler(original_handler);
}

/**
   Set what signal the assert* macros should raise on failed assertion
   (-1 to disable).
 */
void fc_assert_set_fatal(bool fatal) { fatal_assertions = fatal; }

/**
   Checks whether the fc_assert* macros should raise on failed assertion.
 */
bool fc_assert_are_fatal() { return fatal_assertions; }

/**
   Handles a failed assertion.
 */
void fc_assert_handle_failure(const char *condition, const char *file,
                              int line, const char *function,
                              const QString &message)
{
  Q_UNUSED(function)
  QMessageLogger logger(file, line, assert_category().categoryName());
  logger.critical("Assertion %s failed", condition);
  if (!message.isEmpty()) {
    logger.critical().noquote() << message;
  }
  logger.critical().noquote() // TRANS: No full stop after the URL.
      << QString(_("Please report this message at %1")).arg(BUG_URL);
  if (fc_assert_are_fatal()) {
    logger.fatal("%s", _("Assertion failed"));
  }
}

void log_time(const QString &msg, bool log)
{
  static bool logging;
  if (log) {
    logging = true;
  }
  if (logging) {
    qInfo() << qUtf8Printable(msg);
  }
}
