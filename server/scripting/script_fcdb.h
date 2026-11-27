// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

#pragma once

// utility
#include "support.h" // fc__attribute()

// Forward declarations
class QString;

struct connection;
struct player;

// fcdb script functions.
bool script_fcdb_init(const QString &fcdb_luafile);
void script_fcdb_free();

bool script_fcdb_do_string(struct server_connection *caller,
                           const char *str);

// Call Lua functions
bool script_fcdb_user_delegate_to(server_connection *pconn, player *pplayer,
                                  const char *delegate, bool &success);
bool script_fcdb_user_exists(server_connection *pconn, bool &exists);
bool script_fcdb_user_save(server_connection *pconn, const char *password);
bool script_fcdb_user_take(server_connection *requester,
                           server_connection *taker, player *player,
                           bool will_observe, bool &success);
bool script_fcdb_user_verify(server_connection *pconn, const char *username,
                             bool &success);
