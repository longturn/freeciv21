/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once
#include "fc_types.h"

#include "hand_gen.h"


struct Treaty;
struct packet_diplomacy_info;
struct connection;

#define SPECLIST_TAG treaty
#define SPECLIST_TYPE struct Treaty
#include "speclist.h"

#define treaty_list_iterate(list, p)                                        \
  TYPED_LIST_ITERATE(struct Treaty, list, p)
#define treaty_list_iterate_end LIST_ITERATE_END

void establish_embassy(struct player *pplayer, struct player *aplayer);

void diplhand_init(void);
void diplhand_free(void);
void free_treaties(void);

struct Treaty *find_treaty(struct player *plr0, struct player *plr1);

void send_diplomatic_meetings(struct connection *dest);
void cancel_all_meetings(struct player *pplayer);
void reject_all_treaties(struct player *pplayer);

struct treaty_list *get_all_treaties(void);


