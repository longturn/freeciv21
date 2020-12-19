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

#include "fc_types.h"

enum client_vote_type { CVT_NONE = 0, CVT_YES, CVT_NO, CVT_ABSTAIN };

struct voteinfo {
  /* Set by the server via packets. */
  int vote_no;
  char user[MAX_LEN_NAME];
  char desc[512];
  int percent_required;
  int flags;
  int yes;
  int no;
  int abstain;
  int num_voters;
  bool resolved;
  bool passed;

  /* Set/used by the client. */
  enum client_vote_type client_vote;
  time_t remove_time;
};

void voteinfo_queue_init(void);
void voteinfo_queue_free(void);
void voteinfo_queue_remove(int vote_no);
void voteinfo_queue_delayed_remove(int vote_no);
void voteinfo_queue_check_removed(void);
void voteinfo_queue_add(int vote_no, const char *user, const char *desc,
                        int percent_required, int flags);
struct voteinfo *voteinfo_queue_find(int vote_no);
void voteinfo_do_vote(int vote_no, enum client_vote_type vote);
struct voteinfo *voteinfo_queue_get_current(int *pindex);
void voteinfo_queue_next(void);
int voteinfo_queue_size(void);

bool voteinfo_bar_can_be_shown(void);


