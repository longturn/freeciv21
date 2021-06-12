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

// Forward declarations
class QString;

bool client_start_server(const QString &user_name);
void client_kill_server(bool force);

bool is_server_running();
bool can_client_access_hack();

void send_client_wants_hack(const char *filename);
void send_save_game(const char *filename);

void set_ruleset(const char *ruleset);
