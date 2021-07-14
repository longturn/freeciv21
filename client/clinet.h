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
class QTcpSocket;
class QString;
class QUrl;

int connect_to_server(const QUrl &url, char *errbuf, int errbufsize);

void make_connection(QTcpSocket *sock, const QString &username);

void input_from_server(QTcpSocket *sock);
void disconnect_from_server();

double try_to_autoconnect(const QUrl &url);
void start_autoconnecting_to_server(const QUrl &url);
