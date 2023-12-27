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

#include <memory>
#include <optional>
#include <variant>

// Forward declarations
class QIODevice;
class QLocalServer;
class QTcpServer;
class QString;

struct connection;

#define BUF_SIZE 512

#define SERVER_LAN_PORT 4555
#define SERVER_LAN_TTL 1
#define SERVER_LAN_VERSION 2

using socket_server =
    std::variant<std::unique_ptr<QTcpServer>, std::unique_ptr<QLocalServer>>;
using optional_socket_server = std::optional<socket_server>;

optional_socket_server server_open_socket();
void flush_packets();
void incoming_client_packets(connection *pconn);
void close_connections_and_socket();
void really_close_connections();
void init_connections();
int server_make_connection(QIODevice *new_sock, const QString &client_addr,
                           const QString &ip_addr);
void finish_unit_waits();
void connection_ping(struct connection *pconn);
void handle_conn_pong(struct connection *pconn);
void handle_client_heartbeat(struct connection *pconn);
void get_lanserver_announcement();
