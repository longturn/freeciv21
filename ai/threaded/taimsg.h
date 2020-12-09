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

#define SPECENUM_NAME taimsgtype
#define SPECENUM_VALUE0 TAI_MSG_THR_EXIT
#define SPECENUM_VALUE0NAME "Exit"
#define SPECENUM_VALUE1 TAI_MSG_FIRST_ACTIVITIES
#define SPECENUM_VALUE1NAME "FirstActivities"
#define SPECENUM_VALUE2 TAI_MSG_PHASE_FINISHED
#define SPECENUM_VALUE2NAME "PhaseFinished"
#include "specenum_gen.h"

#define SPECENUM_NAME taireqtype
#define SPECENUM_VALUE0 TAI_REQ_WORKER_TASK
#define SPECENUM_VALUE0NAME "WorkerTask"
#define SPECENUM_VALUE1 TAI_REQ_TURN_DONE
#define SPECENUM_VALUE1NAME "TurnDone"
#include "specenum_gen.h"

struct tai_msg {
  enum taimsgtype type;
  struct player *plr;
  void *data;
};

struct tai_req {
  enum taireqtype type;
  struct player *plr;
  void *data;
};

#define SPECLIST_TAG taimsg
#define SPECLIST_TYPE struct tai_msg
#include "speclist.h"

#define SPECLIST_TAG taireq
#define SPECLIST_TYPE struct tai_req
#include "speclist.h"

void tai_send_msg(enum taimsgtype type, struct player *pplayer, void *data);
void tai_send_req(enum taireqtype type, struct player *pplayer, void *data);

void tai_first_activities(struct ai_type *ait, struct player *pplayer);
void tai_phase_finished(struct ai_type *ait, struct player *pplayer);


