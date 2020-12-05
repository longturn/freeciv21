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
#ifndef FC__TEXAICITY_H
#define FC__TEXAICITY_H

/* ai/default */
#include "daicity.h"

struct city;
struct texai_req;

struct texai_city {
  struct ai_city defai; /* Keep this first so default ai finds it */
  int unit_wants[U_LAST];
};

void texai_city_alloc(struct ai_type *ait, struct city *pcity);
void texai_city_free(struct ai_type *ait, struct city *pcity);

void texai_city_worker_requests_create(struct ai_type *ait,
                                       struct player *pplayer,
                                       struct city *pcity);
void texai_city_worker_wants(struct ai_type *ait, struct player *pplayer,
                             struct city *pcity);
void texai_req_worker_task_rcv(struct texai_req *req);

static inline struct texai_city *texai_city_data(struct ai_type *ait,
                                                 const struct city *pcity)
{
  return (struct texai_city *) city_ai_data(pcity, ait);
}

#endif /* FC__TEXAICITY_H */
