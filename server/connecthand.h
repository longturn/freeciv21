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
#ifndef FC__CONNECTHAND_H
#define FC__CONNECTHAND_H

#include "connection.h" // cmdlevel
#include "support.h" /* bool type */

#include "fc_types.h"

struct connection;
struct conn_list;
struct packet_authentication_reply;
struct packet_login_request;
struct packet_server_join_req;

void conn_set_access(struct connection *pconn, enum cmdlevel new_level,
                     bool granted);

void establish_new_connection(struct connection *pconn);
void reject_new_connection(const char *msg, struct connection *pconn);

bool handle_login_request(struct connection *pconn,
                          struct packet_server_join_req *req);

void lost_connection_to_client(struct connection *pconn);

void send_conn_info(struct conn_list *src, struct conn_list *dest);
void send_conn_info_remove(struct conn_list *src, struct conn_list *dest);

struct player *find_uncontrolled_player(void);
bool connection_attach(struct connection *pconn, struct player *pplayer,
                       bool observing);
void connection_detach(struct connection *pconn, bool remove_unused_player);

bool connection_delegate_take(struct connection *pconn,
                              struct player *pplayer);
bool connection_delegate_restore(struct connection *pconn);

void connection_close_server(struct connection *pconn, const char *reason);

#endif /* FC__CONNECTHAND_H */
