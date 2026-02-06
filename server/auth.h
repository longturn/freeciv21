/***********************************************************************
_   ._       Copyright (c) 1996-2021 Freeciv21 and Freeciv contributors.
 \  |    This file is part of Freeciv21. Freeciv21 is free software: you
  \_|        can redistribute it and/or modify it under the terms of the
 .' '.              GNU General Public License  as published by the Free
 :O O:             Software Foundation, either version 3 of the License,
 '/ \'           or (at your option) any later version. You should have
  :X:      received a copy of the GNU General Public License along with
  :X:              Freeciv21. If not, see https://www.gnu.org/licenses/.
***********************************************************************/
#pragma once

struct server_connection;

bool auth_user(server_connection *pconn, char *username);
void auth_process_status(server_connection *pconn);
bool auth_handle_reply(server_connection *pconn, char *password);

const char *auth_get_username(server_connection *pconn);
const char *auth_get_ipaddr(server_connection *pconn);
