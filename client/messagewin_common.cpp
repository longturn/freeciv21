/*       ,%%%%%%%,                 ***************************************
       ,%%/\%%%%/\%,         Copyright (c) 1996-2021 Freeciv21 and Freeciv
      ,%%%\c "" J/%%,        contributors. This file is part of Freeciv21.
      %%%%/ d  b \%%%  Freeciv21 is free software: you can redistribute it
      %%%%    _  |%%% and/or modify it under the terms of the GNU  General
      `%%%%(=_Y_=)%%'    Public License  as published by the Free Software
       `%%%%`\7/%%%'         Foundation, either version 3 of the  License,
         `%%%%%%%'       or (at your option) any later version. You should
            have received  a copy of the GNU  General Public License along
                 with Freeciv21. If not, see https://www.gnu.org/licenses/.
 */

#include <cstring>

// utility
#include "fcintl.h"

// common
#include "featured_text.h"
/* client/include */
#include "citydlg_g.h"
#include "mapview_g.h"

// client
#include "client_main.h"
#include "mapview.h"
#include "messagewin_common.h"
#include "options.h"
#include "page_game.h"
#include "update_queue.h"

// gui-qt
#include "messagewin.h"

static struct message **messages = nullptr;
static int messages_total = 0;
static int messages_alloc = 0;

/**
   Update the message dialog if needed.
 */
static void meswin_dialog_update()
{
  if (!can_client_change_view()) {
    return;
  }
  update_queue::uq()->add(real_meswin_dialog_update);
}

/**
   Clear all messages.
 */
void meswin_clear_older(int turn, int phase)
{
  int i;
  int j = 0;

  if (!gui_options.show_previous_turn_messages) {
    turn = MESWIN_CLEAR_ALL;
  }

  for (i = 0; i < messages_total
              && (turn < 0
                  || (messages[i]->turn < turn
                      || (messages[i]->turn == turn
                          && messages[i]->phase < phase)));
       i++) {
    delete[] messages[i]->descr;

    text_tag_list_destroy(messages[i]->tags);
    delete messages[i];
    messages[i] = nullptr;
  }

  if (i != 0) {
    for (; i < messages_total; i++, j++) {
      messages[j] = messages[i];
      messages[i] = nullptr;
    }
    messages_total = j;
  }

  meswin_dialog_update();
}

/**
   Add a message.
 */
void meswin_add(const char *message, const struct text_tag_list *tags,
                struct tile *ptile, enum event_type event, int turn,
                int phase)
{
  const size_t min_msg_len = 50;
  size_t msg_len = qstrlen(message);

  char *s = new char[MAX(msg_len, min_msg_len) + 1];
  int i, nspc;
  struct message *msg;

  if (messages_total + 2 > messages_alloc) {
    messages_alloc = messages_total + 32;
    messages = static_cast<struct message **>(
        fc_realloc(messages, messages_alloc * sizeof(struct message *)));
  }

  msg = new struct message();
  qstrcpy(s, message);

  nspc = min_msg_len - qstrlen(s);
  if (nspc > 0) {
    strncat(s, "                                                  ", nspc);
  }

  msg->tile = ptile;
  msg->event = event;
  msg->descr = s;
  msg->tags = text_tag_list_copy(tags);
  msg->location_ok = (ptile != nullptr);
  msg->visited = false;
  msg->turn = turn;
  msg->phase = phase;
  messages[messages_total++] = msg;

  /* Update the city_ok fields of all messages since the city may have
   * changed owner. */
  for (i = 0; i < messages_total; i++) {
    if (messages[i]->location_ok) {
      struct city *pcity = tile_city(messages[i]->tile);

      messages[i]->city_ok =
          (pcity
           && (!client_has_player()
               || can_player_see_city_internals(client_player(), pcity)));
    } else {
      messages[i]->city_ok = false;
    }
  }

  meswin_dialog_update();
}

/**
   Returns the pointer to a message.  Returns nullptr on error.
 */
const struct message *meswin_get_message(int message_index)
{
  if (message_index >= 0 && message_index < messages_total) {
    return messages[message_index];
  } else {
    // Can happen in turn change...
    return nullptr;
  }
}

/**
   Returns the number of message in the window.
 */
int meswin_get_num_messages() { return messages_total; }

/**
   Sets the visited-state of a message
 */
void meswin_set_visited_state(int message_index, bool state)
{
  fc_assert_ret(0 <= message_index && message_index < messages_total);

  messages[message_index]->visited = state;
}

/**
   Called from messagewin.c if the user clicks on the popup-city button.
 */
void meswin_popup_city(int message_index)
{
  fc_assert_ret(0 <= message_index && message_index < messages_total);

  if (messages[message_index]->city_ok) {
    struct tile *ptile = messages[message_index]->tile;
    struct city *pcity = tile_city(ptile);

    if (gui_options.center_when_popup_city) {
      queen()->mapview_wdg->center_on_tile(ptile);
    }

    if (pcity && can_player_see_units_in_city(client.conn.playing, pcity)) {
      /* If the event was the city being destroyed, pcity will be nullptr
       * and we'd better not try to pop it up.  It's also possible that
       * events will happen on enemy cities; we generally don't want to pop
       * those dialogs up either (although it's hard to be certain).
       *
       * In both cases, it would be better if the popup button weren't
       * highlighted at all - this is left up to the GUI. */
      popup_city_dialog(pcity);
    }
  }
}

/**
   Called from messagewin.c if the user clicks on the goto button.
 */
void meswin_goto(int message_index)
{
  fc_assert_ret(0 <= message_index && message_index < messages_total);

  if (messages[message_index]->location_ok) {
    queen()->mapview_wdg->center_on_tile(messages[message_index]->tile);
  }
}
