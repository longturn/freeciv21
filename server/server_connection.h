// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

#include "connection.h"

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

  /// The list of ignored connection patterns.
  struct conn_pattern_list *ignore_list;

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

// Connection patterns.
struct conn_pattern;

#define SPECLIST_TAG conn_pattern
#define SPECLIST_TYPE struct conn_pattern
#include "speclist.h"
#define conn_pattern_list_iterate(plist, ppatern)                           \
  TYPED_LIST_ITERATE(struct conn_pattern, plist, ppatern)
#define conn_pattern_list_iterate_end LIST_ITERATE_END

#define SPECENUM_NAME conn_pattern_type
#define SPECENUM_VALUE0 CPT_USER
#define SPECENUM_VALUE0NAME "user"
#define SPECENUM_VALUE1 CPT_HOST
#define SPECENUM_VALUE1NAME "host"
#define SPECENUM_VALUE2 CPT_IP
#define SPECENUM_VALUE2NAME "ip"
#include "specenum_gen.h"

struct conn_pattern *conn_pattern_new(conn_pattern_type type,
                                      const char *wildcard);
void conn_pattern_destroy(struct conn_pattern *ppattern);

bool conn_pattern_match(const conn_pattern *ppattern,
                        const server_connection *pconn);
bool conn_pattern_list_match(const conn_pattern_list *plist,
                             const server_connection *pconn);

size_t conn_pattern_to_string(const conn_pattern *ppattern, char *buf,
                              size_t buf_len);
conn_pattern *conn_pattern_from_string(const char *pattern,
                                       conn_pattern_type prefer,
                                       char *error_buf,
                                       size_t error_buf_len);
