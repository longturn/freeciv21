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
#ifndef FC__TEXAIWORLD_H
#define FC__TEXAIWORLD_H

#include "texaimsg.h"

void texai_world_init(void);
void texai_world_close(void);

void texai_map_init(void);
void texai_map_close(void);
struct civ_map *texai_map_get(void);

void texai_tile_info(struct tile *ptile);
void texai_tile_info_recv(void *data);

void texai_city_created(struct city *pcity);
void texai_city_changed(struct city *pcity);
void texai_city_info_recv(void *data, enum texaimsgtype msgtype);
void texai_city_destroyed(struct city *pcity);
void texai_city_destruction_recv(void *data);
struct city *texai_map_city(int city_id);

void texai_unit_created(struct unit *punit);
void texai_unit_changed(struct unit *punit);
void texai_unit_info_recv(void *data, enum texaimsgtype msgtype);
void texai_unit_destroyed(struct unit *punit);
void texai_unit_destruction_recv(void *data);
void texai_unit_move_seen(struct unit *punit);
void texai_unit_moved_recv(void *data);

#endif /* FC__TEXAIWORLD_H */
