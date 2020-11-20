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

// Qt
#include <QString>
#include <QTcpSocket>

/* utility */
#include "capstr.h"
#include "dataio.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "support.h"

/* common */
#include "game.h"
#include "packets.h"
#include "version.h"

/* client */
#include "agents.h"
#include "attribute.h"
#include "chatline_g.h"
#include "client_main.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "connectdlg_g.h"
#include "dialogs_g.h"      /* popdown_races_dialog() */
#include "gui_main_g.h"     /* add_net_input(), remove_net_input() */
#include "mapview_common.h" /* unqueue_mapview_update */
#include "menu_g.h"
#include "messagewin_g.h"
#include "options.h"
#include "packhand.h"
#include "pages_g.h"
#include "plrdlg_g.h"
#include "repodlgs_g.h"

#include "clinet.h"

/* In autoconnect mode, try to connect to once a second */
#define AUTOCONNECT_INTERVAL 500

/* In autoconnect mode, try to connect 100 times */
#define MAX_AUTOCONNECT_ATTEMPTS 100

extern char forced_tileset_name[512];

/**********************************************************************/ /**
   Close socket and cleanup.  This one doesn't print a message, so should
   do so before-hand if necessary.
 **************************************************************************/
static void close_socket_nomessage(struct connection *pc)
{
  connection_common_close(pc);
  remove_net_input();
  popdown_races_dialog();
  close_connection_dialog();

  set_client_state(C_S_DISCONNECTED);
}

/**********************************************************************/ /**
   Client connection close socket callback. It shouldn't be called directy.
   Use connection_close() instead.
 **************************************************************************/
static void client_conn_close_callback(struct connection *pconn)
{
  QString reason;

  if (pconn->sock != nullptr) {
    reason = pconn->sock->errorString();
  } else {
    reason = QString::fromUtf8(_("unknown reason"));
  }

  close_socket_nomessage(pconn);
  /* If we lost connection to the internal server - kill it. */
  client_kill_server(TRUE);
  qCritical("Lost connection to server: %s.", qPrintable(reason));
  output_window_printf(ftc_client, _("Lost connection to server (%s)!"),
                       qUtf8Printable(reason));
}

/**********************************************************************/ /**
   Try to connect to a server:
    - try to create a TCP socket to the given hostname and port (default to
      localhost:DEFAULT_SOCK_PORT) and connect it to `names'.
    - if successful:
           - start monitoring the socket for packets from the server
           - send a "login request" packet to the server
       and - return 0
    - if unable to create the connection, close the socket, put an error
      message in ERRBUF and return the Unix error code (ie., errno, which
      will be non-zero).
 **************************************************************************/
static int try_to_connect(QString &hostname, int port, QString &username,
                          char *errbuf, int errbufsize)
{
  // Apply defaults
  if (hostname == nullptr) {
    hostname = "localhost";
  }

  if (port == 0) {
    port = DEFAULT_SOCK_PORT;
  }

  connections_set_close_callback(client_conn_close_callback);

  /* connection in progress? wait. */
  if (client.conn.used) {
    (void) fc_strlcpy(errbuf, _("Connection in progress."), errbufsize);
    return -1;
  }
  client.conn.used = true; // Now there will be a connection :)

  // Connect
  client.conn.sock = new QTcpSocket;

  QObject::connect(
      client.conn.sock,
      QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
      [] {
        connection_close(&client.conn,
                         qUtf8Printable(client.conn.sock->errorString()));
      });

  client.conn.sock->connectToHost(hostname, port);
  if (!client.conn.sock->waitForConnected(-1)) {
    errbuf[0] = '\0';
    return -1;
  }

  make_connection(client.conn.sock, username);

  return 0;
}

/**********************************************************************/ /**
   Connect to a freeciv-server instance -- or at least try to.  On success,
   return 0; on failure, put an error message in ERRBUF and return -1.
 **************************************************************************/
int connect_to_server(QString &username, QString &hostname, int port,
                      char *errbuf, int errbufsize)
{
  if (errbufsize > 0 && errbuf != NULL) {
    errbuf[0] = '\0';
  }

  if (0 != try_to_connect(hostname, port, username, errbuf, errbufsize)) {
    return -1;
  }

  if (gui_options.use_prev_server) {
    sz_strlcpy(gui_options.default_server_host, qUtf8Printable(hostname));
    gui_options.default_server_port = port;
  }

  return 0;
}

/**********************************************************************/ /**
   Called after a connection is completed (e.g., in try_to_connect).
 **************************************************************************/
void make_connection(QTcpSocket *sock, QString &username)
{
  struct packet_server_join_req req;

  connection_common_init(&client.conn);
  client.conn.sock = sock;
  client.conn.client.last_request_id_used = 0;
  client.conn.client.last_processed_request_id_seen = 0;
  client.conn.client.request_id_of_currently_handled_packet = 0;
  client.conn.incoming_packet_notify = notify_about_incoming_packet;
  client.conn.outgoing_packet_notify = notify_about_outgoing_packet;

  /* call gui-dependent stuff in gui_main.c */
  add_net_input(client.conn.sock);

  /* now send join_request package */

  req.major_version = MAJOR_VERSION;
  req.minor_version = MINOR_VERSION;
  req.patch_version = PATCH_VERSION;
  sz_strlcpy(req.version_label, VERSION_LABEL);
  sz_strlcpy(req.capability, our_capability);
  sz_strlcpy(req.username, qUtf8Printable(username));

  send_packet_server_join_req(&client.conn, &req);
}

/**********************************************************************/ /**
   Get rid of server connection. This also kills internal server if it's
   used.
 **************************************************************************/
void disconnect_from_server(void)
{
  const bool force = !client.conn.used;

  attribute_flush();

  stop_turn_change_wait();

  /* If it's internal server - kill him
   * We assume that we are always connected to the internal server  */
  if (!force) {
    client_kill_server(FALSE);
  }
  close_socket_nomessage(&client.conn);
  if (force) {
    client_kill_server(TRUE);
  }
  output_window_append(ftc_client, _("Disconnected from server."));

  if (gui_options.save_options_on_exit) {
    options_save(NULL);
  }
}

/**********************************************************************/ /**
   A wrapper around read_socket_data() which also handles the case the
   socket becomes writeable and there is still data which should be sent
   to the server.

   Returns:
     -1  :  an error occurred - you should close the socket FIXME dropped!
     -2  :  the connection was closed
     >0  :  number of bytes read
     =0  :  no data read, would block
 **************************************************************************/
static int read_from_connection(struct connection *pc, bool block)
{
  QTcpSocket *socket = pc->sock;
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
    socket->waitForReadyRead();
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

/**********************************************************************/ /**
   This function is called when the client received a new input from the
   server.
 **************************************************************************/
void input_from_server(QTcpSocket *sock)
{
  int nb;

  fc_assert_ret(sock == client.conn.sock);

  nb = read_from_connection(&client.conn, FALSE);
  if (0 <= nb) {
    agents_freeze_hint();
    while (client.conn.used) {
      enum packet_type type;
      void *packet = get_packet_from_connection(&client.conn, &type);

      if (NULL != packet) {
        client_packet_input(packet, type);
        ::operator delete(packet);
      } else {
        break;
      }
    }
    if (client.conn.used) {
      agents_thaw_hint();
    }
  } else if (-2 == nb) {
    connection_close(&client.conn, _("server disconnected"));
  } else {
    connection_close(&client.conn, _("read error"));
  }
}

/**********************************************************************/ /**
   This function will sniff from the given socket, get the packet and call
   client_packet_input. It will return if there is a network error or if
   the PACKET_PROCESSING_FINISHED packet for the given request is
   received.
 **************************************************************************/
void input_from_server_till_request_got_processed(QTcpSocket *socket,
                                                  int expected_request_id)
{
  fc_assert_ret(expected_request_id);
  fc_assert_ret(socket == client.conn.sock);

  log_debug("input_from_server_till_request_got_processed("
            "expected_request_id=%d)",
            expected_request_id);

  while (TRUE) {
    int nb = read_from_connection(&client.conn, TRUE);

    if (0 <= nb) {
      enum packet_type type;

      while (TRUE) {
        void *packet = get_packet_from_connection(&client.conn, &type);
        if (NULL == packet) {
          break;
        }

        client_packet_input(packet, type);
        free(packet);

        if (type == PACKET_PROCESSING_FINISHED) {
          log_debug("ifstrgp: expect=%d, seen=%d", expected_request_id,
                    client.conn.client.last_processed_request_id_seen);
          if (client.conn.client.last_processed_request_id_seen
              >= expected_request_id) {
            log_debug("ifstrgp: got it; returning");
            return;
          }
        }
      }
    } else if (-2 == nb) {
      connection_close(&client.conn, _("server disconnected"));
      break;
    } else {
      connection_close(&client.conn, _("read error"));
      break;
    }
  }
}

static bool autoconnecting = FALSE;
/**********************************************************************/ /**
   Make an attempt to autoconnect to the server.
   It returns number of seconds it should be called again.
 **************************************************************************/
double try_to_autoconnect(void)
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
    qFatal(_("Failed to contact server \"%s\" at port "
             "%d as \"%s\" after %d attempts"),
           qUtf8Printable(server_host), server_port,
           qUtf8Printable(user_name), count);
    exit(EXIT_FAILURE);
  }

  if (try_to_connect(server_host, server_port, user_name, errbuf,
                     sizeof(errbuf))
      == 0) {
    // Success! Don't call me again
    autoconnecting = FALSE;
    return FC_INFINITY;
  } else {
    // All errors are fatal
    qFatal(_("Error contacting server \"%s\" at port %d "
             "as \"%s\":\n %s\n"),
           qUtf8Printable(server_host), server_port,
           qUtf8Printable(user_name), errbuf);
    exit(EXIT_FAILURE);
  }
}

/**********************************************************************/ /**
   Start trying to autoconnect to freeciv-server.  Calls
   get_server_address(), then arranges for try_to_autoconnect(), which
   calls try_to_connect(), to be called roughly every
   AUTOCONNECT_INTERVAL milliseconds, until success, fatal error or
   user intervention.
 **************************************************************************/
void start_autoconnecting_to_server(void)
{
  output_window_printf(
      ftc_client,
      _("Auto-connecting to server \"%s\" at port %d "
        "as \"%s\" every %f second(s) for %d times"),
      qUtf8Printable(server_host), server_port, qUtf8Printable(user_name),
      0.001 * AUTOCONNECT_INTERVAL, MAX_AUTOCONNECT_ATTEMPTS);

  autoconnecting = TRUE;
}
