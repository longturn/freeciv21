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
#define SPECENUM_NAME texaimsgtype
#define SPECENUM_VALUE0 TEXAI_MSG_THR_EXIT
#define SPECENUM_VALUE0NAME "Exit"
#define SPECENUM_VALUE1 TEXAI_MSG_FIRST_ACTIVITIES
#define SPECENUM_VALUE1NAME "FirstActivities"
#define SPECENUM_VALUE2 TEXAI_MSG_PHASE_FINISHED
#define SPECENUM_VALUE2NAME "PhaseFinished"
#define SPECENUM_VALUE3 TEXAI_MSG_TILE_INFO
#define SPECENUM_VALUE3NAME "TileInfo"
#define SPECENUM_VALUE4 TEXAI_MSG_MAP_ALLOC
#define SPECENUM_VALUE4NAME "MapAlloc"
#define SPECENUM_VALUE5 TEXAI_MSG_MAP_FREE
#define SPECENUM_VALUE5NAME "MapFree"
#define SPECENUM_VALUE6 TEXAI_MSG_CITY_CREATED
#define SPECENUM_VALUE6NAME "CityCreated"
#define SPECENUM_VALUE7 TEXAI_MSG_CITY_CHANGED
#define SPECENUM_VALUE7NAME "CityChanged"
#define SPECENUM_VALUE8 TEXAI_MSG_CITY_DESTROYED
#define SPECENUM_VALUE8NAME "CityDestroyed"
#define SPECENUM_VALUE9 TEXAI_MSG_UNIT_CREATED
#define SPECENUM_VALUE9NAME "UnitCreated"
#define SPECENUM_VALUE10 TEXAI_MSG_UNIT_CHANGED
#define SPECENUM_VALUE10NAME "UnitChanged"
#define SPECENUM_VALUE11 TEXAI_MSG_UNIT_DESTROYED
#define SPECENUM_VALUE11NAME "UnitDestroyed"
#define SPECENUM_VALUE12 TEXAI_MSG_UNIT_MOVED
#define SPECENUM_VALUE12NAME "UnitMoved"
#include "specenum_gen.h"

#define SPECENUM_NAME texaireqtype
#define SPECENUM_VALUE0 TEXAI_REQ_WORKER_TASK
#define SPECENUM_VALUE0NAME "WorkerTask"
#define SPECENUM_VALUE1 TEXAI_REQ_TURN_DONE
#define SPECENUM_VALUE1NAME "TurnDone"
#define SPECENUM_VALUE2 TEXAI_BUILD_CHOICE
#define SPECENUM_VALUE2NAME "BuildChoice"
#include "specenum_gen.h"

struct texai_msg {
  enum texaimsgtype type;
  struct player *plr;
  void *data;
};

struct texai_req {
  enum texaireqtype type;
  struct player *plr;
  void *data;
};

#define SPECLIST_TAG texaimsg
#define SPECLIST_TYPE struct texai_msg
#include "speclist.h"

#define SPECLIST_TAG texaireq
#define SPECLIST_TYPE struct texai_req
#include "speclist.h"

void texai_send_msg(enum texaimsgtype type, struct player *pplayer,
                    void *data);
void texai_send_req(enum texaireqtype type, struct player *pplayer,
                    void *data);

void texai_first_activities(struct ai_type *ait, struct player *pplayer);
void texai_phase_finished(struct ai_type *ait, struct player *pplayer);

