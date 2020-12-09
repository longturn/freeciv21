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

/* See handicap_desc() for what these do. */
enum handicap_type {
  H_DIPLOMAT = 0,
  H_AWAY,
  H_LIMITEDHUTS,
  H_DEFENSIVE,
  H_EXPERIMENTAL,
  H_RATES,
  H_TARGETS,
  H_HUTS,
  H_FOG,
  H_NOPLANES,
  H_MAP,
  H_DIPLOMACY,
  H_REVOLUTION,
  H_EXPANSION,
  H_DANGER,
  H_CEASEFIRE,
  H_NOBRIBE_WF,
  H_PRODCHGPEN,
  H_LAST
};

BV_DEFINE(bv_handicap, H_LAST);

void handicaps_init(struct player *pplayer);
void handicaps_close(struct player *pplayer);

void handicaps_set(struct player *pplayer, bv_handicap handicaps);
bool has_handicap(const struct player *pplayer, enum handicap_type htype);

const char *handicap_desc(enum handicap_type htype, bool *inverted);


