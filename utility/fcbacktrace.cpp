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
#endif /* HAVE_CONFIG_H */

#include <backward.hpp>

/* utility */
#include "log.h"
#include "shared.h"

#include "fcbacktrace.h"

/* We don't want backtrace-spam to testmatic logs */
#if defined(FREECIV_DEBUG) && !defined(FREECIV_TESTMATIC)
#define BACKTRACE_ACTIVE 1
#endif

#ifdef BACKTRACE_ACTIVE
/* We write always in level LOG_NORMAL and not in higher one since those
 * interact badly with server callback to send error messages to local
 * client. */
#define LOG_BACKTRACE LOG_NORMAL

#define MAX_NUM_FRAMES 64

static log_pre_callback_fn previous = NULL;

static void backtrace_log(QtMsgType level, bool print_from_where,
                          const char *where, const char *msg);
#endif /* BACKTRACE_ACTIVE */

/********************************************************************/ /**
   Take backtrace log callback to use
 ************************************************************************/
void backtrace_init(void)
{
#ifdef BACKTRACE_ACTIVE
  previous = log_set_pre_callback(backtrace_log);
#endif
}

/********************************************************************/ /**
   Remove backtrace log callback from use
 ************************************************************************/
void backtrace_deinit(void)
{
#ifdef BACKTRACE_ACTIVE
  log_pre_callback_fn active;

  active = log_set_pre_callback(previous);

  if (active != backtrace_log) {
    /* We were not the active callback!
     * Restore the active callback and log error */
    log_set_pre_callback(active);
    qCritical("Backtrace log (pre)callback cannot be removed");
  }
#endif /* BACKTRACE_ACTIVE */
}

#ifdef BACKTRACE_ACTIVE
/********************************************************************/ /**
   Main backtrace callback called from logging code.
 ************************************************************************/
static void backtrace_log(QtMsgType level, bool print_from_where,
                          const char *where, const char *msg)
{
  if (previous != NULL) {
    /* Call chained callback first */
    previous(level, print_from_where, where, msg);
  }

  if (level <= LOG_ERROR) {
    backtrace_print(LOG_BACKTRACE);
  }
}

#endif /* BACKTRACE_ACTIVE */

/********************************************************************/ /**
   Print backtrace
 ************************************************************************/
void backtrace_print(QtMsgType level)
{
#ifdef BACKTRACE_ACTIVE
  using namespace backward;
  StackTrace st;
  st.load_here(MAX_NUM_FRAMES);

  Printer p;
  p.object = false;
  p.color_mode = ColorMode::automatic;
  p.address = true;
  p.print(st, stderr);
#endif /* BACKTRACE_ACTIVE */
}
