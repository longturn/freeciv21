/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

// utility
#include "support.h" // bool type and fc__attribute

// Forward definitions
class QString;

#define MAX_LEN_CONSOLE_LINE                                                \
  1024 // closing '/* closing '\0' included */' included

/*
 * A note on "rfc-style":
 *
 * This style of server output, started with the /rfcstyle server
 * command, prefixes all output with a status number. This is similar
 * to how some common ascii based internet protocols like FTP and SMTP
 * work. A parser can check these numbers to determine whether an
 * action was successful or not, instead of attempting to parse the
 * text (which can be translated into various languages and easily
 * change between versions). This status number is given to the output
 * functions below as their first parameter, or to cmd_reply* as their
 * third parameter.
 */

enum rfc_status {
  C_IGNORE = -1,        // never print RFC-style number prefix
  C_COMMENT = 0,        // for human eyes only
  C_VERSION = 1,        // version info
  C_DEBUG = 2,          // debug info
  C_LOG_BASE = 10,      // 10, 11, 12 depending on log level
  C_OK = 100,           // success of requested operation
  C_CONNECTION = 101,   // new client
  C_DISCONNECTED = 102, // client gone
  C_REJECTED = 103,     // client rejected
  C_FAIL = 200,         // failure of requested operation
  C_METAERROR = 201,    // failure of meta server
  C_SYNTAX = 300,       // syntax error or value out of range
  C_BOUNCE = 301,       // option no longer available
  C_GENFAIL = 400,      // failure not caused by a requested operation
  C_WARNING = 500,      // something may be wrong
  C_READY = 999         // waiting for input
};

#define CON_RED "\u001b[31m"
#define CON_GREEN "\u001b[32m"
#define CON_YELLOW "\u001b[33m"
#define CON_BLUE "\u001b[34m"
#define CON_MAGENTA "\u001b[35m"
#define CON_CYAN "\u001b[36m"
#define CON_WHITE "\u001b[37m"
#define CON_RESET "\u001b[0m"

void con_set_color(const char *);

// initialize logging via console
void con_log_init(const QString &log_filename);
void con_log_close();

// write to console and add line-break, and show prompt if required.
void con_write(enum rfc_status rfc_status, const char *message, ...)
    fc__attribute((__format__(__printf__, 2, 3)));

/* write to console and add line-break, and show prompt if required.
   ie, same as con_write, but without the format string stuff. */
void con_puts(enum rfc_status rfc_status, const char *str);

// ensure timely update
void con_flush();

// initialize prompt; display initial message
void con_prompt_init();

// make sure a prompt is printed, and re-printed after every message
void con_prompt_on();

// do not print a prompt after every message
void con_prompt_off();

// user pressed enter: will need a new prompt
void con_prompt_enter();

// clear "user pressed enter" state (used in special cases)
void con_prompt_enter_clear();

// set server output style
void con_set_style(bool i);

// return server output style
bool con_get_style();
