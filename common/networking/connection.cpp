// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors

// self
#include "connection.h"

// generated
#include <packets_gen.h>

// utility
#include "fcintl.h"
#include "genhash.h"
#include "log.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

// common
#include "fc_types.h"
#include "game.h"
#include "packets.h"
#include "player.h"

// Qt
#include <QChar>
#include <QLocalSocket>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QtLogging> // qDebug, qWarning, qCricital, etc

// std
#include <cstdlib> // EXIT_FAILURE, free, at_quick_exit
#include <cstring> // str*, mem*

static void default_conn_close_callback(struct connection *pconn);

/* String used for connection.addr and related cases to indicate
 * blank/unknown/not-applicable address:
 */
const char blank_addr_str[] = "---.---.---.---";

/**
  This callback is used when an error occurs trying to write to the
  connection. The effect of the callback should be to close the connection.
  This is here so that the server and client can take appropriate
  (different) actions: server lost a client, client lost connection to
  server. Never attempt to call this function directly, call
  connection_close() instead.
 */
static conn_close_fn_t conn_close_callback = default_conn_close_callback;

/**
   Default 'conn_close_fn_t' to close a connection.
 */
static void default_conn_close_callback(struct connection *pconn)
{
  fc_assert_msg(conn_close_callback != default_conn_close_callback,
                "Closing a socket (%s) before calling "
                "connections_set_close_callback().",
                conn_description(pconn));
}

/**
   Register the close_callback.
 */
void connections_set_close_callback(conn_close_fn_t func)
{
  conn_close_callback = func;
}

/**
   Call the conn_close_callback.
 */
void connection_close(struct connection *pconn, const QString &reason)
{
  fc_assert_ret(nullptr != pconn);

  if (nullptr != reason && pconn->closing_reason.isEmpty()) {
    // NB: we don't overwrite the original reason.
    pconn->closing_reason = reason;
  }

  (*conn_close_callback)(pconn);
}

/**
   Make sure that there is at least extra_space bytes free space in buffer,
   allocating more memory if needed.
 */
static bool buffer_ensure_free_extra_space(struct socket_packet_buffer *buf,
                                           int extra_space)
{
  // room for more?
  if (buf->nsize - buf->ndata < extra_space) {
    // added this check so we don't gobble up too much mem
    if (buf->ndata + extra_space > MAX_LEN_BUFFER) {
      return false;
    }
    buf->nsize = buf->ndata + extra_space;
    buf->data =
        static_cast<unsigned char *>(fc_realloc(buf->data, buf->nsize));
  }

  return true;
}

/**
   Read data from socket, and check if a packet is ready.
   Returns:
     -1  :  an error occurred - you should close the socket
     -2  :  the connection was closed
     >0  :  number of bytes read
     =0  :  non-blocking sockets only; no data read, would block
 */
int read_socket_data(QIODevice *sock, struct socket_packet_buffer *buffer)
{
  int didget;

  if (!buffer_ensure_free_extra_space(buffer, MAX_LEN_PACKET)) {
    // Let's first process the packets in the buffer
    return 0;
  }

  log_debug("try reading %lu bytes", buffer->nsize - buffer->ndata);
  didget = sock->read(reinterpret_cast<char *>(buffer->data + buffer->ndata),
                      buffer->nsize - buffer->ndata);

  if (didget > 0) {
    buffer->ndata += didget;
    log_debug("didget:%d", didget);
    return didget;
  } else if (didget == 0) {
    log_debug("EOF on socket read");
    return -2;
  }

  return -1;
}

/**
   Write wrapper function -vasc
 */
static int write_socket_data(struct connection *pc,
                             struct socket_packet_buffer *buf, int limit)
{
  int start, nput, nblock;

  if (!conn_is_valid(pc)) {
    return 0;
  }

  for (start = 0; buf->ndata - start > limit;) {
    if (!pc->sock->isOpen()) {
      connection_close(pc, _("network exception"));
      return -1;
    }

    nblock = MIN(buf->ndata - start, MAX_LEN_PACKET);
    log_debug("trying to write %d limit=%d", nblock, limit);
    if ((nput = pc->sock->write(
             reinterpret_cast<const char *>(buf->data) + start, nblock))
        == -1) {
      connection_close(pc, pc->sock->errorString().toUtf8().data());
      return -1;
    }
    start += nput;
  }

  if (start > 0) {
    buf->ndata -= start;
    memmove(buf->data, buf->data + start, buf->ndata);
    pc->last_write = timer_renew(pc->last_write, TIMER_USER, TIMER_ACTIVE);
    timer_start(pc->last_write);
  }

  return 0;
}

/**
   Flush'em
 */
void flush_connection_send_buffer_all(struct connection *pc)
{
  if (pc && pc->used && pc->send_buffer->ndata > 0) {
    write_socket_data(pc, pc->send_buffer, 0);
    if (pc->notify_of_writable_data) {
      pc->notify_of_writable_data(pc, pc->send_buffer
                                          && pc->send_buffer->ndata > 0);
    }
  }
  if (pc && pc->sock) {
    if (auto socket = qobject_cast<QLocalSocket *>(pc->sock)) {
      socket->flush();
    } else if (auto socket = qobject_cast<QTcpSocket *>(pc->sock)) {
      socket->flush();
    }
  }
}

/**
   Flush'em
 */
static void flush_connection_send_buffer_packets(struct connection *pc)
{
  if (pc && pc->used && pc->send_buffer->ndata >= MAX_LEN_PACKET) {
    write_socket_data(pc, pc->send_buffer, MAX_LEN_PACKET - 1);
    if (pc->notify_of_writable_data) {
      pc->notify_of_writable_data(pc, pc->send_buffer
                                          && pc->send_buffer->ndata > 0);
    }
  }
  if (pc && pc->sock) {
    if (auto socket = qobject_cast<QLocalSocket *>(pc->sock)) {
      socket->flush();
    } else if (auto socket = qobject_cast<QTcpSocket *>(pc->sock)) {
      socket->flush();
    }
  }
}

/**
   Add data to send to the connection.
 */
static bool add_connection_data(struct connection *pconn,
                                const unsigned char *data, int len)
{
  struct socket_packet_buffer *buf;

  if (!conn_is_valid(pconn)) {
    return true;
  }

  buf = pconn->send_buffer;
  log_debug("add %d bytes to %lu (space =%lu)", len, buf->ndata, buf->nsize);
  if (!buffer_ensure_free_extra_space(buf, len)) {
    connection_close(pconn, _("buffer overflow"));
    return false;
  }

  memcpy(buf->data + buf->ndata, data, len);
  buf->ndata += len;

  return true;
}

/**
   Write data to socket. Return TRUE on success.
 */
bool connection_send_data(struct connection *pconn,
                          const unsigned char *data, int len)
{
  if (!conn_is_valid(pconn)) {
    return true;
  }

  pconn->statistics.bytes_send += len;

  if (0 < pconn->send_buffer->do_buffer_sends) {
    flush_connection_send_buffer_packets(pconn);
    if (!add_connection_data(pconn, data, len)) {
      qDebug("cut connection %s due to huge send buffer (1)",
             conn_description(pconn));
      return false;
    }
    flush_connection_send_buffer_packets(pconn);
  } else {
    flush_connection_send_buffer_all(pconn);
    if (!add_connection_data(pconn, data, len)) {
      qDebug("cut connection %s due to huge send buffer (2)",
             conn_description(pconn));
      return false;
    }
    flush_connection_send_buffer_all(pconn);
  }
  return true;
}

/**
   Turn on buffering, using a counter so that calls may be nested.
 */
void connection_do_buffer(struct connection *pc)
{
  if (pc && pc->used) {
    pc->send_buffer->do_buffer_sends++;
  }
}

/**
   Turn off buffering if internal counter of number of times buffering
   was turned on falls to zero, to handle nested buffer/unbuffer pairs.
   When counter is zero, flush any pending data.
 */
void connection_do_unbuffer(struct connection *pc)
{
  if (!conn_is_valid(pc)) {
    return;
  }

  pc->send_buffer->do_buffer_sends--;
  if (0 > pc->send_buffer->do_buffer_sends) {
    qCritical("Too many calls to unbuffer %s!", pc->username);
    pc->send_buffer->do_buffer_sends = 0;
  }

  if (0 == pc->send_buffer->do_buffer_sends) {
    flush_connection_send_buffer_all(pc);
  }
}

/**
   Convenience functions to buffer a list of connections.
 */
void conn_list_do_buffer(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) { connection_do_buffer(pconn); }
  conn_list_iterate_end;
}

/**
   Convenience functions to unbuffer a list of connections.
 */
void conn_list_do_unbuffer(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn) { connection_do_unbuffer(pconn); }
  conn_list_iterate_end;
}

/**
   Find connection by id, from game.all_connections.
   Returns nullptr if not found.
   Number of connections will always be relatively small given
   current implementation, so linear search should be fine.
 */
struct connection *conn_by_number(int id)
{
  conn_list_iterate(game.all_connections, pconn)
  {
    fc_assert_msg(pconn != nullptr,
                  "Trying to look at the id of a non existing connection");

    if (pconn->id == id) {
      return pconn;
    }
  }
  conn_list_iterate_end;

  return nullptr;
}

/**
   Return malloced struct, appropriately initialized.
 */
struct socket_packet_buffer *new_socket_packet_buffer()
{
  auto *buf = new socket_packet_buffer;
  buf->ndata = 0;
  buf->do_buffer_sends = 0;
  buf->nsize = 10 * MAX_LEN_PACKET;
  buf->data = static_cast<unsigned char *>(fc_malloc(buf->nsize));

  return buf;
}

/**
   Free malloced struct
 */
static void free_socket_packet_buffer(struct socket_packet_buffer *buf)
{
  if (buf) {
    if (buf->data) {
      free(buf->data);
    }
    delete buf;
    buf = nullptr;
  }
}

/**
 ° Return pointer to static string containing a description for this
 ° connection, based on pconn->name, pconn->addr, and (if applicable)
 ° pconn->playing->name.  (Also pconn->established and pconn->observer.)
 °
 ° Note that when pconn is client.conn (connection to server),
 ° pconn->name and pconn->addr contain empty string, and pconn->playing
 ° is nullptr: in this case return string "server".
 *
 * If `is_private` is true, show the actual hostname, otherwise mask it.
 */
const char *conn_description(const struct connection *pconn, bool is_private)
{
  static char buffer[MAX_LEN_NAME * 2 + MAX_LEN_ADDR + 128];
  buffer[0] = '\0';

  if (*pconn->username != '\0') {
    if (is_private) {
      fc_snprintf(buffer, sizeof(buffer), _("%s from %s"), pconn->username,
                  qUtf8Printable(pconn->addr));
    } else {
      fc_snprintf(buffer, sizeof(buffer), "%s", pconn->username);
    }
  } else {
    sz_strlcpy(buffer, "server");
  }
  if (!pconn->closing_reason.isEmpty()) {
    /* TRANS: Appending the reason why a connection has closed.
     * Preserve leading space. */
    cat_snprintf(buffer, sizeof(buffer), _(" (%s)"),
                 qUtf8Printable(pconn->closing_reason));
  } else if (!pconn->established) {
    // TRANS: preserve leading space.
    sz_strlcat(buffer, _(" (connection incomplete)"));
    return buffer;
  }
  if (nullptr != pconn->playing) {
    // TRANS: preserve leading space.
    cat_snprintf(buffer, sizeof(buffer), _(" (player %s)"),
                 player_name(pconn->playing));
  }
  if (pconn->observer) {
    // TRANS: preserve leading space.
    sz_strlcat(buffer, _(" (observer)"));
  }

  return buffer;
}

/**
   Return TRUE iff the connection is currently allowed to edit.
 */
bool can_conn_edit(const struct connection *pconn)
{
  return (can_conn_enable_editing(pconn) && game.info.is_edit_mode
          && (nullptr != pconn->playing));
}

/**
   Return TRUE iff the connection is allowed to start editing.
 */
bool can_conn_enable_editing(const struct connection *pconn)
{
  return pconn->access_level == ALLOW_HACK;
}

/**
   Get next request id. Takes wrapping of the 16 bit wide unsigned int
   into account.
 */
int get_next_request_id(int old_request_id)
{
  int result = old_request_id + 1;

  if ((result & 0xffff) == 0) {
    log_packet("INFORMATION: request_id has wrapped around; "
               "setting from %d to 2",
               result);
    result = 2;
  }
  fc_assert(0 != result);

  return result;
}

/**
   Allocate and initialize packet hashs for given connection.
 */
static void init_packet_hashs(struct connection *pc)
{
  pc->phs.sent = new genhash *[PACKET_LAST] {};
  pc->phs.received = new genhash *[PACKET_LAST] {};
  pc->phs.handlers = packet_handlers_initial();
}

/**
   Free packet hash resources from given connection.
 */
static void free_packet_hashes(struct connection *pc)
{
  int i;

  if (pc->phs.sent) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.sent[i] != nullptr) {
        genhash_destroy(pc->phs.sent[i]);
      }
    }
    delete[] pc->phs.sent;
    pc->phs.sent = nullptr;
  }

  if (pc->phs.received) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.received[i] != nullptr) {
        genhash_destroy(pc->phs.received[i]);
      }
    }
    delete[] pc->phs.received;
    pc->phs.received = nullptr;
  }
}

/**
   Initialize common part of connection structure. This is used by
   both server and client.
 */
void connection_common_init(struct connection *pconn)
{
  pconn->established = false;
  pconn->used = true;
  packet_header_init(&pconn->packet_header);
  pconn->last_write = nullptr;
  pconn->buffer = new_socket_packet_buffer();
  pconn->send_buffer = new_socket_packet_buffer();
  pconn->statistics.bytes_send = 0;

  init_packet_hashs(pconn);

  byte_vector_init(&pconn->compression.queue);
  pconn->compression.frozen_level = 0;
}

/**
    Connection closing part common to server and client.
 */
void connection_common_close(struct connection *pconn)
{
  if (!pconn->used) {
    qCritical("WARNING: Trying to close already closed connection");
  } else {
    if (pconn->sock)
      pconn->sock->deleteLater();
    pconn->sock = nullptr;
    pconn->used = false;
    pconn->established = false;

    free_socket_packet_buffer(pconn->buffer);
    pconn->buffer = nullptr;

    free_socket_packet_buffer(pconn->send_buffer);
    pconn->send_buffer = nullptr;

    if (pconn->last_write) {
      timer_destroy(pconn->last_write);
      pconn->last_write = nullptr;
    }

    byte_vector_free(&pconn->compression.queue);
    free_packet_hashes(pconn);
  }
}

/**
   Set the network capability string for 'pconn'.
 */
void conn_set_capability(struct connection *pconn, const char *capability)
{
  fc_assert(strlen(capability) < sizeof(pconn->capability));

  sz_strlcpy(pconn->capability, capability);
  pconn->phs.handlers = packet_handlers_get(capability);
}

/**
   Remove all is-game-info cached packets from the connection. This resets
   the delta-state partially.
 */
void conn_reset_delta_state(struct connection *pc)
{
  int i;

  for (i = 0; i < PACKET_LAST; i++) {
    if (packet_has_game_info_flag(packet_type(i))) {
      if (nullptr != pc->phs.sent && nullptr != pc->phs.sent[i]) {
        genhash_clear(pc->phs.sent[i]);
      }
      if (nullptr != pc->phs.received && nullptr != pc->phs.received[i]) {
        genhash_clear(pc->phs.received[i]);
      }
    }
  }
}

/**
   Freeze the connection. Then the packets sent to it won't be sent
   immediatly, but later, using a compression method. See futher details in
   common/packets.[ch].
 */
void conn_compression_freeze(struct connection *pconn)
{
  if (0 == pconn->compression.frozen_level) {
    byte_vector_reserve(&pconn->compression.queue, 0);
  }
  pconn->compression.frozen_level++;
}

/**
   Returns TRUE if the connection is frozen. See also
   conn_compression_freeze().
 */
bool conn_compression_frozen(const struct connection *pconn)
{
  return 0 < pconn->compression.frozen_level;
}

/**
   Freeze a connection list.
 */
void conn_list_compression_freeze(const struct conn_list *pconn_list)
{
  conn_list_iterate(pconn_list, pconn) { conn_compression_freeze(pconn); }
  conn_list_iterate_end;
}

/**
   Thaw a connection list.
 */
void conn_list_compression_thaw(const struct conn_list *pconn_list)
{
  conn_list_iterate(pconn_list, pconn) { conn_compression_thaw(pconn); }
  conn_list_iterate_end;
}

/**
   Returns TRUE if the given connection is attached to a player which it
   also controls (i.e. not a player observer).
 */
bool conn_controls_player(const struct connection *pconn)
{
  return pconn && pconn->playing && !pconn->observer;
}

/**
   Returns TRUE if the given connection is a global observer.
 */
bool conn_is_global_observer(const struct connection *pconn)
{
  return pconn && !pconn->playing && pconn->observer;
}

/**
   Returns the player that this connection is attached to, or nullptr. Note
   that this will return the observed player for connections that are
   observing players.
 */
struct player *conn_get_player(const struct connection *pconn)
{
  if (!pconn) {
    return nullptr;
  }
  return pconn->playing;
}

/**
   Returns the current access level of the given connection.
   NB: If 'pconn' is nullptr, this function will return ALLOW_NONE.
 */
enum cmdlevel conn_get_access(const struct connection *pconn)
{
  if (!pconn) {
    return ALLOW_NONE; // Would not want to give hack on error...
  }
  return pconn->access_level;
}

/**
   Returns TRUE if the connection is valid, i.e. not nullptr, not closed, not
   closing, etc.
 */
bool conn_is_valid(const connection *pconn)
{
  return (pconn && pconn->used && !pconn->is_closing);
}
