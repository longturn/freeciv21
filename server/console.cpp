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

#include <cstdarg>
#include <cstdio>

// utility
#include "fcbacktrace.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

// ms clang wants readline to included here
#include <readline/readline.h>
// common
#include "game.h"

// server
#include "console.h"
#include "notify.h"

static bool console_show_prompt = false;
static bool console_prompt_is_showing = false;
static bool console_rfcstyle = false;
static bool readline_received_enter = true;

namespace {
static QString log_prefix();
static QtMessageHandler original_handler = nullptr;

/**
   Function to handle log messages.
 */
static void console_handle_message(QtMsgType type,
                                   const QMessageLogContext &context,
                                   const QString &message)
{
  con_set_color(CON_GREEN);
  if (type == QtCriticalMsg) {
    con_set_color(CON_RED);
    notify_conn(nullptr, nullptr, E_LOG_ERROR, ftc_warning, "%s",
                qUtf8Printable(message));
  } else if (type == QtFatalMsg) {
    // Make sure that message is not left to buffers when server dies
    conn_list_iterate(game.est_connections, pconn)
    {
      pconn->send_buffer->do_buffer_sends = 0;
      pconn->compression.frozen_level = 0;
    }
    conn_list_iterate_end;

    notify_conn(nullptr, nullptr, E_LOG_FATAL, ftc_warning, "%s",
                qUtf8Printable(message));
    notify_conn(nullptr, nullptr, E_LOG_FATAL, ftc_warning,
                _("Please report this message at %s"), BUG_URL);
  }

  if (original_handler != nullptr) {
    original_handler(type, context, log_prefix() + message);
  }
  con_set_color(CON_RESET);
}
} // anonymous namespace

/**
 Print the prompt if it is not the last thing printed.
 */
static void con_update_prompt()
{
  if (console_prompt_is_showing || !console_show_prompt) {
    return;
  }

  if (readline_received_enter) {
    readline_received_enter = false;
  } else {
    rl_forced_update_display();
  }
  console_prompt_is_showing = true;
}

/**
   Prefix for log messages saved to file. At the moment the turn and the
   current date and time are used.
 */
namespace {
static QString log_prefix()
{
  // TRANS: T for turn
  return game.info.turn > 0 ? QString::asprintf(_("T%03d: "), game.info.turn)
                            : QLatin1String("");
}
} // anonymous namespace

/**
   Initialize logging via console.
 */
void con_log_init(const QString &log_filename)
{
  log_set_file(log_filename);
  backtrace_init();

  // Install our handler last so it gets executed first
  original_handler = qInstallMessageHandler(console_handle_message);
}

/**
   Deinitialize logging
 */
void con_log_close()
{
  backtrace_deinit();

  log_close();
}

void con_set_color(const char *col)
{
  fc_printf("%s", col);
  console_prompt_is_showing = false;
  con_update_prompt();
}
/**
   Write to console and add line-break, and show prompt if required.
 */
void con_write(enum rfc_status rfc_status, const char *message, ...)
{
  // First buffer contains featured text tags
  static char buf1[(MAX_LEN_CONSOLE_LINE * 3) / 2];
  static char buf2[MAX_LEN_CONSOLE_LINE];
  va_list args;

  va_start(args, message);
  fc_vsnprintf(buf1, sizeof(buf1), message, args);
  va_end(args);

  // remove all format tags
  featured_text_to_plain_text(buf1, buf2, sizeof(buf2), nullptr, false);
  con_puts(rfc_status, buf2);
}

/**
   Write to console and add line-break, and show prompt if required.
   Same as con_write, but without the format string stuff.
   The real reason for this is because __attribute__ complained
   with con_write(C_COMMENT,"") of "warning: zero-length format string";
   this allows con_puts(C_COMMENT,"");
 */
void con_puts(enum rfc_status rfc_status, const char *str)
{
  if (rfc_status > 0) {
    con_set_color(CON_YELLOW);
  }
  if (console_prompt_is_showing) {
    fc_printf("\n");
  }
  if ((console_rfcstyle) && (rfc_status >= 0)) {
    fc_printf("%.3d %s\n", rfc_status, str);
  } else {
    fc_printf("%s\n", str);
  }
  console_prompt_is_showing = false;
  con_update_prompt();
  con_set_color(CON_RESET);
}

/**
   Ensure timely update.
 */
void con_flush() { fflush(stdout); }

/**
   Set style.
 */
void con_set_style(bool i)
{
  console_rfcstyle = i;
  if (console_rfcstyle) {
    con_puts(C_OK, _("Ok. RFC-style set."));
  } else {
    con_puts(C_OK, _("Ok. Standard style set."));
  }
}

/**
   Returns rfc-style.
 */
bool con_get_style() { return console_rfcstyle; }

/**
   Initialize prompt; display initial message.
 */
void con_prompt_init()
{
  static bool first = true;

  if (first) {
    con_puts(C_COMMENT, "");
    con_puts(C_COMMENT, _("For introductory help, type 'help'."));
    first = false;
  }
}

/**
   Do not print a prompt after log messages.
 */
void con_prompt_off() { console_show_prompt = false; }

/**
   User pressed enter: will need a new prompt
 */
void con_prompt_enter()
{
  console_prompt_is_showing = false;
  readline_received_enter = true;
}
