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

#include <ctime>

// utility
#include "log.h"

// common
#include "packets.h"

/* client/include */
#include "voteinfo_bar_g.h"

// client
#include "client_main.h"
#include "options.h"

#include "voteinfo.h"

// Define struct voteinfo_list type.
#define SPECLIST_TAG voteinfo
#define SPECLIST_TYPE struct voteinfo
#include "speclist.h"
#define voteinfo_list_iterate(alist, pitem)                                 \
  TYPED_LIST_ITERATE(struct voteinfo, alist, pitem)
#define voteinfo_list_iterate_end LIST_ITERATE_END

static struct voteinfo_list *voteinfo_queue = nullptr;
static int voteinfo_queue_current_index = 0;

/**
   Remove the vote with number 'vote_no' after a small amount of time so
   that the user can see that it was removed.
 */
void voteinfo_queue_delayed_remove(int vote_no)
{
  struct voteinfo *vi;

  fc_assert_ret_msg(nullptr != voteinfo_queue,
                    "%s() called before votinfo_queue_init()!",
                    __FUNCTION__);

  vi = voteinfo_queue_find(vote_no);
  if (vi == nullptr) {
    return;
  }
  vi->remove_time = time(nullptr);
}

/**
   Check for old votes that should be removed from the queue. This function
   should be called periodically from a timer callback.
 */
void voteinfo_queue_check_removed()
{
  time_t now;
  struct voteinfo_list *removed;

  if (voteinfo_queue == nullptr) {
    return;
  }

  now = time(nullptr);
  removed = voteinfo_list_new();
  voteinfo_list_iterate(voteinfo_queue, vi)
  {
    if (vi != nullptr && vi->remove_time > 0 && now - vi->remove_time > 2) {
      voteinfo_list_append(removed, vi);
    }
  }
  voteinfo_list_iterate_end;

  voteinfo_list_iterate(removed, vi) { voteinfo_queue_remove(vi->vote_no); }
  voteinfo_list_iterate_end;

  if (voteinfo_list_size(removed) > 0) {
    voteinfo_gui_update();
  }

  voteinfo_list_destroy(removed);
}

/**
   Remove the given vote from the queue immediately.
 */
void voteinfo_queue_remove(int vote_no)
{
  struct voteinfo *vi;

  fc_assert_ret_msg(nullptr != voteinfo_queue,
                    "%s() called before votinfo_queue_init()!",
                    __FUNCTION__);

  vi = voteinfo_queue_find(vote_no);
  if (vi == nullptr) {
    return;
  }

  voteinfo_list_remove(voteinfo_queue, vi);
  delete vi;
}

/**
   Create a new voteinfo record and place it in the queue.
 */
void voteinfo_queue_add(int vote_no, const char *user, const char *desc,
                        int percent_required, int flags)
{
  struct voteinfo *vi;

  fc_assert_ret_msg(nullptr != voteinfo_queue,
                    "%s() called before votinfo_queue_init()!",
                    __FUNCTION__);

  vi = new voteinfo[1]();
  vi->vote_no = vote_no;
  sz_strlcpy(vi->user, user);
  sz_strlcpy(vi->desc, desc);
  vi->percent_required = percent_required;
  vi->flags = flags;

  if (gui_options->voteinfo_bar_new_at_front) {
    voteinfo_list_prepend(voteinfo_queue, vi);
    voteinfo_queue_current_index = 0;
  } else {
    voteinfo_list_append(voteinfo_queue, vi);
  }
}

/**
   Find the voteinfo record corresponding to the given vote number.
 */
struct voteinfo *voteinfo_queue_find(int vote_no)
{
  fc_assert_ret_val_msg(nullptr != voteinfo_queue, nullptr,
                        "%s() called before votinfo_queue_init()!",
                        __FUNCTION__);

  voteinfo_list_iterate(voteinfo_queue, vi)
  {
    if (vi->vote_no == vote_no) {
      return vi;
    }
  }
  voteinfo_list_iterate_end;
  return nullptr;
}

/**
   Initialize data structures used by this module.
 */
void voteinfo_queue_init()
{
  if (voteinfo_queue != nullptr) {
    voteinfo_queue_free();
  }
  voteinfo_queue = voteinfo_list_new();
  voteinfo_queue_current_index = 0;
}

/**
   Free memory allocated by this module.
 */
void voteinfo_queue_free()
{
  if (voteinfo_queue == nullptr) {
    return;
  }

  voteinfo_list_iterate(voteinfo_queue, vi)
  {
    delete vi;
    vi = nullptr;
  }
  voteinfo_list_iterate_end;

  voteinfo_list_destroy(voteinfo_queue);
  voteinfo_queue = nullptr;
  voteinfo_queue_current_index = 0;
}

/**
   Get the voteinfo record at the start of the vote queue. If 'pindex' is
   non-nullptr, it is set to queue index of that record. This function is
   used in conjunction with voteinfo_queue_next().
 */
struct voteinfo *voteinfo_queue_get_current(int *pindex)
{
  struct voteinfo *vi;
  int size;

  if (voteinfo_queue == nullptr) {
    return nullptr;
  }

  size = voteinfo_list_size(voteinfo_queue);

  if (size <= 0) {
    return nullptr;
  }

  if (!(0 <= voteinfo_queue_current_index
        && voteinfo_queue_current_index < size)) {
    voteinfo_queue_next();
  }

  vi = voteinfo_list_get(voteinfo_queue, voteinfo_queue_current_index);

  if (vi != nullptr && pindex != nullptr) {
    *pindex = voteinfo_queue_current_index;
  }

  return vi;
}

/**
   Convenience function for submitting a vote to the server.
   NB: Only to be used if the server has the "voteinfo" capability.
 */
void voteinfo_do_vote(int vote_no, enum client_vote_type vote)
{
  struct voteinfo *vi;
  struct packet_vote_submit packet;

  if (!can_client_control()) {
    return;
  }

  vi = voteinfo_queue_find(vote_no);
  if (vi == nullptr) {
    return;
  }

  packet.vote_no = vi->vote_no;

  switch (vote) {
  case CVT_YES:
    packet.value = 1;
    break;
  case CVT_NO:
    packet.value = -1;
    break;
  case CVT_ABSTAIN:
    packet.value = 0;
    break;
  default:
    return;
    break;
  }

  send_packet_vote_submit(&client.conn, &packet);
  vi->client_vote = vote;
}

/**
   Cycle through the votes in the queue.
 */
void voteinfo_queue_next()
{
  int size;

  if (voteinfo_queue == nullptr) {
    return;
  }

  size = voteinfo_list_size(voteinfo_queue);

  voteinfo_queue_current_index++;
  if (voteinfo_queue_current_index >= size) {
    voteinfo_queue_current_index = 0;
  }
}

/**
   Returns the number of pending votes.
 */
int voteinfo_queue_size()
{
  return (nullptr != voteinfo_queue ? voteinfo_list_size(voteinfo_queue)
                                    : 0);
}
