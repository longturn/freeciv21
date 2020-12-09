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
#include <QMutex>
#include <QWaitCondition>
/* utility */
#include "fcthread.h"

/* common */
#include "player.h"

/* ai/default */
#include "aidata.h"

/* ai/tex */
#include "texaimsg.h"

struct player;

struct texai_msgs {
  QWaitCondition thr_cond;
  QMutex mutex;
  struct texaimsg_list *msglist;
};

struct texai_reqs {
  struct texaireq_list *reqlist;
};

struct texai_plr {
  struct ai_plr defai; /* Keep this first so default ai finds it */
  struct unit_list *units;
};

struct ai_type *texai_get_self(void); /* Actually in texai.c */

void texai_init_threading(void);

bool texai_thread_running(void);

void texai_map_alloc(void);
void texai_whole_map_copy(void);
void texai_map_free(void);
void texai_player_alloc(struct ai_type *ait, struct player *pplayer);
void texai_player_free(struct ai_type *ait, struct player *pplayer);
void texai_control_gained(struct ai_type *ait, struct player *pplayer);
void texai_control_lost(struct ai_type *ait, struct player *pplayer);
void texai_refresh(struct ai_type *ait, struct player *pplayer);

void texai_msg_to_thr(struct texai_msg *msg);

void texai_req_from_thr(struct texai_req *req);

static inline struct texai_plr *
texai_player_data(struct ai_type *ait, const struct player *pplayer)
{
  return (struct texai_plr *) player_ai_data(pplayer, ait);
}

struct unit_list *texai_player_units(struct player *pplayer);

