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
#pragma once

// Qt
#include <QLoggingCategory>

#include "fcintl.h"

constexpr auto LOG_FATAL = QtFatalMsg;
constexpr auto LOG_ERROR = QtCriticalMsg;
constexpr auto LOG_WARN = QtWarningMsg;
constexpr auto LOG_NORMAL = QtInfoMsg;
constexpr auto LOG_VERBOSE = QtDebugMsg;
constexpr auto LOG_DEBUG = QtDebugMsg;

/* If one wants to compare autogames with lots of code changes, the line
 * numbers can cause a lot of noise. In that case set this to a fixed
 * value. */
#define __FC_LINE__ __LINE__

void log_close(void);
bool log_init(const QString &level_str = QStringLiteral("info"));
void log_set_file(const QString &path);
const QString &log_get_level();

/* The log macros */
#define log_base(level, message, ...)                                       \
  do {                                                                      \
    switch (level) {                                                        \
    case QtFatalMsg:                                                        \
      qFatal(message, ##__VA_ARGS__);                                       \
      break;                                                                \
    case QtCriticalMsg:                                                     \
      qCritical(message, ##__VA_ARGS__);                                    \
      break;                                                                \
    case QtWarningMsg:                                                      \
      qWarning(message, ##__VA_ARGS__);                                     \
      break;                                                                \
    case QtInfoMsg:                                                         \
      qInfo(message, ##__VA_ARGS__);                                        \
      break;                                                                \
    case QtDebugMsg:                                                        \
      qDebug(message, ##__VA_ARGS__);                                       \
      break;                                                                \
    }                                                                       \
  } while (false)

#ifdef FREECIV_DEBUG
#define log_debug(message, ...) qDebug(message, ##__VA_ARGS__)
#else
#define log_debug(message, ...) {}
#endif

#ifdef FREECIV_TESTMATIC
#define log_testmatic(message, ...) qCritical(message, ##__VA_ARGS__)
#else
#define log_testmatic(message, ...) ((void) 0)
#endif

#define log_testmatic_alt(lvl, ...) log_testmatic(__VA_ARGS__)

/* Used by game debug command */
#define log_test qInfo
#define log_packet qDebug
#define log_packet_detailed log_debug
#define LOG_TEST LOG_NORMAL /* needed by citylog_*() functions */

/* Assertions. */
Q_DECLARE_LOGGING_CATEGORY(assert_category)

void fc_assert_set_fatal(bool fatal_assertions);
bool fc_assert_are_fatal();

void fc_assert_handle_failure(const char *condition, const char *file,
                              int line, const char *function,
                              const QString &message = QString());

/* Like assert(). */
// The lambda below is used to allow returning a value from a multi-line
// macro. We need a macro for line number reporting to work.
#define fc_assert(condition)                                                \
  ((condition)                                                              \
       ? ((void) 0)                                                         \
       : fc_assert_handle_failure(#condition, QT_MESSAGELOG_FILE,           \
                                  QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC))

/* Like assert() with extra message. */
#define fc_assert_msg(condition, message, ...)                              \
  ((condition)                                                              \
       ? ((void) 0)                                                         \
       : fc_assert_handle_failure(                                          \
           #condition, QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE,              \
           QT_MESSAGELOG_FUNC, QString::asprintf(message, ##__VA_ARGS__)))

/* Do action on failure. */
#define fc_assert_action(condition, action)                                 \
  if (!(condition)) {                                                       \
    fc_assert_handle_failure(#condition, QT_MESSAGELOG_FILE,                \
                             QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC);       \
    action;                                                                 \
  }

/* Return on failure. */
#define fc_assert_ret(condition) fc_assert_action(condition, return )
/* Return a value on failure. */
#define fc_assert_ret_val(condition, val)                                   \
  fc_assert_action(condition, return val)
/* Exit on failure. */
#define fc_assert_exit(condition)                                           \
  fc_assert_action(condition, exit(EXIT_FAILURE))

/* Do action on failure with extra message. */
#define fc_assert_action_msg(condition, action, message, ...)               \
  if (!(condition)) {                                                       \
    fc_assert_handle_failure(#condition, QT_MESSAGELOG_FILE,                \
                             QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC,        \
                             QString::asprintf(message, ##__VA_ARGS__));    \
    action;                                                                 \
  }
/* Return on failure with extra message. */
#define fc_assert_ret_msg(condition, message, ...)                          \
  fc_assert_action_msg(condition, return, message, ##__VA_ARGS__)
/* Return a value on failure with extra message. */
#define fc_assert_ret_val_msg(condition, val, message, ...)                 \
  fc_assert_action_msg(condition, return val, message, ##__VA_ARGS__)
/* Exit on failure with extra message. */
#define fc_assert_exit_msg(condition, message, ...)                         \
  fc_assert_action(condition, qFatal(message, ##__VA_ARGS__);               \
                   exit(EXIT_FAILURE))

#ifdef __cplusplus
#ifdef FREECIV_CXX11_STATIC_ASSERT
#define FC_STATIC_ASSERT(cond, tag) static_assert(cond, #tag)
#endif /* FREECIV_CXX11_STATIC_ASSERT */
#else /* __cplusplus */
#ifdef FREECIV_C11_STATIC_ASSERT
#define FC_STATIC_ASSERT(cond, tag) _Static_assert(cond, #tag)
#endif /* FREECIV_C11_STATIC_ASSERT */
#ifdef FREECIV_STATIC_STRLEN
#define FC_STATIC_STRLEN_ASSERT(cond, tag) FC_STATIC_ASSERT(cond, tag)
#else /* FREECIV_STATIC_STRLEN */
#define FC_STATIC_STRLEN_ASSERT(cond, tag)
#endif /* FREECIV_STATIC_STRLEN */
#endif /* __cplusplus */

#ifndef FC_STATIC_ASSERT
/* Static (compile-time) assertion.
 * "tag" is a semi-meaningful C identifier which will appear in the
 * compiler error message if the assertion fails. */
#define FC_STATIC_ASSERT(cond, tag)                                         \
  enum { static_assert_##tag = 1 / (!!(cond)) }
#endif

void log_time(QString msg, bool log = false);


