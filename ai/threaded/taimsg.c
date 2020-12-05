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

/* ai/threaded */
#include "taiplayer.h"

#include "taimsg.h"

/**********************************************************************/ /**
   Construct and send message to player thread.
 **************************************************************************/
void tai_send_msg(enum taimsgtype type, struct player *pplayer, void *data)
{
  struct tai_msg *msg;

  if (!tai_thread_running()) {
    /* No player thread to send messages to */
    return;
  }

  msg = fc_malloc(sizeof(*msg));

  msg->type = type;
  msg->plr = pplayer;
  msg->data = data;

  tai_msg_to_thr(msg);
}

/**********************************************************************/ /**
   Construct and send request from player thread.
 **************************************************************************/
void tai_send_req(enum taireqtype type, struct player *pplayer, void *data)
{
  struct tai_req *req = fc_malloc(sizeof(*req));

  req->type = type;
  req->plr = pplayer;
  req->data = data;

  tai_req_from_thr(req);
}

/**********************************************************************/ /**
   Time for phase first activities
 **************************************************************************/
void tai_first_activities(struct ai_type *ait, struct player *pplayer)
{
  tai_send_msg(TAI_MSG_FIRST_ACTIVITIES, pplayer, NULL);
}

/**********************************************************************/ /**
   Player phase has finished
 **************************************************************************/
void tai_phase_finished(struct ai_type *ait, struct player *pplayer)
{
  tai_send_msg(TAI_MSG_PHASE_FINISHED, pplayer, NULL);
}
