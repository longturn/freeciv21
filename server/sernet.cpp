/***********************************************************************
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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <QNetworkDatagram>
#include <QUdpSocket>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

// Qt
#include <QCoreApplication>
#include <QHostInfo>
#include <QTcpServer>
#include <QTcpSocket>

/* utility */
#include "capability.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

/* common */
#include "dataio.h"
#include "events.h"
#include "game.h"
#include "packets.h"

/* server/scripting */
#include "script_server.h"

/* server */
#include "aiiface.h"
#include "auth.h"
#include "connecthand.h"
#include "console.h"
#include "meta.h"
#include "plrhand.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "voting.h"

#include "sernet.h"

static struct connection connections[MAX_NUM_CONNECTIONS];

static int *listen_socks;
static int listen_count;
static QUdpSocket *udp_socket = nullptr;

#define PROCESSING_TIME_STATISTICS 0

static int server_accept_connection(int sockfd);
static void start_processing_request(struct connection *pconn,
                                     int request_id);
static void finish_processing_request(struct connection *pconn);

static void get_lanserver_announcement(void);
static void send_lanserver_response(void);

/*************************************************************************/ /**
   Close the connection (very low-level). See also
   server_conn_close_callback().
 *****************************************************************************/
static void close_connection(struct connection *pconn)
{
  if (!pconn) {
    return;
  }

  if (pconn->server.ping_timers != NULL) {
    timer_list_destroy(pconn->server.ping_timers);
    pconn->server.ping_timers = NULL;
  }

  conn_pattern_list_destroy(pconn->server.ignore_list);
  pconn->server.ignore_list = NULL;

  /* safe to do these even if not in lists: */
  conn_list_remove(game.glob_observers, pconn);
  conn_list_remove(game.all_connections, pconn);
  conn_list_remove(game.est_connections, pconn);

  pconn->playing = NULL;
  pconn->client_gui = GUI_STUB;
  pconn->access_level = ALLOW_NONE;
  connection_common_close(pconn);

  send_updated_vote_totals(NULL);
}

/*************************************************************************/ /**
   Close all network stuff: connections, listening sockets, metaserver
   connection...
 *****************************************************************************/
void close_connections_and_socket(void)
{
  int i;

  lsend_packet_server_shutdown(game.all_connections);

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (connections[i].used) {
      close_connection(&connections[i]);
    }
    conn_list_destroy(connections[i].self);
  }

  /* Remove the game connection lists and make sure they are empty. */
  conn_list_destroy(game.glob_observers);
  conn_list_destroy(game.all_connections);
  conn_list_destroy(game.est_connections);

  for (i = 0; i < listen_count; i++) {
    fc_closesocket(listen_socks[i]);
  }
  delete[] listen_socks;

  if (srvarg.announce != ANNOUNCE_NONE) {
    udp_socket->close();
    delete udp_socket;
    udp_socket = nullptr;
  }

  send_server_info_to_metaserver(META_GOODBYE);
  server_close_meta();

  packets_deinit();
  fc_shutdown_network();
}

/*************************************************************************/ /**
   Now really close connections marked as 'is_closing'.
   Do this here to avoid recursive sending.
 *****************************************************************************/
void really_close_connections()
{
  struct connection *closing[MAX_NUM_CONNECTIONS];
  struct connection *pconn;
  int i, num;

  do {
    num = 0;

    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      pconn = connections + i;
      if (pconn->used && pconn->server.is_closing) {
        closing[num++] = pconn;
        /* Remove closing connections from the lists (hard detach)
         * to avoid sending to closing connections. */
        conn_list_remove(game.glob_observers, pconn);
        conn_list_remove(game.est_connections, pconn);
        conn_list_remove(game.all_connections, pconn);
        if (NULL != conn_get_player(pconn)) {
          conn_list_remove(conn_get_player(pconn)->connections, pconn);
        }
      }
    }

    for (i = 0; i < num; i++) {
      /* Now really close them. */
      pconn = closing[i];
      lost_connection_to_client(pconn);
      close_connection(pconn);
    }
  } while (0 < num); /* May some errors occurred, let's check. */
}

/*************************************************************************/ /**
   Break a client connection. You should almost always use
   connection_close_server() instead of calling this function directly.
 *****************************************************************************/
static void server_conn_close_callback(struct connection *pconn)
{
  /* Do as little as possible here to avoid recursive evil. */
  pconn->server.is_closing = TRUE;
}

/*************************************************************************/ /**
   If a connection lags too much this function is called and we try to cut
   it.
 *****************************************************************************/
static void cut_lagging_connection(struct connection *pconn)
{
  if (!pconn->server.is_closing && game.server.tcptimeout != 0
      && pconn->last_write && conn_list_size(game.all_connections) > 1
      && pconn->access_level != ALLOW_HACK
      && timer_read_seconds(pconn->last_write) > game.server.tcptimeout) {
    /* Cut the connections to players who lag too much.  This
     * usually happens because client animation slows the client
     * too much and it can't keep up with the server.  We don't
     * cut HACK connections, or cut in single-player games, since
     * it wouldn't help the game progress.  For other connections
     * the best thing to do when they lag too much is to be
     * disconnected and reconnect. */
    log_verbose("connection (%s) cut due to lagging player",
                conn_description(pconn));
    connection_close_server(pconn, _("lagging connection"));
  }
}

/*************************************************************************/ /**
   Attempt to flush all information in the send buffers for upto 'netwait'
   seconds.
 *****************************************************************************/
void flush_packets(void)
{
  for (int i = 0; i < MAX_NUM_CONNECTIONS; i++) { // check for freaky players
    struct connection *pconn = &connections[i];

    if (pconn->used && !pconn->server.is_closing) {
      if (!pconn->sock->isOpen()) {
        log_verbose("connection (%s) cut due to exception data",
                    conn_description(pconn));
        connection_close_server(pconn, _("network exception"));
      } else {
        if (pconn->send_buffer && pconn->send_buffer->ndata > 0) {
          flush_connection_send_buffer_all(pconn);
        }
        // FIXME Handle connections not taking writes
        // They should be cut instead of filling their buffer
        // cut_lagging_connection(pconn);
      }
    }
  }
}

struct packet_to_handle {
  void *data;
  enum packet_type type;
};

/*************************************************************************/ /**
   Simplify a loop by wrapping get_packet_from_connection.
 *****************************************************************************/
static bool get_packet(struct connection *pconn,
                       struct packet_to_handle *ppacket)
{
  ppacket->data = get_packet_from_connection(pconn, &ppacket->type);

  return NULL != ppacket->data;
}

/*************************************************************************/ /**
   Handle all incoming packets on a client connection.
   Precondition - we have read_socket_data.
   Postcondition - there are no more packets to handle on this connection.
 *****************************************************************************/
void incoming_client_packets(struct connection *pconn)
{
  struct packet_to_handle packet;
#if PROCESSING_TIME_STATISTICS
  struct timer *request_time = NULL;
#endif

  while (get_packet(pconn, &packet)) {
    bool command_ok;

#if PROCESSING_TIME_STATISTICS
    int request_id;

    request_time = timer_renew(request_time, TIMER_USER, TIMER_ACTIVE);
    timer_start(request_time);
#endif /* PROCESSING_TIME_STATISTICS */

    pconn->server.last_request_id_seen =
        get_next_request_id(pconn->server.last_request_id_seen);

#if PROCESSING_TIME_STATISTICS
    request_id = pconn->server.last_request_id_seen;
#endif /* PROCESSING_TIME_STATISTICS */

    connection_do_buffer(pconn);
    start_processing_request(pconn, pconn->server.last_request_id_seen);

    command_ok = server_packet_input(pconn, packet.data, packet.type);
    free(packet.data);

    finish_processing_request(pconn);
    connection_do_unbuffer(pconn);

#if PROCESSING_TIME_STATISTICS
    log_verbose("processed request %d in %gms", request_id,
                timer_read_seconds(request_time) * 1000.0);
#endif /* PROCESSING_TIME_STATISTICS */

    if (!command_ok) {
      connection_close_server(pconn, _("rejected"));
    }
  }

#if PROCESSING_TIME_STATISTICS
  timer_destroy(request_time);
#endif /* PROCESSING_TIME_STATISTICS */
}

/*************************************************************************/ /**
   Get and handle:
   - new connections,
   - input from connections,
   - input from server operator in stdin

   This function also handles prompt printing, via the con_prompt_*
   functions.  That is, other functions should not need to do so.  --dwp
 *****************************************************************************/
enum server_events server_sniff_all_input(void)
{
  // TODO later
#if 0
  int i, s;
  int max_desc;
  bool excepting;
  fc_timeval tv;
#ifdef FREECIV_SOCKET_ZERO_NOT_STDIN
  char *bufptr;
#endif

  con_prompt_init();

  while (TRUE) {
    con_prompt_on(); /* accepting new input */

    if (force_end_of_sniff) {
      force_end_of_sniff = FALSE;
      con_prompt_off();
      return S_E_FORCE_END_OF_SNIFF;
    }

    get_lanserver_announcement();

    /* Pinging around for statistics */
    if (time(NULL) > (game.server.last_ping + game.server.pingtime)) {
      /* send data about the previous run */
      send_ping_times_to_all();

      conn_list_iterate(game.all_connections, pconn)
      {
        if ((!pconn->server.is_closing
             && 0 < timer_list_size(pconn->server.ping_timers)
             && timer_read_seconds(
                    timer_list_front(pconn->server.ping_timers))
                    > game.server.pingtimeout)
            || pconn->ping_time > game.server.pingtimeout) {
          /* cut mute players, except for hack-level ones */
          if (pconn->access_level == ALLOW_HACK) {
            log_verbose("connection (%s) [hack-level] ping timeout ignored",
                        conn_description(pconn));
          } else {
            log_verbose("connection (%s) cut due to ping timeout",
                        conn_description(pconn));
            connection_close_server(pconn, _("ping timeout"));
          }
        } else if (pconn->established) {
          /* We don't send ping to connection not established, because
           * we wouldn't be able to handle asynchronous ping/pong with
           * different packet header size. */
          connection_ping(pconn);
        }
      }
      conn_list_iterate_end;
      game.server.last_ping = time(NULL);
    }

    /* if we've waited long enough after a failure, respond to the client */
    conn_list_iterate(game.all_connections, pconn)
    {
      if (srvarg.auth_enabled && !pconn->server.is_closing
          && pconn->server.status != AS_ESTABLISHED) {
        auth_process_status(pconn);
      }
    }
    conn_list_iterate_end

        /* Don't wait if timeout == -1 (i.e. on auto games) */
        if (S_S_RUNNING == server_state() && game.info.timeout == -1)
    {
      call_ai_refresh();
      script_server_signal_emit("pulse");
      (void) send_server_info_to_metaserver(META_REFRESH);
      return S_E_END_OF_TURN_TIMEOUT;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (!no_input) {
#ifdef FREECIV_SOCKET_ZERO_NOT_STDIN
      fc_init_console();
#endif /* FREECIV_SOCKET_ZERO_NOT_STDIN */
    }

    con_prompt_off(); /* output doesn't generate a new prompt */

//     if (!socket->waitForReadyRead(1000 /* ms */)) {
    // TODO timer
    if (fc_select(max_desc + 1, &readfs, &writefs, &exceptfs, &tv) == 0) {
      /* timeout */
      call_ai_refresh();
      script_server_signal_emit("pulse");
      (void) send_server_info_to_metaserver(META_REFRESH);
      if (current_turn_timeout() > 0 && S_S_RUNNING == server_state()
          && game.server.phase_timer
          && (timer_read_seconds(game.server.phase_timer)
                  + game.server.additional_phase_seconds
              > game.tinfo.seconds_to_phasedone)) {
        con_prompt_off();
        return S_E_END_OF_TURN_TIMEOUT;
      }
      if ((game.server.autosaves & (1 << AS_TIMER))
          && S_S_RUNNING == server_state()
          && (timer_read_seconds(game.server.save_timer)
              >= game.server.save_frequency * 60)) {
        save_game_auto("Timer", AS_TIMER);
        game.server.save_timer =
            timer_renew(game.server.save_timer, TIMER_USER, TIMER_ACTIVE);
        timer_start(game.server.save_timer);
      }

      if (!no_input) {
#ifndef FREECIV_SOCKET_ZERO_NOT_STDIN
        really_close_connections();
        continue;
#endif /* FREECIV_SOCKET_ZERO_NOT_STDIN */
      }
    }

    // TODO Probably not needed
    excepting = FALSE;
    for (i = 0; i < listen_count; i++) {
      if (FD_ISSET(listen_socks[i], &exceptfs)) {
        excepting = TRUE;
        break;
      }
    }
    if (excepting) { /* handle Ctrl-Z suspend/resume */
      continue;
    }
  }
  con_prompt_off();

  // TODO use timer
  call_ai_refresh();
  script_server_signal_emit("pulse");

  if (current_turn_timeout() > 0 && S_S_RUNNING == server_state()
      && game.server.phase_timer
      && (timer_read_seconds(game.server.phase_timer)
              + game.server.additional_phase_seconds
          > game.tinfo.seconds_to_phasedone)) {
    return S_E_END_OF_TURN_TIMEOUT;
  }
  if ((game.server.autosaves & (1 << AS_TIMER))
      && S_S_RUNNING == server_state()
      && (timer_read_seconds(game.server.save_timer)
          >= game.server.save_frequency * 60)) {
    save_game_auto("Timer", AS_TIMER);
    game.server.save_timer =
        timer_renew(game.server.save_timer, TIMER_USER, TIMER_ACTIVE);
    timer_start(game.server.save_timer);
  }
#endif // comment
  return S_E_OTHERWISE;
}

/*************************************************************************/ /**
   Make up a name for the connection, before we get any data from
   it to use as a sensible name.  Name will be 'c' + integer,
   guaranteed not to be the same as any other connection name,
   nor player name nor user name, nor connection id (avoid possible
   confusions).   Returns pointer to static buffer, and fills in
   (*id) with chosen value.
 *****************************************************************************/
static const char *makeup_connection_name(int *id)
{
  static unsigned short i = 0;
  static char name[MAX_LEN_NAME];

  for (;;) {
    if (i == (unsigned short) -1) {
      /* don't use 0 */
      i++;
    }
    fc_snprintf(name, sizeof(name), "c%u", (unsigned int) ++i);
    if (NULL == player_by_name(name) && NULL == player_by_user(name)
        && NULL == conn_by_number(i) && NULL == conn_by_user(name)) {
      *id = i;
      return name;
    }
  }
}

/*************************************************************************/ /**
   Server accepts connection from client:
   Low level socket stuff, and basic-initialize the connection struct.
   Returns 0 on success, -1 on failure (bad accept(), or too many
   connections).
 *****************************************************************************/
static int server_accept_connection(QTcpSocket *socket)
{
  // Lookup the host name of the remote end.
  // The IP address will always work
  auto remote = socket->peerAddress().toString();
  // Try a remote DNS lookup
  auto host_info = QHostInfo::fromName(remote); // FIXME Blocking call
  if (host_info.error() == QHostInfo::NoError) {
    remote = host_info.hostName();
  }

  // Reject the connection if we have reached the hard-coded limit
  if (conn_list_size(game.all_connections) >= MAX_NUM_CONNECTIONS) {
    log_verbose("Rejecting new connection from %s: maximum number of "
                "connections exceeded (%d).",
                qPrintable(remote), MAX_NUM_CONNECTIONS);
  }

  // Reject the connection if we have reached the limit for this host
  if (0 != game.server.maxconnectionsperhost) {
    int count = 0;

    conn_list_iterate(game.all_connections, pconn)
    {
      // Use TolerantConversion so one connections from the same address on
      // IPv4 and IPv6 are rejected as well.
      if (socket->peerAddress().isEqual(pconn->sock->peerAddress(),
                                        QHostAddress::TolerantConversion)) {
        continue;
      }
      if (++count >= game.server.maxconnectionsperhost) {
        log_verbose("Rejecting new connection from %s: maximum number of "
                    "connections for this address exceeded (%d).",
                    qPrintable(remote), game.server.maxconnectionsperhost);

        delete socket;
        return -1;
      }
    }
    conn_list_iterate_end;
  }

  return server_make_connection(socket, remote);
}

/*************************************************************************/ /**
   Server accepts connection from client:
   Low level socket stuff, and basic-initialize the connection struct.
   Returns 0 on success, -1 on failure (bad accept(), or too many
   connections).
 *****************************************************************************/
int server_make_connection(QTcpSocket *new_sock, const QString &client_addr)
{
  struct timer *timer;
  int i;

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    struct connection *pconn = &connections[i];

    if (!pconn->used) {
      connection_common_init(pconn);
      pconn->sock = new_sock;
      pconn->observer = FALSE;
      pconn->playing = NULL;
      pconn->capability[0] = '\0';
      pconn->access_level = access_level_for_next_connection();
      pconn->notify_of_writable_data = NULL;
      pconn->server.currently_processed_request_id = 0;
      pconn->server.last_request_id_seen = 0;
      pconn->server.auth_tries = 0;
      pconn->server.auth_settime = 0;
      pconn->server.status = AS_NOT_ESTABLISHED;
      pconn->server.ping_timers = timer_list_new_full(timer_destroy);
      pconn->server.granted_access_level = pconn->access_level;
      pconn->server.ignore_list =
          conn_pattern_list_new_full(conn_pattern_destroy);
      pconn->server.is_closing = FALSE;
      pconn->ping_time = -1.0;
      pconn->incoming_packet_notify = NULL;
      pconn->outgoing_packet_notify = NULL;

      sz_strlcpy(pconn->username, makeup_connection_name(&pconn->id));
      pconn->addr = client_addr;
      sz_strlcpy(pconn->server.ipaddr,
                 qUtf8Printable(new_sock->peerAddress().toString()));

      conn_list_append(game.all_connections, pconn);

      log_verbose("connection (%s) from %s (%s)", pconn->username,
                  qUtf8Printable(pconn->addr), pconn->server.ipaddr);
      /* Give a ping timeout to send the PACKET_SERVER_JOIN_REQ, or close
       * the mute connection. This timer will be canceled into
       * connecthand.c:handle_login_request(). */
      timer = timer_new(TIMER_USER, TIMER_ACTIVE);
      timer_start(timer);
      timer_list_append(pconn->server.ping_timers, timer);
      return 0;
    }
  }

  // Should not happen as per the check earlier in server_attempt_connection
  log_error("maximum number of connections reached");
  new_sock->deleteLater();
  return -1;
}

/*************************************************************************/ /**
   Open server socket to be used to accept client connections
   and open a server socket for server LAN announcements.
 *****************************************************************************/
QTcpServer *server_open_socket()
{
  auto server = new QTcpServer;

  log_verbose("Server attempting to listen on %s:%d",
              srvarg.bind_addr ? srvarg.bind_addr : "(any)", srvarg.port);

  if (!server->listen(QHostAddress::Any, srvarg.port)) {
    // Failed

    // TRANS: %1 is a port number, %2 is the error message
    log_fatal(qPrintable(
        QString::fromUtf8(_("Server: cannot listen on port %1: %2"))
            .arg(srvarg.port)
            .arg(server->errorString())));

    QCoreApplication::exit(EXIT_FAILURE);
    return server;
  }

  // FIXME
  connections_set_close_callback(server_conn_close_callback);

  if (srvarg.announce == ANNOUNCE_NONE) {
    return server;
  }

  enum QHostAddress::SpecialAddress address_type;
  switch (srvarg.announce) {
  case ANNOUNCE_IPV6:
    address_type = QHostAddress::AnyIPv6;
    break;
  case ANNOUNCE_IPV4:
  default:
    address_type = QHostAddress::AnyIPv4;
  }
  /* Create socket for server LAN announcements */
  udp_socket = new QUdpSocket();

  if (!udp_socket->bind(address_type, SERVER_LAN_PORT,
                        QAbstractSocket::ReuseAddressHint)) {
    log_error("SO_REUSEADDR failed: %s",
              udp_socket->errorString().toLocal8Bit().data());
    return server;
  }
  auto group = get_multicast_group(srvarg.announce == ANNOUNCE_IPV6);
  if (!udp_socket->joinMulticastGroup(QHostAddress(group))) {
    log_error("Announcement socket binding failed: %s",
              udp_socket->errorString().toLocal8Bit().data());
  }

  return server;
}

/*************************************************************************/ /**
   Initialize connection related stuff. Attention: Logging is not
   available within this functions!
 *****************************************************************************/
void init_connections(void)
{
  int i;

  game.all_connections = conn_list_new();
  game.est_connections = conn_list_new();
  game.glob_observers = conn_list_new();

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    struct connection *pconn = &connections[i];

    pconn->used = FALSE;
    pconn->self = conn_list_new();
    conn_list_prepend(pconn->self, pconn);
  }
}

/*************************************************************************/ /**
   Starts processing of request packet from client.
 *****************************************************************************/
static void start_processing_request(struct connection *pconn,
                                     int request_id)
{
  fc_assert_ret(request_id);
  fc_assert_ret(pconn->server.currently_processed_request_id == 0);
  log_debug("start processing packet %d from connection %d", request_id,
            pconn->id);
  conn_compression_freeze(pconn);
  send_packet_processing_started(pconn);
  pconn->server.currently_processed_request_id = request_id;
}

/*************************************************************************/ /**
   Finish processing of request packet from client.
 *****************************************************************************/
static void finish_processing_request(struct connection *pconn)
{
  if (!pconn || !pconn->used) {
    return;
  }
  fc_assert_ret(pconn->server.currently_processed_request_id);
  log_debug("finish processing packet %d from connection %d",
            pconn->server.currently_processed_request_id, pconn->id);
  send_packet_processing_finished(pconn);
  pconn->server.currently_processed_request_id = 0;
  conn_compression_thaw(pconn);
}

/*************************************************************************/ /**
   Ping a connection.
 *****************************************************************************/
void connection_ping(struct connection *pconn)
{
  struct timer *timer = timer_new(TIMER_USER, TIMER_ACTIVE);

  log_debug("sending ping to %s (open=%d)", conn_description(pconn),
            timer_list_size(pconn->server.ping_timers));
  timer_start(timer);
  timer_list_append(pconn->server.ping_timers, timer);
  send_packet_conn_ping(pconn);
}

/*************************************************************************/ /**
   Handle response to ping.
 *****************************************************************************/
void handle_conn_pong(struct connection *pconn)
{
  struct timer *timer;

  if (timer_list_size(pconn->server.ping_timers) == 0) {
    log_error("got unexpected pong from %s", conn_description(pconn));
    return;
  }

  timer = timer_list_front(pconn->server.ping_timers);
  pconn->ping_time = timer_read_seconds(timer);
  timer_list_pop_front(pconn->server.ping_timers);
  log_debug("got pong from %s (open=%d); ping time = %fs",
            conn_description(pconn),
            timer_list_size(pconn->server.ping_timers), pconn->ping_time);
}

/*************************************************************************/ /**
   Handle client's regular hearbeat
 *****************************************************************************/
void handle_client_heartbeat(struct connection *pconn)
{
  log_debug("Received heartbeat");
}

/*************************************************************************/ /**
   Send ping time info about all connections to all connections.
 *****************************************************************************/
void send_ping_times_to_all()
{
  struct packet_conn_ping_info packet;
  int i;

  i = 0;
  conn_list_iterate(game.est_connections, pconn)
  {
    if (!pconn->used) {
      continue;
    }
    fc_assert(i < ARRAY_SIZE(packet.conn_id));
    packet.conn_id[i] = pconn->id;
    packet.ping_time[i] = pconn->ping_time;
    i++;
  }
  conn_list_iterate_end;
  packet.connections = i;

  lsend_packet_conn_ping_info(game.est_connections, &packet);
}

/*************************************************************************/ /**
   Listen for UDP packets multicasted from clients requesting
   announcement of servers on the LAN.
 *****************************************************************************/
static void get_lanserver_announcement(void)
{
  fd_set readfs, exceptfs;
  fc_timeval tv;
  char *msgbuf;
  struct data_in din;
  int type;

  if (srvarg.announce == ANNOUNCE_NONE) {
    return;
  }

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if (udp_socket->hasPendingDatagrams()) {
    QNetworkDatagram qnd = udp_socket->receiveDatagram();
    msgbuf = qnd.data().data();
    dio_input_init(&din, msgbuf, 1);
    dio_get_uint8_raw(&din, &type);
    if (type == SERVER_LAN_VERSION) {
      log_debug("Received request for server LAN announcement.");
      send_lanserver_response();
    } else {
      log_debug("Received invalid request for server LAN announcement.");
    }
  }
}

/*************************************************************************/ /**
   This function broadcasts an UDP packet to clients with
   that requests information about the server state.
 *****************************************************************************/
/* We would need a raw network connection for broadcast messages */
static void send_lanserver_response(void)
{
  char buffer[MAX_LEN_PACKET];
  char hostname[512];
  char port[256];
  char version[256];
  char players[256];
  int nhumans;
  char humans[256];
  char status[256];
  struct raw_data_out dout;
  union fc_sockaddr addr;
  int socksend, setting = 1;
  const char *group;
  size_t size;
  enum QHostAddress::SpecialAddress address_type;
  QUdpSocket lockal_udpsock;

  /* Set the UDP Multicast group IP address of the packet. */
  group = get_multicast_group(srvarg.announce == ANNOUNCE_IPV6);
  switch (srvarg.announce) {
  case ANNOUNCE_IPV6:
    address_type = QHostAddress::AnyIPv6;
    break;
  case ANNOUNCE_IPV4:
  default:
    address_type = QHostAddress::AnyIPv4;
  }
  lockal_udpsock.bind(address_type, SERVER_LAN_PORT + 1,
                      QAbstractSocket::ReuseAddressHint);

  lockal_udpsock.joinMulticastGroup(QHostAddress(group));
  /* Create a description of server state to send to clients.  */
  if (srvarg.identity_name[0] != '\0') {
    sz_strlcpy(hostname, srvarg.identity_name);
  } else if (fc_gethostname(hostname, sizeof(hostname)) != 0) {
    sz_strlcpy(hostname, "none");
  }

  fc_snprintf(version, sizeof(version), "%d.%d.%d%s", MAJOR_VERSION,
              MINOR_VERSION, PATCH_VERSION, VERSION_LABEL);

  switch (server_state()) {
  case S_S_INITIAL:
    /* TRANS: Game state for local server */
    fc_snprintf(status, sizeof(status), _("Pregame"));
    break;
  case S_S_RUNNING:
    /* TRANS: Game state for local server */
    fc_snprintf(status, sizeof(status), _("Running"));
    break;
  case S_S_OVER:
    /* TRANS: Game state for local server */
    fc_snprintf(status, sizeof(status), _("Game over"));
    break;
  }

  fc_snprintf(players, sizeof(players), "%d", normal_player_count());

  nhumans = 0;
  players_iterate(pplayer)
  {
    if (pplayer->is_alive && is_human(pplayer)) {
      nhumans++;
    }
  }
  players_iterate_end;
  fc_snprintf(humans, sizeof(humans), "%d", nhumans);

  fc_snprintf(port, sizeof(port), "%d", srvarg.port);

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8_raw(&dout, SERVER_LAN_VERSION);
  dio_put_string_raw(&dout, hostname);
  dio_put_string_raw(&dout, port);
  dio_put_string_raw(&dout, version);
  dio_put_string_raw(&dout, status);
  dio_put_string_raw(&dout, players);
  dio_put_string_raw(&dout, humans);
  dio_put_string_raw(&dout, get_meta_message_string());
  size = dio_output_used(&dout);
  lockal_udpsock.writeDatagram(QByteArray(buffer, size), QHostAddress(group),
                               SERVER_LAN_PORT + 1);
  lockal_udpsock.close();
}
