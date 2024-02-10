/*
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
 */

// common
#include "game.h"
// client
#include "chatline_common.h"
#include "fc_client.h"
#include "packhand_gen.h"
#include "page_network.h"
#include "pages_g.h"
#include "qtg_cxxside.h"

/**
   Configure the dialog depending on what type of authentication request the
   server is making.
 */
void handle_authentication_req(enum authentication_type type,
                               const char *message)
{
  set_client_page(PAGE_NETWORK);
  qobject_cast<page_network *>(king()->pages[PAGE_NETWORK])
      ->handle_authentication_req(type, message);
}

/**
   Provide a packet handler for packet_game_load.

   This regenerates the player information from a loaded game on the
   server.
 */
void handle_game_load(bool load_successful, const char *filename)
{
  Q_UNUSED(filename)
  if (load_successful) {
    set_client_page(PAGE_START);

    if (game.info.is_new_game) {
      // It's pregame. Create a player and connect to him
      send_chat("/take -");
    }
  }
}
