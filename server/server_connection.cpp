// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "server_connection.h"

#include "game.h"

/**
   Find connection by exact user name, from game.all_connections,
   case-insensitve.  Returns nullptr if not found.
 */
server_connection *conn_by_user(const char *user_name)
{
  conn_list_iterate(game.all_connections, pconn)
  {
    if (fc_strcasecmp(user_name, pconn->username) == 0) {
      return static_cast<server_connection *>(pconn);
    }
  }
  conn_list_iterate_end;

  return nullptr;
}
