// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

// common
#include "connection.h"

// std
#include <vector>

// Qt
#include <QString>

/**
 * A connection, as seen from the server.
 */
struct server_connection : public connection {
  /// Holds the id of the request which is processed now. Can be zero.
  int currently_processed_request_id;

  /// Will increase for every received packet.
  int last_request_id_seen;

  /// The start times of the PACKET_CONN_PING which have been sent but
  /// weren't PACKET_CONN_PONGed yet?
  QList<civtimer *> *ping_timers;

  /// Holds number of tries for authentication from client.
  int auth_tries;

  /**
   * The time that the server will respond after receiving an auth reply.
   * This is used to throttle the connection. Also used to reject a
   * connection if we've waited too long for a password.
   */
  time_t auth_settime;

  /// Used to follow where the connection is in the authentication process
  enum auth_status status;
  char password[MAX_LEN_PASSWORD];

  /// For reverse lookup and blacklisting in db
  char ipaddr[MAX_LEN_ADDR];

  /// The access level initially given to the client upon connection.
  enum cmdlevel granted_access_level;

  /// The list of ignored usernames.
  std::vector<QString> ignore_list;

  /// If we use delegation the original player (playing) is replaced. Save it
  /// here to easily restore it.
  struct {
    bool status; ///< True if player currently delegated to us
    struct player *playing;
    bool observer;
  } delegation;
};

server_connection *conn_by_user(const char *user_name);
server_connection *conn_by_user_prefix(const char *user_name,
                                       enum m_pre_result *result);
