/**********************************************************************
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
#pragma once

// Qt
#include <QLoggingCategory>

// Timing logging is has to be filtered out to compare logs when needed.
Q_DECLARE_LOGGING_CATEGORY(timers_category)

enum timer_timetype {
  TIMER_CPU, /* time spent by the CPU */
  TIMER_USER /* time as seen by the user ("wall clock") */
};

enum timer_use {
  TIMER_ACTIVE, /* use this timer */
  TIMER_IGNORE  /* ignore this timer */
};
/*
 * TIMER_IGNORE is to leave a timer in the code, but not actually
 * use it, and not make any time-related system calls for it.
 * It is also used internally to turn off timers if the system
 * calls indicate that timing is not available.
 * Also note TIMER_DEBUG below.
 */

#ifdef FREECIV_DEBUG
#define TIMER_DEBUG TIMER_ACTIVE
#else
#define TIMER_DEBUG TIMER_IGNORE
#endif

class civtimer;

civtimer *timer_new(enum timer_timetype type, enum timer_use use);
civtimer *timer_renew(civtimer *t, enum timer_timetype type,
                      enum timer_use use);

void timer_destroy(civtimer *t);
bool timer_in_use(civtimer *t);

void timer_clear(civtimer *t);
void timer_start(civtimer *t);
void timer_stop(civtimer *t);

double timer_read_seconds(civtimer *t);
