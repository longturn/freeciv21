// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

/**************************************************************************
 *  The connection struct and related stuff.
 *  Includes cmdlevel stuff, which is connection-based.
 ***************************************************************************/

#pragma once

// utility
#include "support.h" // bool type
#include "timing.h"

// common
#include "fc_types.h"

// Qt
#include <QList>
#include <QString>

// std
#include <ctime> // time_t

// Forward declarations
class QIODevice;
class QString;

struct conn_pattern_list;
struct genhash;
struct packet_handlers;

/***************************************************************************
  On the distinction between nations(formerly races), players, and users,
  see doc/HACKING
***************************************************************************/

// where the connection is in the authentication process
enum auth_status {
  AS_NOT_ESTABLISHED = 0,
  AS_FAILED,
  AS_REQUESTING_NEW_PASS,
  AS_REQUESTING_OLD_PASS,
  AS_ESTABLISHED
};

/***********************************************************
  This is a buffer where the data is first collected,
  whenever it arrives to the client/server.
***********************************************************/
struct socket_packet_buffer {
  unsigned long ndata;
  int do_buffer_sends;
  unsigned long nsize;
  unsigned char *data;
};

struct packet_header {
  unsigned int length : 4; // Actually 'enum data_type'
  unsigned int type : 4;   // Actually 'enum data_type'
};

#define SPECVEC_TAG byte
#define SPECVEC_TYPE unsigned char
#include "specvec.h"

/***********************************************************
  The connection struct represents a single client or server
  at the other end of a network connection.
***********************************************************/
struct connection {
  int id; /* used for server/client communication */
  QIODevice *sock = nullptr;
  bool used;
  bool established; // have negotiated initial packets
  struct packet_header packet_header;
  QString closing_reason;

  /// Something has occurred that means the connection should be closed, but
  /// the closing has been postponed.
  bool is_closing = false;

  /* connection is "observer", not controller; may be observing
   * specific player, or all (implementation incomplete).
   */
  bool observer;

  /* nullptr for connections not yet associated with a specific player.
   */
  struct player *playing;

  struct socket_packet_buffer *buffer;
  struct socket_packet_buffer *send_buffer;
  class civtimer *last_write;

  double ping_time;

  struct conn_list *self; // list with this connection as single element
  char username[MAX_LEN_NAME];
  QString addr;

  /*
   * "capability" gives the capability string of the executable (be it
   * a client or server) at the other end of the connection.
   */
  char capability[MAX_LEN_CAPSTR];

  /*
   * "access_level" stores the current access level of the client
   * corresponding to this connection.
   */
  enum cmdlevel access_level;

  void (*notify_of_writable_data)(struct connection *pc,
                                  bool data_available_and_socket_full);

  /*
   * Called before an incoming packet is processed. The packet_type
   * argument should really be a "enum packet_type". However due
   * circular dependency this is impossible.
   */
  void (*incoming_packet_notify)(struct connection *pc, int packet_type,
                                 int size);

  /*
   * Called before a packet is sent. The packet_type argument should
   * really be a "enum packet_type". However due circular dependency
   * this is impossible.
   */
  void (*outgoing_packet_notify)(struct connection *pc, int packet_type,
                                 int size, int request_id);
  struct {
    struct genhash **sent;
    struct genhash **received;
    const struct packet_handlers *handlers;
  } phs;

  struct {
    int frozen_level;

    struct byte_vector queue;
  } compression;
  struct {
    int bytes_send;
  } statistics;

  /// Increases for every packet sent.
  int last_request_id_used;
};

typedef void (*conn_close_fn_t)(struct connection *pconn);
void connections_set_close_callback(conn_close_fn_t func);
void connection_close(struct connection *pconn, const QString &reason);

int read_socket_data(QIODevice *sock, struct socket_packet_buffer *buffer);
void flush_connection_send_buffer_all(struct connection *pc);
bool connection_send_data(struct connection *pconn,
                          const unsigned char *data, int len);

void connection_do_buffer(struct connection *pc);
void connection_do_unbuffer(struct connection *pc);

void conn_list_do_buffer(struct conn_list *dest);
void conn_list_do_unbuffer(struct conn_list *dest);

struct connection *conn_by_number(int id);

struct socket_packet_buffer *new_socket_packet_buffer();
void connection_common_init(struct connection *pconn);
void connection_common_close(struct connection *pconn);
void conn_set_capability(struct connection *pconn, const char *capability);
void conn_reset_delta_state(struct connection *pconn);

void conn_compression_freeze(struct connection *pconn);
bool conn_compression_thaw(struct connection *pconn);
bool conn_compression_frozen(const struct connection *pconn);
void conn_list_compression_freeze(const struct conn_list *pconn_list);
void conn_list_compression_thaw(const struct conn_list *pconn_list);

const char *conn_description(const struct connection *pconn,
                             bool is_private = true);
bool conn_controls_player(const struct connection *pconn);
bool conn_is_global_observer(const struct connection *pconn);
enum cmdlevel conn_get_access(const struct connection *pconn);

struct player;
struct player *conn_get_player(const struct connection *pconn);

bool can_conn_edit(const struct connection *pconn);
bool can_conn_enable_editing(const struct connection *pconn);

int get_next_request_id(int old_request_id);

extern const char blank_addr_str[];

bool conn_is_valid(const connection *pconn);
