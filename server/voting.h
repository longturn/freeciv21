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
#include "support.h" /* bool type */


enum vote_condition_flags {
  VCF_NONE = 0,
  VCF_NODISSENT = (1 << 0),   /* No 'no' votes.' */
  VCF_NOPASSALONE = (1 << 1), /* Can't pass with just one vote for,
                               * when there are multiple voters. */
  VCF_TEAMONLY = (1 << 2)     /* Only team members can vote on it. */
};

enum vote_type { VOTE_YES, VOTE_NO, VOTE_ABSTAIN, VOTE_NUM };

/* Forward declarations. */
struct connection;
struct conn_list;

struct vote_cast {
  enum vote_type vote_cast; /* see enum above */
  int conn_id;              /* user id */
};

#define SPECLIST_TAG vote_cast
#define SPECLIST_TYPE struct vote_cast
#include "speclist.h"
#define vote_cast_list_iterate(alist, pvc)                                  \
  TYPED_LIST_ITERATE(struct vote_cast, alist, pvc)
#define vote_cast_list_iterate_end LIST_ITERATE_END

struct vote {
  int caller_id; /* caller connection id */
  int command_id;
  char cmdline[512]; /* Must match MAX_LEN_CONSOLE_LINE. */
  int turn_count;    /* Number of turns active. */
  struct vote_cast_list *votes_cast;
  int vote_no; /* place in the queue */
  int yes;
  int no;
  int abstain;
  int flags;
  double need_pc;
};

#define SPECLIST_TAG vote
#define SPECLIST_TYPE struct vote
#include "speclist.h"
#define vote_list_iterate(alist, pvote)                                     \
  TYPED_LIST_ITERATE(struct vote, alist, pvote)
#define vote_list_iterate_end LIST_ITERATE_END

extern struct vote_list *vote_list;
extern int vote_number_sequence;

void voting_init(void);
void voting_free(void);
void voting_turn(void);

int count_voters(const struct vote *pvote);
void clear_all_votes(void);
void cancel_connection_votes(struct connection *pconn);
bool conn_can_vote(const struct connection *pconn, const struct vote *pvote);
bool conn_can_see_vote(const struct connection *pconn,
                       const struct vote *pvote);
struct vote *get_vote_by_no(int vote_no);
void connection_vote(struct connection *pconn, struct vote *pvote,
                     enum vote_type type);
struct vote *get_vote_by_caller(const struct connection *caller);
void remove_vote(struct vote *pvote);
struct vote *vote_new(struct connection *caller, const char *allargs,
                      int command_id);
bool vote_would_pass_immediately(const struct connection *caller,
                                 int command_id);
const struct connection *vote_get_caller(const struct vote *pvote);
bool vote_is_team_only(const struct vote *pvote);
int describe_vote(struct vote *pvote, char *buf, int buflen);
void send_running_votes(struct connection *pconn, bool only_team_votes);
void send_remove_team_votes(struct connection *pconn);
void send_updated_vote_totals(struct conn_list *dest);


