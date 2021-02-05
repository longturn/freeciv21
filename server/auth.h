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

#include "shared.h"

struct connection;

bool auth_user(struct connection *pconn, char *username);
void auth_process_status(struct connection *pconn);
bool auth_handle_reply(struct connection *pconn, char *password);

const char *auth_get_username(struct connection *pconn);
const char *auth_get_ipaddr(struct connection *pconn);
bool auth_set_password(struct connection *pconn, const char *password);
const char *auth_get_password(struct connection *pconn);
bool auth_set_salt(struct connection *pconn, int salt);
int auth_get_salt(struct connection *pconn);
