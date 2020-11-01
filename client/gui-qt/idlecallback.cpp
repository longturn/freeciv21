/**************************************************************************
           ^\      Copyright (c) 1996-2020 ＦＲＥＥＣＩＶ ２１ and Freeciv
 /        //o__o              contributors. This file is part of Freeciv21.
/\       /  __/        Freeciv21 is free software: you can redistribute it
\ \______\  /                  and/or modify it under the terms of the GNU
 \         /              General Public License  as published by the Free
  \ \----\ \          Software Foundation, either version 3 of the License,
   \_\_   \_\_      or (at your option) any later version. You should have
                        received a copy of the  GNU General Public License
           along with Freeciv21. If not, see https://www.gnu.org/licenses/.
**************************************************************************/

#include "idlecallback.h"

mr_idle *mr_idle::m_instance = 0;

/**********************************************************************/ /**
   Constructor for idle callbacks
 **************************************************************************/
mr_idle::mr_idle()
{
  connect(&timer, &QTimer::timeout, this, &mr_idle::idling);
  timer.start(5);
}

/**********************************************************************/ /**
   Destructor for idle callbacks
 **************************************************************************/
mr_idle::~mr_idle()
{
  call_me_back *cb;

  while (!callback_list.isEmpty()) {
    cb = callback_list.dequeue();
    delete cb;
  }
}

/**********************************************************************/ /**
   Deletes current instance
 **************************************************************************/
void mr_idle::drop()
{
  if (m_instance) {
    delete m_instance;
    m_instance = 0;
  }
}

/**********************************************************************/ /**
   Returns given instance
 **************************************************************************/
mr_idle *mr_idle::idlecb()
{
  if (!m_instance)
    m_instance = new mr_idle;
  return m_instance;
}

/**********************************************************************/ /**
   Slot used to execute 1 callback from callbacks stored in idle list
 **************************************************************************/
void mr_idle::idling()
{
  call_me_back *cb;

  while (!callback_list.isEmpty()) {
    cb = callback_list.dequeue();
    (cb->callback)(cb->data);
    delete cb;
  }
}

/**********************************************************************/ /**
   Adds one callback to execute later
 **************************************************************************/
void mr_idle::add_callback(call_me_back *cb) { callback_list.enqueue(cb); }
