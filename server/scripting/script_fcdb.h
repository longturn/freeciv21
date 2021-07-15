/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

// utility
#include "support.h" // fc__attribute()

// server
#include "fcdb.h"

// Forward declarations
class QString;

struct connection;
struct player;

// fcdb script functions.
bool script_fcdb_init(const QString &fcdb_luafile);
void script_fcdb_free();

bool script_fcdb_do_string(struct connection *caller, const char *str);

// Call Lua functions
bool script_fcdb_user_delegate_to(connection *pconn, player *pplayer,
                                  const char *delegate, bool &success);
bool script_fcdb_user_exists(connection *pconn, bool &exists);
bool script_fcdb_user_save(connection *pconn, const char *password);
bool script_fcdb_user_take(connection *requester, connection *taker,
                           player *player, bool will_observe, bool &success);
bool script_fcdb_user_verify(connection *pconn, const char *username,
                             bool &success);
