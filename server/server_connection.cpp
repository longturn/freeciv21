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

/**
 * Helper for match_prefix.
 */
static const char *connection_accessor(int i)
{
  return conn_list_get(game.all_connections, i)->username;
}

/**
   Like conn_by_username(), but allow unambigous prefix (i.e. abbreviation).
   Returns nullptr if could not match, or if ambiguous or other problem, and
   fills *result with characterisation of match/non-match (see
   "utility/shared.[ch]").
 */
server_connection *conn_by_user_prefix(const char *user_name,
                                       enum m_pre_result *result)
{
  int ind;

  *result =
      match_prefix(connection_accessor, conn_list_size(game.all_connections),
                   MAX_LEN_NAME - 1, fc_strncasequotecmp,
                   effectivestrlenquote, user_name, &ind);

  if (*result < M_PRE_AMBIGUOUS) {
    return static_cast<server_connection *>(
        conn_list_get(game.all_connections, ind));
  } else {
    return nullptr;
  }
}
