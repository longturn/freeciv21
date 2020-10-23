/*
 * (c) Copyright 2020 The Freeciv21 contributors
 *
 * This file is part of Freeciv21.
 *
 * Freeciv21 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Freeciv21 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Freeciv21.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "server.h"

// Qt
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHostInfo>
#include <QTcpServer>
#include <QTcpSocket>

// Stuff to wait for input on stdin.
#ifdef Q_OS_WIN
#include <QtCore/QWinEventNotifier>
#include <windows.h>
#else
#include <QtCore/QSocketNotifier>
#include <unistd.h> // STDIN_FILENO
#endif

// Readline
#include <readline/history.h>
#include <readline/readline.h>

// utility
#include "fciconv.h" // local_to_internal_string_malloc

// common
#include "fc_interface.h"

// server
#include "ai.h"
#include "connecthand.h"
#include "console.h"
#include "diplhand.h"
#include "edithand.h"
#include "maphand.h"
#include "mapimg.h"
#include "meta.h"
#include "ruleset.h"
#include "sernet.h"
#include "settings.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "timing.h"
#include "voting.h"

using namespace freeciv;

static const char *HISTORY_FILENAME = "freeciv-server_history";
static const int HISTORY_LENGTH = 100;

namespace {

/*************************************************************************/ /**
   Readline callback for input.
 *****************************************************************************/
void handle_readline_input_callback(char *line)
{
  if (line == nullptr) {
    return;
  }

  if (line[0] != '\0') {
    add_history(line);
  }

  con_prompt_enter(); /* just got an 'Enter' hit */
  auto line_internal = local_to_internal_string_malloc(line);
  (void) handle_stdin_input(NULL, line_internal);
  free(line_internal);
  free(line);
}

/**********************************************************************/ /**
   Initialize server specific functions.
 **************************************************************************/
void fc_interface_init_server(void)
{
  struct functions *funcs = fc_interface_funcs();

  funcs->server_setting_by_name = server_ss_by_name;
  funcs->server_setting_name_get = server_ss_name_get;
  funcs->server_setting_type_get = server_ss_type_get;
  funcs->server_setting_val_bool_get = server_ss_val_bool_get;
  funcs->server_setting_val_int_get = server_ss_val_int_get;
  funcs->server_setting_val_bitwise_get = server_ss_val_bitwise_get;
  funcs->create_extra = create_extra;
  funcs->destroy_extra = destroy_extra;
  funcs->player_tile_vision_get = map_is_known_and_seen;
  funcs->player_tile_city_id_get = server_plr_tile_city_id_get;
  funcs->gui_color_free = server_gui_color_free;

  // Keep this function call at the end. It checks if all required functions
  // are defined.
  fc_interface_init();
}

/**********************************************************************/ /**
   Server initialization.
 **************************************************************************/
QTcpServer *srv_prepare()
{
#ifdef HAVE_FCDB
  if (!srvarg.auth_enabled) {
    con_write(C_COMMENT, _("This freeciv-server program has player "
                           "authentication support, but it's currently not "
                           "in use."));
  }
#endif /* HAVE_FCDB */

  /* make sure it's initialized */
  srv_init();

  fc_init_network();

  /* must be before con_log_init() */
  init_connections();
  con_log_init(srvarg.log_filename, srvarg.loglevel,
               srvarg.fatal_assertions);
  /* logging available after this point */

  auto tcp_server = server_open_socket();

#if IS_BETA_VERSION
  con_puts(C_COMMENT, "");
  con_puts(C_COMMENT, beta_message());
  con_puts(C_COMMENT, "");
#endif

  con_flush();

  settings_init(TRUE);
  stdinhand_init();
  edithand_init();
  voting_init();
  diplhand_init();
  voting_init();
  ai_timer_init();

  server_game_init(FALSE);
  mapimg_init(mapimg_server_tile_known, mapimg_server_tile_terrain,
              mapimg_server_tile_owner, mapimg_server_tile_city,
              mapimg_server_tile_unit, mapimg_server_plrcolor_count,
              mapimg_server_plrcolor_get);

#ifdef HAVE_FCDB
  if (srvarg.fcdb_enabled) {
    bool success;

    success = fcdb_init(srvarg.fcdb_conf);
    free(srvarg.fcdb_conf); /* Never needed again */
    srvarg.fcdb_conf = NULL;
    if (!success) {
      QCoreApplication::exit(EXIT_FAILURE);
      return tcp_server;
    }
  }
#endif /* HAVE_FCDB */

  if (srvarg.ruleset != NULL) {
    const char *testfilename;

    testfilename = fileinfoname(get_data_dirs(), srvarg.ruleset);
    if (testfilename == NULL) {
      log_fatal(_("Ruleset directory \"%s\" not found"), srvarg.ruleset);
      QCoreApplication::exit(EXIT_FAILURE);
      return tcp_server;
    }
    sz_strlcpy(game.server.rulesetdir, srvarg.ruleset);
  }

  /* load a saved game */
  if ('\0' == srvarg.load_filename[0]
      || !load_command(NULL, srvarg.load_filename, FALSE, TRUE)) {
    /* Rulesets are loaded on game initialization, but may be changed later
     * if /load or /rulesetdir is done. */
    load_rulesets(NULL, NULL, FALSE, NULL, TRUE, FALSE, TRUE);
  }

  maybe_automatic_meta_message(default_meta_message_string());

  if (!(srvarg.metaserver_no_send)) {
    log_normal(_("Sending info to metaserver <%s>."), meta_addr_port());
    /* Open socket for meta server */
    if (!server_open_meta(srvarg.metaconnection_persistent)
        || !send_server_info_to_metaserver(META_INFO)) {
      con_write(C_FAIL, _("Not starting without explicitly requested "
                          "metaserver connection."));
      QCoreApplication::exit(EXIT_FAILURE);
      return tcp_server;
    }
  }

  return tcp_server;
}

} // anonymous namespace

/*************************************************************************/ /**
   Creates a server. It starts working as soon as there is an event loop.
 *****************************************************************************/
server::server()
{
  // Get notifications when there's some input on stdin. This is OS-dependent
  // and Qt doesn't have a wrapper. Maybe it should be split to a separate
  // class.
#ifdef Q_OS_WIN
  {
    auto handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle != INVALID_HANDLE_VALUE) {
      auto notifier = new QWinEventNotifier(handle, this);
      connect(notifier, &QWinEventNotifier::activated, this,
              &server::input_on_stdin);
      m_stdin_notifier = notifier;
    }
  }
#else
  {
    // Unix-like
    auto notifier =
        new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this,
            &server::input_on_stdin);
    m_stdin_notifier = notifier;
  }
#endif

  // Are we running an interactive session?
#ifdef Q_OS_WIN
  // isatty and fileno are deprecated on Windows
  m_interactive = _isatty(_fileno(stdin));
#else
  m_interactive = isatty(fileno(stdin));
#endif
  if (m_interactive) {
    init_interactive();
  }

  // Now init the old C API
  fc_interface_init_server();
  m_tcp_server = srv_prepare();
  m_tcp_server->setParent(this);
  connect(m_tcp_server, &QTcpServer::newConnection, this,
          &server::accept_connections);
  connect(m_tcp_server, &QTcpServer::acceptError,
          [](QAbstractSocket::SocketError error) {
            log_error("Error accepting connection: %d", error);
          });

  m_eot_timer = timer_new(TIMER_CPU, TIMER_ACTIVE);
}

/*************************************************************************/ /**
   Shut down a server.
 *****************************************************************************/
server::~server()
{
  if (m_interactive) {
    // Save history
    auto history_file = QString::fromUtf8(freeciv_storage_dir())
                        + QLatin1String("/")
                        + QLatin1String(HISTORY_FILENAME);
    auto history_file_encoded = history_file.toLocal8Bit();
    write_history(history_file_encoded.constData());
    history_truncate_file(history_file_encoded.constData(), HISTORY_LENGTH);
    clear_history();

    // Power down readline
    rl_callback_handler_remove();
  }

  if (m_eot_timer != nullptr) {
    timer_destroy(m_eot_timer);
  }
}

/*************************************************************************/ /**
   Initializes interactive handling of stdin with libreadline.
 *****************************************************************************/
void server::init_interactive()
{
  // Read the history file
  auto storage_dir = QString::fromUtf8(freeciv_storage_dir());
  if (QDir().mkpath(storage_dir)) {
    auto history_file =
        storage_dir + QLatin1String("/") + QLatin1String(HISTORY_FILENAME);
    using_history();
    read_history(history_file.toLocal8Bit().constData());
  }

  // Initialize readline
  rl_initialize();
  rl_callback_handler_install((char *) "> ", handle_readline_input_callback);
  rl_attempted_completion_function = freeciv_completion;
}

/*************************************************************************/ /**
   Server accepts connections from client:
   Low level socket stuff, and basic-initialize the connection struct.
 *****************************************************************************/
void server::accept_connections()
{
  // There may be several connections available.
  while (m_tcp_server->hasPendingConnections()) {
    auto socket = m_tcp_server->nextPendingConnection();
    socket->setParent(this);

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
                  qUtf8Printable(remote), MAX_NUM_CONNECTIONS);
      socket->deleteLater();
      continue;
    }

    // Reject the connection if we have reached the limit for this host
    if (0 != game.server.maxconnectionsperhost) {
      bool success = true;
      int count = 0;

      conn_list_iterate(game.all_connections, pconn)
      {
        // Use TolerantConversion so one connections from the same address on
        // IPv4 and IPv6 are rejected as well.
        if (socket->peerAddress().isEqual(
                pconn->sock->peerAddress(),
                QHostAddress::TolerantConversion)) {
          continue;
        }
        if (++count >= game.server.maxconnectionsperhost) {
          log_verbose("Rejecting new connection from %s: maximum number of "
                      "connections for this address exceeded (%d).",
                      qUtf8Printable(remote),
                      game.server.maxconnectionsperhost);

          success = false;
          socket->deleteLater();
        }
      }
      conn_list_iterate_end;

      if (!success) {
        continue;
      }
    }

    if (server_make_connection(socket, remote) == 0) {
      // Success making the connection, connect signals
      connect(socket, &QIODevice::readyRead, this, &server::input_on_socket);
      connect(socket,
              QOverload<QAbstractSocket::SocketError>::of(
                  &QAbstractSocket::error),
              this, &server::error_on_socket);
    }
  }
}

/*************************************************************************/ /**
   Called when there was an error on a socket.
 *****************************************************************************/
void server::error_on_socket()
{
  // Get the socket
  auto socket = dynamic_cast<QTcpSocket *>(sender());
  if (socket == nullptr) {
    return;
  }

  // Find the corresponding connection
  conn_list_iterate(game.all_connections, pconn)
  {
    if (pconn->sock == socket) {
      connection_close_server(pconn, _("network exception"));
      break;
    }
  }
  conn_list_iterate_end
}

/*************************************************************************/ /**
   Called when there's something to read on a socket.
 *****************************************************************************/
void server::input_on_socket()
{
  // Get the socket
  auto socket = dynamic_cast<QTcpSocket *>(sender());
  if (socket == nullptr) {
    return;
  }

  // Find the corresponding connection
  conn_list_iterate(game.all_connections, pconn)
  {
    if (pconn->sock == socket && !pconn->server.is_closing) {
      auto nb = read_socket_data(pconn->sock, pconn->buffer);
      if (0 <= nb) {
        /* We read packets; now handle them. */
        incoming_client_packets(pconn);
      } else if (-2 == nb) {
        connection_close_server(pconn, _("client disconnected"));
      } else {
        /* Read failure; the connection is closed. */
        connection_close_server(pconn, _("read error"));
      }
      break;
    }
  }
  conn_list_iterate_end
}

/*************************************************************************/ /**
   Called when there's something to read on stdin.
 *****************************************************************************/
void server::input_on_stdin()
{
  if (m_interactive) {
    // Readline does everything nicely in interactive sessions
    rl_callback_read_char();
  } else {
    // Read from the input
    QFile f;
    f.open(stdin, QIODevice::ReadOnly);
    if (f.atEnd() && m_stdin_notifier != nullptr) {
      // QSocketNotifier gets mad after EOF. Turn it off.
      m_stdin_notifier->deleteLater();
      m_stdin_notifier = nullptr;
      log_normal(_("Reached end of standard input."));
    } else {
      // Got something to read. Hopefully there's even a complete line and
      // we can process it.
      auto line = f.readLine();
      auto non_const_line =
          local_to_internal_string_malloc(line.constData());
      (void) handle_stdin_input(NULL, non_const_line);
      free(non_const_line);
    }
  }
}
