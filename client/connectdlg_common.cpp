/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/
#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

// Qt
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QTcpServer>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

/* utility */
#include "astring.h"
#include "capability.h"
#include "deprecations.h"
#include "fciconv.h"
#include "fcintl.h"
#include "ioz.h"
#include "log.h"
#include "net_types.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"
#include "climisc.h"
#include "clinet.h" /* connect_to_server() */
#include "connectdlg_common.h"
#include "connectdlg_g.h"
#include "packhand_gen.h"
#include "tilespec.h"

#define WAIT_BETWEEN_TRIES 100000 /* usecs */
#define NUMBER_OF_TRIES 500

bool server_quitting = FALSE;

static char challenge_fullname[MAX_LEN_PATH];
static bool client_has_hack = FALSE;

int internal_server_port;

class serverProcess : public QProcess {
  Q_DISABLE_COPY(serverProcess);

public:
  static serverProcess *i();

private:
  void drop();
  serverProcess();
  static serverProcess *m_instance;
};
serverProcess *serverProcess::m_instance = 0;

/* Server process constructor */
serverProcess::serverProcess()
{
  connect(this,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [=](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode)
            qInfo() << _("Freeciv Server") << exitStatus;
            drop();
          });
}
/* Server process instance */
serverProcess *serverProcess::i()
{
  if (!m_instance) {
    m_instance = new serverProcess;
  }
  return m_instance;
}
/* Removes server process */
void serverProcess::drop()
{
  if (m_instance) {
    m_instance->deleteLater();
    m_instance = 0;
  }
}

/**************************************************************************
The general chain of events:

Two distinct paths are taken depending on the choice of mode:

if the user selects the multi- player mode, then a packet_req_join_game
packet is sent to the server. It is either successful or not. The end.

If the user selects a single- player mode (either a new game or a save game)
then:
   1. the packet_req_join_game is sent.
   2. on receipt, if we can join, then a challenge packet is sent to the
      server, so we can get hack level control.
   3. if we can't get hack, then we get dumped to multi- player mode. If
      we can, then:
      a. for a new game, we send a series of packet_generic_message packets
         with commands to start the game.
      b. for a saved game, we send the load command with a
         packet_generic_message, then we send a PACKET_PLAYER_LIST_REQUEST.
         the response to this request will tell us if the game was loaded or
         not. if not, then we send another load command. if so, then we send
         a series of packet_generic_message packets with commands to start
         the game.
**************************************************************************/

/**********************************************************************/ /**
   Tests if the client has started the server.
 **************************************************************************/
bool is_server_running(void)
{
  if (server_quitting) {
    return FALSE;
  }
  return serverProcess::i()->state();
}

/**********************************************************************/ /**
   Returns TRUE if the client has hack access.
 **************************************************************************/
bool can_client_access_hack(void) { return client_has_hack; }

/**********************************************************************/ /**
   Kills the server if the client has started it.

   If the 'force' parameter is unset, we just do a /quit.  If it's set, then
   we'll send a signal to the server to kill it (use this when the socket
   is disconnected already).
 **************************************************************************/
void client_kill_server(bool force)
{
  if (is_server_running()) {
    if (client.conn.used && client_has_hack) {
      /* This does a "soft" shutdown of the server by sending a /quit.
       *
       * This is useful when closing the client or disconnecting because it
       * doesn't kill the server prematurely.  In particular, killing the
       * server in the middle of a save can have disastrous results.  This
       * method tells the server to quit on its own.  This is safer from a
       * game perspective, but more dangerous because if the kill fails the
       * server will be left running.
       *
       * Another potential problem is because this function is called atexit
       * it could potentially be called when we're connected to an unowned
       * server.  In this case we don't want to kill it. */
      send_chat("/quit");
      server_quitting = TRUE;
    } else if (force) {
      /* Either we already disconnected, or we didn't get control of the
       * server. In either case, the only thing to do is a "hard" kill of
       * the server. */
      serverProcess::i()->kill();
      server_quitting = FALSE;
    }
  }

  client_has_hack = FALSE;
}

/*********************************************************************/ /**
   Finds the next (lowest) free port.
 *************************************************************************/
static int find_next_free_port(int starting_port, int highest_port)
{
  // Make sure it's destroyed and resources are cleaned up on return
  QTcpServer server;

  // Simply attempt to listen until we find a port that works
  for (int port = starting_port; port < highest_port; ++port) {
    if (server.listen(QHostAddress::LocalHost, port)) {
      return port;
    }
  }

  return -1;
}

/**********************************************************************/ /**
   Forks a server if it can. Returns FALSE if we find we
   couldn't start the server.
 **************************************************************************/
bool client_start_server(void)
{
  QStringList program = {QStringLiteral("freeciv-server.exe"),
                         "./freeciv-server", "freeciv-server"};
  QStringList arguments;
  QString trueFcser, ruleset, storage, port_buf, savesdir, scensdir;
  char buf[512];
  int connect_tries = 0;

  /* only one server (forked from this client) shall be running at a time */
  /* This also resets client_has_hack. */
  client_kill_server(TRUE);

  output_window_append(ftc_client, _("Starting local server..."));

  /* find a free port */
  /* Mitigate the risk of ending up with the port already
   * used by standalone server on Windows where this is known to be buggy
   * by not starting from DEFAULT_SOCK_PORT but from one higher. */
  internal_server_port = find_next_free_port(DEFAULT_SOCK_PORT + 1,
                                             DEFAULT_SOCK_PORT + 1 + 10000);

  if (internal_server_port < 0) {
    output_window_append(ftc_client, _("Couldn't start the server."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));
    return FALSE;
  }

  storage = QString::fromUtf8(freeciv_storage_dir());
  if (storage == NULL) {
    output_window_append(ftc_client,
                         _("Cannot find freeciv storage directory"));
    output_window_append(
        ftc_client, _("You'll have to start server manually. Sorry..."));
    return FALSE;
  }

  ruleset = QString::fromUtf8(tileset_what_ruleset(tileset));

  /* Set up the command-line parameters. */
  port_buf = QString::number(internal_server_port);
  savesdir = QStringLiteral("%1%2saves").arg(storage, DIR_SEPARATOR);
  scensdir = QStringLiteral("%1%2scenarios").arg(storage, DIR_SEPARATOR);

  arguments << QStringLiteral("-p") << port_buf << QStringLiteral("--bind")
            << QStringLiteral("localhost") << QStringLiteral("-q")
            << QStringLiteral("1") << QStringLiteral("-e")
            << QStringLiteral("--saves") << savesdir
            << QStringLiteral("--scenarios") << scensdir
            << QStringLiteral("-A") << QStringLiteral("none");
  if (!logfile.isEmpty()) {
    arguments << QStringLiteral("--debug") << log_get_level()
              << QStringLiteral("--log") << logfile;
  }
  if (scriptfile.isEmpty()) {
    arguments << QStringLiteral("--read") << scriptfile;
  }
  if (savefile.isEmpty()) {
    arguments << QStringLiteral("--file") << savefile;
  }
  if (ruleset != NULL) {
    arguments << QStringLiteral("--ruleset") << ruleset;
  }

  for (auto const &trueServer : qAsConst(program)) {
    serverProcess::i()->start(trueServer, arguments);
    trueFcser = trueServer;
    if (serverProcess::i()->waitForStarted(3000) == true) {
      break;
    }
  }
  // Wait for the server to print its welcome screen
  serverProcess::i()->waitForReadyRead();
  server_quitting = FALSE;
  /* a reasonable number of tries */
  QString srv = QStringLiteral("localhost");
  while (connect_to_server(
             user_name, srv, internal_server_port, buf,
             sizeof(buf) && serverProcess::i()->state() == QProcess::Running)
         == -1) {
    fc_usleep(WAIT_BETWEEN_TRIES);
    if (connect_tries++ > NUMBER_OF_TRIES) {
      qCritical("Last error from connect attempts: '%s'", buf);
      break;
    }
  }
  /* weird, but could happen, if server doesn't support new startup stuff
   * capabilities won't help us here... */
  if (!client.conn.used || serverProcess::i()->processId() == 0) {
    /* possible that server is still running. kill it, kill it with Igni */
    client_kill_server(TRUE);

    qCritical("Failed to connect to spawned server!");
#ifdef FREECIV_DEBUG
    qDebug("Tried with commandline: '%s'",
           QString(trueFcser + arguments.join(QStringLiteral(" ")))
               .toLocal8Bit()
               .data());
#endif
    output_window_append(ftc_client, _("Couldn't connect to the server."));
    output_window_append(ftc_client,
                         _("We probably couldn't start it from here."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));

    return FALSE;
  }

  /* We set the topology to match the view.
   *
   * When a typical player launches a game, he wants the map orientation to
   * match the tileset orientation.  So if you use an isometric tileset you
   * get an iso-map and for a classic tileset you get a classic map.  In
   * both cases the map wraps in the X direction by default.
   *
   * This works with hex maps too now.  A hex map always has
   * tileset_is_isometric(tileset) return TRUE.  An iso-hex map has
   * tileset_hex_height(tileset) != 0, while a non-iso hex map
   * has tileset_hex_width(tileset) != 0.
   *
   * Setting the option here is a bit of a hack, but so long as the client
   * has sufficient permissions to do so (it doesn't have HACK access yet) it
   * is safe enough.  Note that if you load a savegame the topology will be
   * set but then overwritten during the load.
   *
   * Don't send it now, it will be sent to the server when receiving the
   * server setting infos. */
  {
    char topobuf[16];

    fc_strlcpy(topobuf, "WRAPX", sizeof(topobuf));
    if (tileset_is_isometric(tileset) && 0 == tileset_hex_height(tileset)) {
      fc_strlcat(topobuf, "|ISO", sizeof(topobuf));
    }
    if (0 < tileset_hex_width(tileset) || 0 < tileset_hex_height(tileset)) {
      fc_strlcat(topobuf, "|HEX", sizeof(topobuf));
    }
    desired_settable_option_update("topology", topobuf, FALSE);
  }

  return TRUE;
}

/**********************************************************************/ /**
   Generate a random string.
 **************************************************************************/
static void randomize_string(char *str, size_t n)
{
  const char chars[] =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;

  for (i = 0; i < n - 1; i++) {
    str[i] = chars[fc_rand(sizeof(chars) - 1)];
  }
  str[i] = '\0';
}

/**********************************************************************/ /**
   If the client is capable of 'wanting hack', then the server will
   send the client a filename in the packet_join_game_reply packet.

   This function creates the file with a suitably random string in it
   and then sends the string to the server. If the server can open
   and read the string, then the client is given hack access.
 **************************************************************************/
void send_client_wants_hack(const char *filename)
{
  if (filename[0] != '\0') {
    struct packet_single_want_hack_req req;
    struct section_file *file;
    const char *sdir = freeciv_storage_dir();

    if (sdir == NULL) {
      return;
    }

    if (!is_safe_filename(filename)) {
      return;
    }

    make_dir(sdir);

    fc_snprintf(challenge_fullname, sizeof(challenge_fullname),
                "%s%c%s", sdir, DIR_SEPARATOR_CHAR, filename);

    /* generate an authentication token */
    randomize_string(req.token, sizeof(req.token));

    file = secfile_new(FALSE);
    secfile_insert_str(file, req.token, "challenge.token");
    if (!secfile_save(file, challenge_fullname, 0, FZ_PLAIN)) {
      qCritical("Couldn't write token to temporary file: %s",
                challenge_fullname);
    }
    secfile_destroy(file);

    /* tell the server what we put into the file */
    send_packet_single_want_hack_req(&client.conn, &req);
  }
}

/**********************************************************************/ /**
   Handle response (by the server) if the client has got hack or not.
 **************************************************************************/
void handle_single_want_hack_reply(bool you_have_hack)
{
  /* remove challenge file */
  if (challenge_fullname[0] != '\0') {
    if (fc_remove(challenge_fullname) == -1) {
      qCritical("Couldn't remove temporary file: %s", challenge_fullname);
    }
    challenge_fullname[0] = '\0';
  }

  if (you_have_hack) {
    output_window_append(ftc_client,
                         _("Established control over the server. "
                           "You have command access level 'hack'."));
    client_has_hack = TRUE;
  } else if (is_server_running()) {
    /* only output this if we started the server and we NEED hack */
    output_window_append(ftc_client,
                         _("Failed to obtain the required access "
                           "level to take control of the server. "
                           "Attempting to shut down server."));
    client_kill_server(TRUE);
  }
}

/**********************************************************************/ /**
   Send server command to save game.
 **************************************************************************/
void send_save_game(const char *filename)
{
  if (filename) {
    send_chat_printf("/save %s", filename);
  } else {
    send_chat("/save");
  }
}

/**********************************************************************/ /**
   Handle the list of rulesets sent by the server.
 **************************************************************************/
void handle_ruleset_choices(const struct packet_ruleset_choices *packet)
{
  char **rulesets = new char*[packet->ruleset_count];
  int i;
  size_t suf_len = qstrlen(RULESET_SUFFIX);

  for (i = 0; i < packet->ruleset_count; i++) {
    size_t len = qstrlen(packet->rulesets[i]);

    rulesets[i] = fc_strdup(packet->rulesets[i]);

    if (len > suf_len
        && strcmp(rulesets[i] + len - suf_len, RULESET_SUFFIX) == 0) {
      rulesets[i][len - suf_len] = '\0';
    }
  }
  set_rulesets(packet->ruleset_count, rulesets);

  for (i = 0; i < packet->ruleset_count; i++) {
    delete[] rulesets[i];
  }
  delete[] rulesets;
}

/**********************************************************************/ /**
   Called by the GUI code when the user sets the ruleset.  The ruleset
   passed in here should match one of the strings given to set_rulesets().
 **************************************************************************/
void set_ruleset(const char *ruleset)
{
  char buf[4096];

  fc_snprintf(buf, sizeof(buf), "/read %s%s", ruleset, RULESET_SUFFIX);
  log_debug("Executing '%s'", buf);
  send_chat(buf);
}
