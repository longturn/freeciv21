/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__CLINET_H
#define FC__CLINET_H

// Forward declarations
class QTcpSocket;

int connect_to_server(const char *username, const char *hostname, int port,
                      char *errbuf, int errbufsize);

void make_connection(QTcpSocket *sock, const char *username);

void input_from_server(QTcpSocket *sock);
void input_from_server_till_request_got_processed(QTcpSocket *socket,
                                                  int expected_request_id);
void disconnect_from_server(void);

double try_to_autoconnect(void);
void start_autoconnecting_to_server(void);

#endif /* FC__CLINET_H */
