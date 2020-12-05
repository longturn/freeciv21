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
