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
#include "timing.h"
#include <QElapsedTimer>

/* utility */
#include "log.h"

enum timer_state { TIMER_STARTED, TIMER_STOPPED };

class civtimer : public QElapsedTimer {
public:
  civtimer(enum timer_timetype type, enum timer_use use);
  enum timer_state state;
  enum timer_timetype type;
  enum timer_use use;
  double sec;
  int msec;
};

civtimer::civtimer(enum timer_timetype ttype, enum timer_use tuse)
    : QElapsedTimer(), state(TIMER_STOPPED), type(ttype), use(tuse),
      sec(0.0), msec(0)
{
}

/***********************************************************************
   Allocate a new timer with specified "type" and "use".
 ***********************************************************************/
civtimer *timer_new(enum timer_timetype type, enum timer_use use)
{
  return timer_renew(NULL, type, use);
}

/************************************************************************
   Allocate a new timer, or reuse t, with specified "type" and "use".
 ***********************************************************************/
civtimer *timer_renew(civtimer *t, enum timer_timetype type,
                      enum timer_use use)
{
  if (!t) {
    t = new civtimer(type, use);
  }
  t->type = type;
  t->use = use;
  timer_clear(t);
  return t;
}

/*******************************************************************/ /**
   Deletes timer
 ***********************************************************************/
void timer_destroy(civtimer *t)
{
  if (t != nullptr) {
    delete t;
  }
}

/*******************************************************************/ /**
   Return whether timer is in use.
   t may be NULL, in which case returns 0
 ***********************************************************************/
bool timer_in_use(civtimer *t) { return (t && t->use != TIMER_IGNORE); }

/*******************************************************************/ /**
   Reset accumulated time to zero, and stop timer if going.
   That is, this may be called whether t is started or stopped;
   in either case the timer is in the stopped state after this function.
 ***********************************************************************/
void timer_clear(civtimer *t)
{
  fc_assert_ret(NULL != t);
  t->state = TIMER_STOPPED;
  t->sec = 0.0;
  t->msec = 0;
}

/*******************************************************************/ /**
   Start timing, adding to previous accumulated time if timer has not
   been cleared.  A warning is printed if the timer is already started.
 ***********************************************************************/
void timer_start(civtimer *t)
{
  fc_assert_ret(NULL != t);

  if (t->use == TIMER_IGNORE) {
    return;
  }
  if (t->state == TIMER_STARTED) {
    qCritical("tried to start already started timer");
    return;
  }
  t->state = TIMER_STARTED;
  t->restart();
}

/*******************************************************************/ /**
   Stop timing, and accumulate time so far.
   (The current time is stored in t->start, so that timer_read_seconds
   can call this to take a point reading if the timer is active.)
   A warning is printed if the timer is already stopped.
 ***********************************************************************/
void timer_stop(civtimer *t)
{
  fc_assert_ret(NULL != t);

  if (t->use == TIMER_IGNORE) {
    return;
  }
  if (t->state == TIMER_STOPPED) {
    qCritical("tried to stop already stopped timer");
    return;
  }
  t->msec = t->elapsed();
  t->sec = (double(t->elapsed()) / 1000);
  t->state = TIMER_STOPPED;
}

/*******************************************************************/ /**
   Read value from timer.  If the timer is not stopped, this stops the
   timer, reads it (and accumulates), and then restarts it.
   Returns 0.0 for unused timers.
 ***********************************************************************/
double timer_read_seconds(civtimer *t)
{
  fc_assert_ret_val(NULL != t, -1.0);

  if (t->use == TIMER_IGNORE) {
    return 0.0;
  }
  if (t->state == TIMER_STARTED) {
    timer_stop(t);
    t->state = TIMER_STARTED;
  }
  return t->sec;
}
