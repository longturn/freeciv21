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

/* utility */
#include "fcthread.h"

/* common */
#include "player.h"

/* ai/default */
#include "aidata.h"

/* ai/threaded */
#include "taimsg.h"

struct player;

struct tai_msgs {
  QWaitCondition thr_cond;
  QMutex mutex;
  struct taimsg_list *msglist;
};

struct tai_reqs {
  struct taireq_list *reqlist;
};

struct tai_plr {
  struct ai_plr defai; /* Keep this first so default ai finds it */
};

void tai_init_threading(void);

bool tai_thread_running(void);

void tai_player_alloc(struct ai_type *ait, struct player *pplayer);
void tai_player_free(struct ai_type *ait, struct player *pplayer);
void tai_control_gained(struct ai_type *ait, struct player *pplayer);
void tai_control_lost(struct ai_type *ait, struct player *pplayer);
void tai_refresh(struct ai_type *ait, struct player *pplayer);

void tai_msg_to_thr(struct tai_msg *msg);

void tai_req_from_thr(struct tai_req *req);

static inline struct tai_plr *tai_player_data(struct ai_type *ait,
                                              const struct player *pplayer)
{
  return (struct tai_plr *) player_ai_data(pplayer, ait);
}
