/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

#include <fc_config.h>

#include <cstring>

// Qt
#include <QCoreApplication>
#include <QHostInfo>
#include <QLocalServer>
#include <QNetworkDatagram>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

// utility
#include "log.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

// generated
#include "fc_version.h"

// common
#include "dataio.h"
#include "events.h"
#include "game.h"
#include "packets.h"

/* server/scripting */
#include "script_server.h"

// server
#include "aiiface.h"
#include "connecthand.h"
#include "meta.h"
#include "plrhand.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "unittools.h"
#include "voting.h"

#include "sernet.h"

static struct connection connections[MAX_NUM_CONNECTIONS];

static QUdpSocket *udp_socket = nullptr;

#define PROCESSING_TIME_STATISTICS 0

static void start_processing_request(struct connection *pconn,
                                     int request_id);
static void finish_processing_request(struct connection *pconn);

static void send_lanserver_response();

/**
   Close the connection (very low-level). See also
   server_conn_close_callback().
 */
static void close_connection(struct connection *pconn)
{
  if (!pconn) {
    return;
  }

  if (pconn->server.ping_timers != nullptr) {
    while (!pconn->server.ping_timers->isEmpty()) {
      timer_destroy(pconn->server.ping_timers->takeFirst());
    }
    delete pconn->server.ping_timers;
    pconn->server.ping_timers = nullptr;
  }

  conn_pattern_list_destroy(pconn->server.ignore_list);
  pconn->server.ignore_list = nullptr;

  // safe to do these even if not in lists:
  conn_list_remove(game.glob_observers, pconn);
  conn_list_remove(game.all_connections, pconn);
  conn_list_remove(game.est_connections, pconn);

  pconn->playing = nullptr;
  pconn->access_level = ALLOW_NONE;
  connection_common_close(pconn);

  send_updated_vote_totals(nullptr);
}

/**
   Close all network stuff: connections, listening sockets, metaserver
   connection...
 */
void close_connections_and_socket()
{
  int i;

  lsend_packet_server_shutdown(game.all_connections);

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (connections[i].used) {
      close_connection(&connections[i]);
    }
    conn_list_destroy(connections[i].self);
  }

  // Remove the game connection lists and make sure they are empty.
  conn_list_destroy(game.glob_observers);
  conn_list_destroy(game.all_connections);
  conn_list_destroy(game.est_connections);

  if (srvarg.announce != ANNOUNCE_NONE && udp_socket) {
    udp_socket->close();
    delete udp_socket;
    udp_socket = nullptr;
  }

  send_server_info_to_metaserver(META_GOODBYE);
  server_close_meta();

  packets_deinit();
}

/**
   Now really close connections marked as 'is_closing'.
   Do this here to avoid recursive sending.
 */
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
        if (nullptr != conn_get_player(pconn)) {
          conn_list_remove(conn_get_player(pconn)->connections, pconn);
        }
      }
    }

    for (i = 0; i < num; i++) {
      // Now really close them.
      pconn = closing[i];
      lost_connection_to_client(pconn);
      close_connection(pconn);
    }
  } while (0 < num); // May some errors occurred, let's check.
}

/**
   Break a client connection. You should almost always use
   connection_close_server() instead of calling this function directly.
 */
static void server_conn_close_callback(struct connection *pconn)
{
  // Do as little as possible here to avoid recursive evil.
  pconn->server.is_closing = true;
}

/**
   Attempt to flush all information in the send buffers for upto 'netwait'
   seconds.
 */
void flush_packets()
{
  for (auto &i : connections) { // check for freaky players
    struct connection *pconn = &i;

    if (pconn->used && !pconn->server.is_closing) {
      if (!pconn->sock->isOpen()) {
        qDebug("connection (%s) cut due to exception data",
               conn_description(pconn));
        connection_close_server(pconn, _("network exception"));
      } else {
        if (pconn->send_buffer && pconn->send_buffer->ndata > 0) {
          flush_connection_send_buffer_all(pconn);
        }
        // FIXME Handle connections not taking writes
        // They should be cut instead of filling their buffer
      }
    }
  }
}

struct packet_to_handle {
  void *data;
  enum packet_type type;
};

/**
   Simplify a loop by wrapping get_packet_from_connection.
 */
static bool get_packet(struct connection *pconn,
                       struct packet_to_handle *ppacket)
{
  ppacket->data = get_packet_from_connection(pconn, &ppacket->type);

  return nullptr != ppacket->data;
}

/**
   Handle all incoming packets on a client connection.
   Precondition - we have read_socket_data.
   Postcondition - there are no more packets to handle on this connection.
 */
void incoming_client_packets(struct connection *pconn)
{
  struct packet_to_handle packet;
#if PROCESSING_TIME_STATISTICS
  civtimer *request_time = nullptr;
#endif

  while (get_packet(pconn, &packet)) {
    bool command_ok;

#if PROCESSING_TIME_STATISTICS
    int request_id;

    request_time = timer_renew(request_time, TIMER_USER, TIMER_ACTIVE);
    timer_start(request_time);
#endif // PROCESSING_TIME_STATISTICS

    pconn->server.last_request_id_seen =
        get_next_request_id(pconn->server.last_request_id_seen);

#if PROCESSING_TIME_STATISTICS
    request_id = pconn->server.last_request_id_seen;
#endif // PROCESSING_TIME_STATISTICS

    connection_do_buffer(pconn);
    start_processing_request(pconn, pconn->server.last_request_id_seen);

    command_ok = server_packet_input(pconn, packet.data, packet.type);
    ::operator delete(packet.data);

    finish_processing_request(pconn);
    connection_do_unbuffer(pconn);

#if PROCESSING_TIME_STATISTICS
    qDebug("processed request %d in %gms", request_id,
           timer_read_seconds(request_time) * 1000.0);
#endif // PROCESSING_TIME_STATISTICS

    if (!command_ok) {
      connection_close_server(pconn, _("rejected"));
    }
  }

#if PROCESSING_TIME_STATISTICS
  timer_destroy(request_time);
#endif // PROCESSING_TIME_STATISTICS
}

/**
   Make up a name for the connection, before we get any data from
   it to use as a sensible name.  Name will be 'c' + integer,
   guaranteed not to be the same as any other connection name,
   nor player name nor user name, nor connection id (avoid possible
   confusions).   Returns pointer to static buffer, and fills in
   (*id) with chosen value.
 */
static const char *makeup_connection_name(int *id)
{
  static unsigned short i = 0;
  static char name[MAX_LEN_NAME];

  for (;;) {
    if (i == static_cast<unsigned short>(-1)) {
      // don't use 0
      i++;
    }
    fc_snprintf(name, sizeof(name), "c%u", static_cast<unsigned int>(++i));
    if (nullptr == player_by_name(name) && nullptr == player_by_user(name)
        && nullptr == conn_by_number(i) && nullptr == conn_by_user(name)) {
      *id = i;
      return name;
    }
  }
}

/**
   Server accepts connection from client:
   Low level socket stuff, and basic-initialize the connection struct.
   Returns 0 on success, -1 on failure (bad accept(), or too many
   connections).
 */
int server_make_connection(QIODevice *new_sock, const QString &client_addr,
                           const QString &ip_addr)
{
  civtimer *timer;
  int i;

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    struct connection *pconn = &connections[i];

    if (!pconn->used) {
      connection_common_init(pconn);
      pconn->sock = new_sock;
      pconn->closing_reason.clear();
      pconn->observer = false;
      pconn->playing = nullptr;
      pconn->capability[0] = '\0';
      pconn->access_level = access_level_for_next_connection();
      pconn->notify_of_writable_data = nullptr;
      pconn->server.currently_processed_request_id = 0;
      pconn->server.last_request_id_seen = 0;
      pconn->server.auth_tries = 0;
      pconn->server.auth_settime = 0;
      pconn->server.status = AS_NOT_ESTABLISHED;
      pconn->server.ping_timers = new QList<civtimer *>;
      pconn->server.granted_access_level = pconn->access_level;
      pconn->server.ignore_list =
          conn_pattern_list_new_full(conn_pattern_destroy);
      pconn->server.is_closing = false;
      pconn->ping_time = -1.0;
      pconn->incoming_packet_notify = nullptr;
      pconn->outgoing_packet_notify = nullptr;

      sz_strlcpy(pconn->username, makeup_connection_name(&pconn->id));
      pconn->addr = client_addr;
      sz_strlcpy(pconn->server.ipaddr, qUtf8Printable(ip_addr));

      conn_list_append(game.all_connections, pconn);

      qDebug("connection (%s) from %s (%s)", pconn->username,
             qUtf8Printable(pconn->addr), pconn->server.ipaddr);
      /* Give a ping timeout to send the PACKET_SERVER_JOIN_REQ, or close
       * the mute connection. This timer will be canceled into
       * connecthand.c:handle_login_request(). */
      timer = timer_new(TIMER_USER, TIMER_ACTIVE);
      timer_start(timer);
      pconn->server.ping_timers->append(timer);
      return 0;
    }
  }

  // Should not happen as per the check earlier in server_attempt_connection
  qCritical("maximum number of connections reached");
  new_sock->deleteLater();
  return -1;
}

/**
   Open server socket to be used to accept client connections
   and open a server socket for server LAN announcements.
 */
std::optional<socket_server> server_open_socket()
{
  // Local socket mode
  if (!srvarg.local_addr.isEmpty()) {
    auto server = std::make_unique<QLocalServer>();
    if (server->listen(srvarg.local_addr)) {
      connections_set_close_callback(server_conn_close_callback);

      // Don't do LAN announcements
      return server;
    } else {
      qCritical().noquote()
          << QString(_("Server: cannot listen on local socket %1: %2"))
                 .arg(srvarg.local_addr)
                 .arg(server->errorString());
      return std::nullopt;
    }
  }

  // TCP socket mode
  auto server = std::make_unique<QTcpServer>();

  int max = srvarg.port + 100;
  for (; srvarg.port < max; ++srvarg.port) {
    qInfo("Server attempting to listen on %s:%d",
          qUtf8Printable(srvarg.bind_addr.toString()), srvarg.port);
    if (server->listen(srvarg.bind_addr, srvarg.port)) {
      break;
    }

    // Failed
    if (srvarg.user_specified_port) {
      // Failure to meet user expectations.
      qFatal("%s",
             qUtf8Printable(
                 QString::fromUtf8(
                     // TRANS: %1 is a port number, %2 is the error message
                     _("Server: cannot listen on port %1: %2"))
                     .arg(srvarg.port)
                     .arg(server->errorString())));
      return std::nullopt;
    }
  }

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
  // Create socket for server LAN announcements
  udp_socket = new QUdpSocket();

  if (!udp_socket->bind(address_type, SERVER_LAN_PORT,
                        QAbstractSocket::ReuseAddressHint)) {
    qCritical("SO_REUSEADDR failed: %s",
              udp_socket->errorString().toLocal8Bit().data());
    return server;
  }
  auto *group = get_multicast_group(srvarg.announce == ANNOUNCE_IPV6);
  if (!udp_socket->joinMulticastGroup(QHostAddress(group))) {
    qCritical("Announcement socket binding failed: %s",
              udp_socket->errorString().toLocal8Bit().data());
  }

  return server;
}

/**
   Initialize connection related stuff. Attention: Logging is not
   available within this functions!
 */
void init_connections()
{
  int i;

  game.all_connections = conn_list_new();
  game.est_connections = conn_list_new();
  game.glob_observers = conn_list_new();

  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    struct connection *pconn = &connections[i];

    pconn->used = false;
    pconn->self = conn_list_new();
    conn_list_prepend(pconn->self, pconn);
  }
}

/**
   Starts processing of request packet from client.
 */
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

/**
   Finish processing of request packet from client.
 */
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

/**
 * Process all unit waits that are expired.
 */
void finish_unit_waits()
{
  time_t now = time(NULL);
  struct unit *punit;
  struct unit_wait *head;

  while ((head = unit_wait_list_front(server.unit_waits))
         && head->wake_up < now) {

    punit = game_unit_by_number(head->id);
    if (!punit) {
      /* Unit doesn't exist anymore. */
      continue;
    }
    if (head->activity == punit->activity) {
      finish_unit_wait(punit, head->activity_count);
    }
    unit_wait_list_pop_front(server.unit_waits);
  }
}

/**
   Ping a connection.
 */
void connection_ping(struct connection *pconn)
{
  civtimer *timer = timer_new(TIMER_USER, TIMER_ACTIVE);

  log_debug("sending ping to %s (open=%d)", conn_description(pconn),
            pconn->server.ping_timers->size());
  timer_start(timer);
  pconn->server.ping_timers->append(timer);
  send_packet_conn_ping(pconn);
}

/**
   Handle response to ping.
 */
void handle_conn_pong(struct connection *pconn)
{
  civtimer *timer;

  if (pconn->server.ping_timers->size() == 0) {
    qCritical("got unexpected pong from %s", conn_description(pconn));
    return;
  }

  timer = pconn->server.ping_timers->front();
  pconn->ping_time = timer_read_seconds(timer);
  timer_destroy(pconn->server.ping_timers->takeFirst());

  log_time(QStringLiteral("got pong from %1 (open=%2); ping time = %3s")
               .arg(conn_description(pconn))
               .arg(pconn->server.ping_timers->size())
               .arg(pconn->ping_time));
}

/**
   Handle client's regular hearbeat
 */
void handle_client_heartbeat(struct connection *pconn)
{
  log_debug("Received heartbeat");
}

/**
   Listen for UDP packets multicasted from clients requesting
   announcement of servers on the LAN.
 */
void get_lanserver_announcement()
{
  struct data_in din;
  int type;

  if (srvarg.announce == ANNOUNCE_NONE) {
    return;
  }

  if (udp_socket && udp_socket->hasPendingDatagrams()) {
    QNetworkDatagram qnd = udp_socket->receiveDatagram();
    auto data = qnd.data();
    dio_input_init(&din, data.constData(), 1);
    fc_assert_ret_msg(dio_get_uint8_raw(&din, &type), "dio error");
    if (type == SERVER_LAN_VERSION) {
      log_debug("Received request for server LAN announcement.");
      send_lanserver_response();
    } else {
      log_debug("Received invalid request for server LAN announcement.");
    }
  }
}

/**
   This function broadcasts an UDP packet to clients with
   that requests information about the server state.
 */
// We would need a raw network connection for broadcast messages
static void send_lanserver_response()
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
  const char *group;
  size_t size;
  enum QHostAddress::SpecialAddress address_type;
  QUdpSocket lockal_udpsock;

  // Set the UDP Multicast group IP address of the packet.
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
  // Create a description of server state to send to clients.
  if (!srvarg.identity_name.isEmpty()) {
    sz_strlcpy(hostname, qUtf8Printable(srvarg.identity_name));
  } else if (fc_gethostname(hostname, sizeof(hostname)) != 0) {
    sz_strlcpy(hostname, "none");
  }

  fc_snprintf(version, sizeof(version), "%d.%d.%d%s", MAJOR_VERSION,
              MINOR_VERSION, PATCH_VERSION, VERSION_LABEL);

  switch (server_state()) {
  case S_S_INITIAL:
    // TRANS: Game state for local server
    fc_snprintf(status, sizeof(status), _("Pregame"));
    break;
  case S_S_RUNNING:
    // TRANS: Game state for local server
    fc_snprintf(status, sizeof(status), _("Running"));
    break;
  case S_S_OVER:
    // TRANS: Game state for local server
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
