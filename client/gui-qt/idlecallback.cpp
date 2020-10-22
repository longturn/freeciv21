/********************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
                              .___.
          /)               ,-^     ^-.
         //               /           \
.-------| |--------------/  __     __  \-------------------.__
|WMWMWMW| |>>>>>>>>>>>>> | />>\   />>\ |>>>>>>>>>>>>>>>>>>>>>>:>
`-------| |--------------| \__/   \__/ |-------------------'^^
         \\               \    /|\    / You should have received
          \)               \   \_/   / a copy of the GNU General
                            |       | Public License along with this
    ⒻⓇⒺⒺⒸⒾⓋ ②①        |+H+H+H+| program; if not, write to
      GPLv3                 \       / 51 Franklin Street,Fifth Floor
                             ^-----^  Boston, MA  02110-1301, USA.
********************************************************************/

#include "idlecallback.h"

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

