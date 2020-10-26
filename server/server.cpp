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
#include <QTimer>

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
#include "rand.h"

// common
#include "fc_interface.h"

// server
#include "ai.h"
#include "aiiface.h"
#include "auth.h"
#include "connecthand.h"
#include "console.h"
#include "diplhand.h"
#include "edithand.h"
#include "maphand.h"
#include "mapimg.h"
#include "meta.h"
#include "notify.h"
#include "ruleset.h"
#include "sanitycheck.h"
#include "savemain.h"
#include "score.h"
#include "script_server.h" // scripting
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

  /* must be before con_log_init() */
  init_connections();
  con_log_init(srvarg.log_filename, srvarg.loglevel,
               srvarg.fatal_assertions);
  /* logging available after this point */

  auto tcp_server = server_open_socket();
  if (!tcp_server->isListening()) {
    // Don't even try to start a game.
    return tcp_server;
  }

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
    : m_is_new_turn(false), m_need_send_pending_events(false),
      m_skip_mapimg(false)
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
  if (!m_tcp_server->isListening()) {
    // Could not listen on the specified port. Rely on the caller checking
    // our state and not starting the event loop.
    return;
  }
  connect(m_tcp_server, &QTcpServer::newConnection, this,
          &server::accept_connections);
  connect(m_tcp_server, &QTcpServer::acceptError,
          [](QAbstractSocket::SocketError error) {
            log_error("Error accepting connection: %d", error);
          });

  m_eot_timer = timer_new(TIMER_CPU, TIMER_ACTIVE);

  // Prepare a game
  prepare_game();

  // Start pulsing
  m_pulse_timer = new QTimer(this);
  m_pulse_timer->start(1000);
  connect(m_pulse_timer, &QTimer::timeout, this, &server::pulse);
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
  if (m_between_turns_timer != nullptr) {
    timer_destroy(m_between_turns_timer);
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
   Checks if the server is ready for the event loop to start. In practice,
   this is only false if opening the port failed.
 *****************************************************************************/
bool server::is_ready() const { return m_tcp_server->isListening(); }

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

      // Prevents quitidle from firing immediately
      m_someone_ever_connected = true;

      // Turn off the quitidle timeout if it's running
      if (m_quitidle_timer != nullptr) {
        // There may be a "race condition" here if the timeout signal is
        // already queued and we're going to quit. This should be fairly rare
        // in practice.
        m_quitidle_timer->stop();
        m_quitidle_timer->deleteLater();
        m_quitidle_timer = nullptr;
      }
    }
  }
}

/*************************************************************************/ /**
   Sends pings to clients if needed.
 *****************************************************************************/
void server::send_pings()
{
  // Pinging around for statistics
  if (time(NULL) > (game.server.last_ping + game.server.pingtime)) {
    // send data about the previous run
    send_ping_times_to_all();

    conn_list_iterate(game.all_connections, pconn)
    {
      if ((!pconn->server.is_closing
           && 0 < timer_list_size(pconn->server.ping_timers)
           && timer_read_seconds(timer_list_front(pconn->server.ping_timers))
                  > game.server.pingtimeout)
          || pconn->ping_time > game.server.pingtimeout) {
        // cut mute players, except for hack-level ones
        if (pconn->access_level == ALLOW_HACK) {
          log_verbose("connection (%s) [hack-level] ping timeout ignored",
                      conn_description(pconn));
        } else {
          log_verbose("connection (%s) cut due to ping timeout",
                      conn_description(pconn));
          connection_close_server(pconn, _("ping timeout"));
        }
      } else if (pconn->established) {
        // We don't send ping to connection not established, because we
        // wouldn't be able to handle asynchronous ping/pong with different
        // packet header size.
        connection_ping(pconn);
      }
    }
    conn_list_iterate_end;
    game.server.last_ping = time(NULL);
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

  really_close_connections();
  update_game_state();
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

  really_close_connections();
  update_game_state();
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

  update_game_state();
}

/*************************************************************************/ /**
   Prepares for a new game.
 *****************************************************************************/
void server::prepare_game()
{
  set_server_state(S_S_INITIAL);

  /* Load a script file. */
  if (NULL != srvarg.script_filename) {
    /* Adding an error message more here will duplicate them. */
    (void) read_init_script(NULL, srvarg.script_filename, TRUE, FALSE);
  }

  (void) aifill(game.info.aifill);
  if (!game_was_started()) {
    event_cache_clear();
  }

  log_normal(_("Now accepting new client connections on port %d."),
             srvarg.port);

  if (game.info.timeout == -1) {
    // Autogame, start as soon as the event loop allows
    QTimer::singleShot(0, this, &server::update_game_state);
  }
}

/*************************************************************************/ /**
   Do everything needed to start a new turn on top of calling begin_turn.
 *****************************************************************************/
void server::begin_turn()
{
  ::begin_turn(m_is_new_turn);

  // Start the first phase
  begin_phase();
}

/*************************************************************************/ /**
   Do everything needed to start a new phase on top of calling begin_phase.
 *****************************************************************************/
void server::begin_phase()
{
  log_debug("Starting phase %d/%d.", game.info.phase,
            game.server.num_phases);
  ::begin_phase(m_is_new_turn);
  if (m_need_send_pending_events) {
    // When loading a savegame, we need to send loaded events, after
    // the clients switched to the game page (after the first
    // packet_start_phase is received).
    conn_list_iterate(game.est_connections, pconn)
    {
      send_pending_events(pconn, true);
    }
    conn_list_iterate_end;
    m_need_send_pending_events = false;
  }

  m_is_new_turn = true;

  // This will thaw the reports and agents at the client.
  lsend_packet_thaw_client(game.est_connections);

#ifdef LOG_TIMERS
  // Before sniff (human player activites), report time to now:
  log_verbose("End/start-turn server/ai activities: %g seconds",
              timer_read_seconds(m_eot_timer));
#endif

  // Do auto-saves just before starting server_sniff_all_input(), so that
  // autosave happens effectively "at the same time" as manual
  // saves, from the point of view of restarting and AI players.
  // Post-increment so we don't count the first loop.
  if (game.info.phase == 0) {
    // Create autosaves if requested.
    if (m_save_counter >= game.server.save_nturns
        && game.server.save_nturns > 0) {
      m_save_counter = 0;
      save_game_auto("Autosave", AS_TURN);
    }
    m_save_counter++;

    if (!m_skip_mapimg) {
      // Save map image(s).
      for (int i = 0; i < mapimg_count(); i++) {
        struct mapdef *pmapdef = mapimg_isvalid(i);
        if (pmapdef != NULL) {
          mapimg_create(pmapdef, FALSE, game.server.save_name,
                        srvarg.saves_pathname);
        } else {
          log_error("%s", mapimg_error());
        }
      }
    } else {
      m_skip_mapimg = FALSE;
    }
  }

  log_debug("sniffingpackets");
  check_for_full_turn_done(); // HACK: don't wait during AI phases

  if (m_between_turns_timer != NULL) {
    game.server.turn_change_time = timer_read_seconds(m_between_turns_timer);
    log_debug("Inresponsive between turns %g seconds",
              game.server.turn_change_time);
  }

  QTimer::singleShot(0, this, &server::update_game_state);
}

/*************************************************************************/ /**
   Do everything needed to end a phase on top of calling end_phase.
 *****************************************************************************/
void server::end_phase()
{
  m_between_turns_timer =
      timer_renew(m_between_turns_timer, TIMER_USER, TIMER_ACTIVE);
  timer_start(m_between_turns_timer);

  /* After sniff, re-zero the timer: (read-out above on next loop) */
  timer_clear(m_eot_timer);
  timer_start(m_eot_timer);

  conn_list_do_buffer(game.est_connections);

  sanity_check();

  // This will freeze the reports and agents at the client.
  lsend_packet_freeze_client(game.est_connections);

  ::end_phase();

  conn_list_do_unbuffer(game.est_connections);

  if (S_S_OVER == server_state()) {
    end_turn();
    return;
  }
  game.server.additional_phase_seconds = 0;

  game.info.phase++;
  if (server_state() == S_S_RUNNING
      && game.info.phase < game.server.num_phases) {
    begin_phase();
  } else {
    end_turn();
  }
}

/*************************************************************************/ /**
   Do everything needed to end a turn on top of calling end_turn.
 *****************************************************************************/
void server::end_turn()
{
  ::end_turn();
  log_debug("Sendinfotometaserver");
  (void) send_server_info_to_metaserver(META_REFRESH);

  if (S_S_OVER != server_state() && check_for_game_over()) {
    set_server_state(S_S_OVER);
    if (game.info.turn > game.server.end_turn) {
      // endturn was reached - rank users based on team scores
      rank_users(TRUE);
    } else {
      // game ended for victory conditions - rank users based on survival
      rank_users(FALSE);
    }
  } else if (S_S_OVER == server_state()) {
    // game terminated by /endgame command - calculate team scores
    rank_users(TRUE);
  }

  if (server_state() == S_S_RUNNING) {
    // Still running, start the next turn!
    begin_turn();
  } else {
    // Game over
    // This will thaw the reports and agents at the client.
    lsend_packet_thaw_client(game.est_connections);

    if (game.server.save_timer != NULL) {
      timer_destroy(game.server.save_timer);
      game.server.save_timer = NULL;
    }
    if (m_between_turns_timer != nullptr) {
      timer_destroy(m_between_turns_timer);
      m_between_turns_timer = nullptr;
    }
    timer_clear(m_eot_timer);

    srv_scores();

    if (game.info.timeout == -1) {
      // Autogame, end game immediately if nobody is connected
      update_game_state();
    }
  }
}

/*************************************************************************/ /**
   Checks if the game state has changed and take action if appropriate.
 *****************************************************************************/
void server::update_game_state()
{
  // Set in the following cases:
  // - in pregame: game start
  // - during the game: turn done, end game and any other command affecting
  //                    game speed
  // It basically says: I got a command that requires to get out of the usual
  // "wait for clients to send stuff" mode.
  if (force_end_of_sniff) {
    force_end_of_sniff = false;

    if (server_state() < S_S_RUNNING) {
      // Pregame: start the game
      // If restarting for lack of players, the state is S_S_OVER,
      // so don't try to start the game.
      srv_ready(); // srv_ready() sets server state to S_S_RUNNING.

      timer_start(m_eot_timer);

      // This will freeze the reports and agents at the client.
      //
      // Do this before the starting the turn so that the PACKET_THAW_CLIENT
      // packet in begin_turn is balanced.
      lsend_packet_freeze_client(game.est_connections);

      // Start the first turn
      m_need_send_pending_events = !game.info.is_new_game;
      m_is_new_turn = game.info.is_new_game;
      m_save_counter = game.info.is_new_game ? 1 : 0;
      m_skip_mapimg = !game.info.is_new_game;
      begin_turn();
    } else {
      end_phase(); // Will end game if needed
    }
  }

  // Game over and all clients disconnected; restart if needed
  if (server_state() == S_S_OVER
      && conn_list_size(game.est_connections) == 0) {
    if (shut_game_down()) {
      prepare_game();
    }
  }

  // Set up the quitidle timer if not done already
  if (m_someone_ever_connected && m_quitidle_timer == nullptr
      && srvarg.quitidle != 0 && conn_list_size(game.est_connections) == 0) {

    if (srvarg.exit_on_end) {
      log_normal(_("Shutting down in %d seconds for lack of players."),
                 srvarg.quitidle);

      set_meta_message_string(N_("shutting down soon for lack of players"));
    } else {
      log_normal(_("Restarting in %d seconds for lack of players."),
                 srvarg.quitidle);

      set_meta_message_string(N_("restarting soon for lack of players"));
    }
    (void) send_server_info_to_metaserver(META_INFO);

    m_quitidle_timer = new QTimer(this);
    m_quitidle_timer->setSingleShot(true);
    m_quitidle_timer->start(1000 * srvarg.quitidle);
    connect(m_quitidle_timer, &QTimer::timeout, this, &server::quit_idle);
  }
}

/*************************************************************************/ /**
   Shuts a game down when all players have left. Returns whether a new game
   should be started.
 *****************************************************************************/
bool server::shut_game_down()
{
  /* Close it even between games. */
  save_system_close();

  if (game.info.timeout == -1 || srvarg.exit_on_end) {
    /* For autogames or if the -e option is specified, exit the server. */
    server_quit();
    return false;
  }

  /* Reset server */
  server_game_free();
  fc_rand_uninit();
  server_game_init(FALSE);
  mapimg_reset();
  load_rulesets(NULL, NULL, FALSE, NULL, TRUE, FALSE, TRUE);
  game.info.is_new_game = TRUE;
  return true;
}

/*************************************************************************/ /**
   Quit because we're idle (ie no one was connected in the last
   srvarg.quitidle seconds).
 *****************************************************************************/
void server::quit_idle()
{
  m_quitidle_timer = nullptr;

  if (srvarg.exit_on_end) {
    log_normal(_("Shutting down for lack of players."));
    set_meta_message_string("shutting down for lack of players");
  } else {
    log_normal(_("Restarting for lack of players."));
    set_meta_message_string("restarting for lack of players");
  }

  (void) send_server_info_to_metaserver(META_INFO);

  set_server_state(S_S_OVER);

  if (srvarg.exit_on_end) {
    // No need for anything more; just quit.
    server_quit();
  } else {
    force_end_of_sniff = true;
    update_game_state();
  }
}

/*************************************************************************/ /**
   Called every second.
 *****************************************************************************/
void server::pulse()
{
  send_pings();

  get_lanserver_announcement();

  // if we've waited long enough after a failure, respond to the client
  conn_list_iterate(game.all_connections, pconn)
  {
    if (srvarg.auth_enabled && !pconn->server.is_closing
        && pconn->server.status != AS_ESTABLISHED) {
      auth_process_status(pconn);
    }
  }
  conn_list_iterate_end

  call_ai_refresh();
  script_server_signal_emit("pulse");
  (void) send_server_info_to_metaserver(META_REFRESH);
  if (current_turn_timeout() > 0 && S_S_RUNNING == server_state()
      && game.server.phase_timer
      && (timer_read_seconds(game.server.phase_timer)
              + game.server.additional_phase_seconds
          > game.tinfo.seconds_to_phasedone)) {
    con_prompt_off();
    // This will be interpreted as "end phase"
    force_end_of_sniff = true;
    update_game_state();
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
}
