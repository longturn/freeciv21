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

// Qt
#include <QLocalSocket>
#include <QString>
#include <QTcpSocket>
#include <QUrl>

// utility
#include "capstr.h"
#include "fcintl.h"
#include "log.h"

// generated
#include "fc_version.h"

// common
#include "packets.h"

// client
#include "attribute.h"
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "dialogs_g.h" // popdown_races_dialog()
#include "governor.h"
#include "options.h"
#include "packhand.h"
#include "qtg_cxxside.h"

// In autoconnect mode, try to connect to once a second
#define AUTOCONNECT_INTERVAL 500

// In autoconnect mode, try to connect 100 times
#define MAX_AUTOCONNECT_ATTEMPTS 100

/**
   Close socket and cleanup.  This one doesn't print a message, so should
   do so before-hand if necessary.
 */
static void close_socket_nomessage(struct connection *pc)
{
  connection_common_close(pc);
  pc->playing = nullptr;

  popdown_races_dialog();

  set_client_state(C_S_DISCONNECTED);
}

/**
   Client connection close socket callback. It shouldn't be called directy.
   Use connection_close() instead.
 */
static void client_conn_close_callback(struct connection *pconn)
{
  QString reason;

  if (pconn->sock != nullptr) {
    reason = pconn->sock->errorString();
  } else {
    reason = QString::fromUtf8(_("unknown reason"));
  }

  close_socket_nomessage(pconn);
  // If we lost connection to the internal server - kill it.
  client_kill_server(true);
  qCritical("Lost connection to server: %s.", qUtf8Printable(reason));
  output_window_printf(ftc_client, _("Lost connection to server (%s)!"),
                       qUtf8Printable(reason));
}

/**
 * Called when there is an error on the socket.
 */
static void error_on_socket()
{
  if (client.conn.sock != nullptr) {
    log_debug("%s", qUtf8Printable(client.conn.sock->errorString()));
    output_window_append(ftc_client,
                         qUtf8Printable(client.conn.sock->errorString()));
  }
  client.conn.used = false;
}

/**
 * Try to connect to a server:
 *  - try to create a TCP socket to the given URL
 *  - if successful:
 *         - start monitoring the socket for packets from the server
 *         - send a "login request" packet to the server
 *     and - return 0
 *  - if unable to create the connection, close the socket, put an error
 *    message in ERRBUF and return the Unix error code (ie., errno, which
 *    will be non-zero).
 */
static int try_to_connect(const QUrl &url, char *errbuf, int errbufsize)
{
  connections_set_close_callback(client_conn_close_callback);

  // connection in progress? wait.
  if (client.conn.used) {
    (void) fc_strlcpy(errbuf, _("Connection in progress."), errbufsize);
    return -1;
  }

  // Reset
  if (client.conn.sock) {
    client.conn.sock->disconnect(client.conn.sock);
    client.conn.sock->deleteLater();
  }
  client.conn.used = true; // Now there will be a connection :)

  // Connect
  if (url.scheme() == QStringLiteral("fc21")) {
    QTcpSocket *sock = new QTcpSocket;
    client.conn.sock = sock;
    QObject::connect(sock, &QAbstractSocket::errorOccurred,
                     &error_on_socket);
    QObject::connect(sock, &QTcpSocket::disconnected,
                     [] { client.conn.used = false; });
    sock->connectToHost(url.host(), url.port());

    if (!sock->waitForConnected(10000)) {
      (void) fc_strlcpy(errbuf, _("Connection timed out."), errbufsize);
      return -1;
    }
    make_connection(sock, url.userName());

  } else if (url.scheme() == QStringLiteral("fc21+local")) {
    QLocalSocket *sock = new QLocalSocket;
    client.conn.sock = sock;
    QObject::connect(sock, &QLocalSocket::errorOccurred, &error_on_socket);
    QObject::connect(
        sock, &QLocalSocket::disconnected,
        [userName = url.userName()] { client.conn.used = false; });
    sock->connectToServer(url.path());

    if (!sock->waitForConnected(10000)) {
      (void) fc_strlcpy(errbuf, _("Connection timed out."), errbufsize);
      return -1;
    }
    make_connection(sock, url.userName());
  }

  return 0;
}

/**
   Connect to a freeciv21-server instance -- or at least try to.  On success,
   return 0; on failure, put an error message in ERRBUF and return -1.
 */
int connect_to_server(const QUrl &url, char *errbuf, int errbufsize)
{
  if (errbufsize > 0 && errbuf != nullptr) {
    errbuf[0] = '\0';
  }

  if (errbuf && 0 != try_to_connect(url, errbuf, errbufsize)) {
    return -1;
  }

  if (gui_options->use_prev_server) {
    sz_strlcpy(gui_options->default_server_host, qUtf8Printable(url.host()));
    gui_options->default_server_port = url.port(DEFAULT_SOCK_PORT);
  }

  return 0;
}

/**
   Called after a connection is completed (e.g., in try_to_connect).
 */
void make_connection(QIODevice *sock, const QString &username)
{
  struct packet_server_join_req req;

  connection_common_init(&client.conn);
  client.conn.sock = sock;
  client.conn.client.last_request_id_used = 0;
  client.conn.client.last_processed_request_id_seen = 0;
  client.conn.client.request_id_of_currently_handled_packet = 0;
  client.conn.incoming_packet_notify = notify_about_incoming_packet;
  client.conn.outgoing_packet_notify = notify_about_outgoing_packet;

  // call gui-dependent stuff in gui_main.c
  add_net_input(client.conn.sock);

  // now send join_request package

  req.major_version = MAJOR_VERSION;
  req.minor_version = MINOR_VERSION;
  req.patch_version = PATCH_VERSION;
  sz_strlcpy(req.version_label, VERSION_LABEL);
  sz_strlcpy(req.capability, our_capability);
  sz_strlcpy(req.username, qUtf8Printable(username));

  send_packet_server_join_req(&client.conn, &req);
}

/**
   Get rid of server connection. This also kills internal server if it's
   used.
 */
void disconnect_from_server()
{
  const bool force = !client.conn.used;

  attribute_flush();

  stop_turn_change_wait();

  /* If it's internal server - kill him
   * We assume that we are always connected to the internal server  */
  if (!force) {
    client_kill_server(false);
  }
  close_socket_nomessage(&client.conn);
  if (force) {
    client_kill_server(true);
  }
  output_window_append(ftc_client, _("Disconnected from server."));

  if (gui_options->save_options_on_exit) {
    options_save(nullptr);
  }
}

/**
   A wrapper around read_socket_data() which also handles the case the
   socket becomes writeable and there is still data which should be sent
   to the server.

   Returns:
     -1  :  an error occurred - you should close the socket FIXME dropped!
     -2  :  the connection was closed
     >0  :  number of bytes read
     =0  :  no data read, would block
 */
static int read_from_connection(struct connection *pc, bool block)
{
  auto socket = pc->sock;
  bool have_data_for_server =
      (pc->used && pc->send_buffer && 0 < pc->send_buffer->ndata);

  if (!socket->isOpen()) {
    return -2;
  }

  // By the way, if there's some data available for the server let's send
  // it now
  if (have_data_for_server) {
    flush_connection_send_buffer_all(pc);
  }

  if (block) {
    // Wait (and block the main event loop) until we get some data
    socket->waitForReadyRead(-1);
  }

  // Consume everything
  int ret = 0;
  while (socket->bytesAvailable() > 0) {
    int result = read_socket_data(socket, pc->buffer);
    if (result == 0) {
      // There is data in the socket but we can't read it, probably the
      // connection buffer is full.
      break;
    } else if (result > 0) {
      ret += result;
    } else {
      // Error
      return result;
    }
  }
  return ret;
}

/**
   This function is called when the client received a new input from the
   server.
 */
void input_from_server(QIODevice *sock)
{
  int nb;

  fc_assert_ret(sock == client.conn.sock);

  nb = read_from_connection(&client.conn, false);
  if (0 <= nb) {
    governor::i()->freeze();
    while (client.conn.used) {
      enum packet_type type;
      void *packet = get_packet_from_connection(&client.conn, &type);

      if (nullptr != packet) {
        client_packet_input(packet, type);
        ::operator delete(packet);

        if (type == PACKET_PROCESSING_FINISHED) {
          if (client.conn.client.last_processed_request_id_seen
              >= cities_results_request()) {
            cma_got_result(cities_results_request());
          }
        }
      } else {
        break;
      }
    }
    if (client.conn.used) {
      governor::i()->unfreeze();
    }
  } else if (-2 == nb) {
    connection_close(&client.conn, _("server disconnected"));
  } else {
    connection_close(&client.conn, _("read error"));
  }
}

static bool autoconnecting = false;
/**
   Make an attempt to autoconnect to the server.
   It returns number of seconds it should be called again.
 */
double try_to_autoconnect(const QUrl &url)
{
  char errbuf[512];
  static int count = 0;

  // Don't repeat autoconnect if not autoconnecting or the user
  // established a connection by himself.
  if (!autoconnecting || client.conn.established) {
    return FC_INFINITY;
  }

  count++;

  if (count >= MAX_AUTOCONNECT_ATTEMPTS) {
    qFatal(_("Failed to contact server \"%s\" after %d attempts"),
           qUtf8Printable(url.toDisplayString()), count);
    exit(EXIT_FAILURE);
  }

  if (try_to_connect(url, errbuf, sizeof(errbuf)) == 0) {
    // Success! Don't call me again
    autoconnecting = false;
    return FC_INFINITY;
  } else {
    // All errors are fatal
    qCritical(_("Error contacting server \"%s\":\n %s\n"),
              qUtf8Printable(url.toDisplayString()), errbuf);
    exit(EXIT_FAILURE);
  }
}

/**
   Start trying to autoconnect to freeciv21-server.  Calls
   get_server_address(), then arranges for try_to_autoconnect(), which
   calls try_to_connect(), to be called roughly every
   AUTOCONNECT_INTERVAL milliseconds, until success, fatal error or
   user intervention.
 */
void start_autoconnecting_to_server(const QUrl &url)
{
  output_window_printf(
      ftc_client,
      _("Auto-connecting to \"%s\" every %f second(s) for %d times"),
      qUtf8Printable(url.toDisplayString()), 0.001 * AUTOCONNECT_INTERVAL,
      MAX_AUTOCONNECT_ATTEMPTS);

  autoconnecting = true;
}
