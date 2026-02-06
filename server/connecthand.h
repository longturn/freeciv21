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

#include "connection.h" // cmdlevel
#include "support.h"    // bool type

#include "fc_types.h"

class QString;

struct conn_list;
struct packet_server_join_req;
struct server_connection;

void conn_set_access(server_connection *pconn, enum cmdlevel new_level,
                     bool granted);

void establish_new_connection(server_connection *pconn);
void reject_new_connection(const char *msg, server_connection *pconn);

bool handle_login_request(server_connection *pconn,
                          struct packet_server_join_req *req);

void lost_connection_to_client(server_connection *pconn);

void send_conn_info(struct conn_list *src, struct conn_list *dest);
void send_conn_info_remove(struct conn_list *src, struct conn_list *dest);

struct player *find_uncontrolled_player();
bool connection_attach(server_connection *pconn, const char *username,
                       struct player *pplayer, bool observing);
void connection_detach(server_connection *pconn, bool remove_unused_player);

bool connection_delegate_take(server_connection *pconn,
                              struct player *pplayer);
bool connection_delegate_restore(server_connection *pconn);

void connection_close_server(server_connection *pconn,
                             const QString &reason);
